#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <asm-generic/socket.h>
#include <string.h>

void init_server_player(player_thread_args_t *arg)
{
  player_t *player = arg->player;
  server_t *server = arg->server;

  send_communication_metadata(server, player->player_id);
  printf("Sent communication metadata to player %d\n", player->player_id);

  char buffer[128] = {0};
  recv(player->sockfd, buffer, MAX_PLAYER_NAME_LENGTH, 0);
  strcpy(player->name, buffer);

  printf("Received player name: %s\n", player->name);

  send_game_metadata(server, player->player_id);
  printf("Sent game metadata to player %d\n", player->player_id);

  if (write(server->pipe_fds[PLAYER_PIPES_START + player->player_id][PIPE_WRITE], player->name, MAX_PLAYER_NAME_LENGTH) < 0)
  {
    perror("write failed");
    exit(1);
  }

  int opt = 0;
  if (read(server->pipe_fds[START_GAME_PIPE][PIPE_READ], &opt, sizeof(opt)) < 0)
  {
    perror("read failed");
    exit(1);
  }
}

void *player_thread(void *arg)
{
  player_thread_args_t *args = (player_thread_args_t *)arg;
  player_t *player = args->player;

  init_server_player(args);

  printf("Received game start signal for player %d\n", player->player_id);

  send_game_state(args->server, args->game, player->player_id);

  printf("Sent game state to player %d\n", player->player_id);

  while (1)
  { 
    request_type_t request_type;
    int response = recv(player->sockfd, &request_type, sizeof(request_type), 0);
    
    if (response < 0)
    {
      perror("recv failed");
      exit(1);
    }

    printf("Received request type %d from player %d\n", request_type, player->player_id);

    if (request_type == MAKE_ACTION)
    {
      receive_game_action(args->server, args->game, player->player_id);
    }
    else if (request_type == SEND_GAME_STATE)
    {
      send_game_state(args->server, args->game, player->player_id);
    }
    else if (request_type == FINISH_GAME)
    {
      break;
    }
    else
    {
      perror("Invalid request type");
      exit(1);
    }

    printf("Finished processing request type %d from player %d\n", request_type, player->player_id);
  }

  close(player->sockfd);

  return NULL;
}

void init_server(server_t *server)
{
  server->num_players = 0;

  server->player_list = (player_t *)malloc(MAX_PLAYERS * sizeof(player_t));

  if (server->player_list == NULL)
  {
    perror("player list malloc failed");
    exit(1);
  }

  server->player_threads = (pthread_t *)malloc(MAX_PLAYERS * sizeof(pthread_t));

  if (server->player_threads == NULL)
  {
    perror("player threads malloc failed");
    exit(1);
  }

  for (int i = 0; i < MAX_PLAYERS + PLAYER_PIPES_START; i++)
  {
    pipe(server->pipe_fds[i]);
  }

  if (pthread_mutex_init(&server->mutex, NULL) != 0)
  {
    perror("mutex init failed");
    exit(1);
  }
}

void run_server(server_t *server)
{
  int opt = 1;

  if ((server->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket failed");
    exit(1);
  }

  if ((server->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket failed");
    exit(1);
  }

  if (setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt failed");
    exit(1);
  }

  server->address.sin_family = AF_INET;
  server->address.sin_addr.s_addr = INADDR_ANY;
  server->address.sin_port = htons(PORT);

  if (bind(server->sockfd, (struct sockaddr *)&server->address, sizeof(server->address)) < 0)
  {
    perror("bind failed");
    exit(1);
  }

  if (listen(server->sockfd, 4) < 0)
  {
    perror("listen failed");
    exit(1);
  }

  wait_for_players(server);
}

void wait_for_players(server_t *server)
{
  game_t game;

  while (server->num_players < MAX_PLAYERS)
  {
    int new_socket;
    int addrlen = sizeof(server->address);

    if ((new_socket = accept(server->sockfd, (struct sockaddr *)&server->address, (socklen_t *)&addrlen)) < 0)
    {
      perror("accept failed");
      exit(1);
    }

    server->player_list[server->num_players].sockfd = new_socket;
    server->player_list[server->num_players].player_id = server->num_players;

    player_thread_args_t *args = (player_thread_args_t *)malloc(sizeof(player_thread_args_t));

    if (args == NULL)
    {
      perror("player thread args malloc failed");
      exit(1);
    }

    args->server = server;
    args->player = &server->player_list[server->num_players];
    args->game = &game;

    if (pthread_create(&server->player_threads[server->num_players], NULL, player_thread, (void *)args) != 0)
    {
      perror("pthread_create failed");
      exit(1);
    }

    server->num_players++;
  }

  char name_buffer[MAX_PLAYER_NAME_LENGTH];

  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    int player_id = server->player_list[i].player_id;
    if (read(server->pipe_fds[PLAYER_PIPES_START + player_id][PIPE_READ], name_buffer, MAX_PLAYER_NAME_LENGTH) < 0)
    {
      perror("read failed");
      exit(1);
    }
  }

  int player_ids[MAX_PLAYERS];
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    player_ids[i] = i;
  }
  init_game(&game, player_ids, MAX_PLAYERS);

  int opt = 1;
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (write(server->pipe_fds[START_GAME_PIPE][PIPE_WRITE], &opt, sizeof(opt)) < 0)
    {
      perror("write failed");
      exit(1);
    }
  }
  printf("Sent start signal to player threads\n");

  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    pthread_join(server->player_threads[i], NULL);
  }
}

void send_communication_metadata(server_t *server, int player_id)
{
  int player_sockfd = server->player_list[player_id].sockfd;
  int int_size = sizeof(int);
  send(player_sockfd, &int_size, 1, 0);

  int big_endian = 1;
  int is_little_endian = *(char *)&big_endian == 1;
  send(player_sockfd, &is_little_endian, 1, 0);
}

void send_game_metadata(server_t *server, int player_id)
{
  int player_sockfd = server->player_list[player_id].sockfd;
  request_type_t request = SEND_GAME_METADATA;
  send(player_sockfd, &request, sizeof(request), 0);
  int symbols_per_card = SYMBOLS_PER_CARD;
  send(player_sockfd, &symbols_per_card, sizeof(symbols_per_card), 0);
  send(player_sockfd, &player_id, sizeof(player_id), 0);
  request = END_REQUEST;
  send(player_sockfd, &request, sizeof(request), 0);
}

void send_game_state(server_t *server, game_t* game, int player_id) 
{
  int player_sockfd = server->player_list[player_id].sockfd, temp;
  
  request_type_t request = SEND_GAME_STATE;
  send(player_sockfd, &request, sizeof(request), 0);
  
  temp = SYMBOLS_PER_CARD;
  send(player_sockfd, &temp, sizeof(temp), 0);
  for (int i = 0; i < SYMBOLS_PER_CARD; i++) {
    temp = game->current_top_card[i];
    send(player_sockfd, &temp, sizeof(temp), 0);
  }
  
  send(player_sockfd, &game->players_count, sizeof(game->players_count), 0);
  for(int i = 0; i < game->players_count; i++) {
    player_state_t player = game->player_states[i];
    player_t player_info = server->player_list[i];
    send(player_sockfd, &player.player_id, sizeof(player.player_id), 0);
    send(player_sockfd, &player_info.name, MAX_PLAYER_NAME_LENGTH, 0);
    send(player_sockfd, &player.current_card, SYMBOLS_PER_CARD * sizeof(int), 0);
    send(player_sockfd, &player.cards_in_hand_count, sizeof(player.cards_in_hand_count), 0);
    send(player_sockfd, &player.swaps_left, sizeof(player.swaps_left), 0);
    send(player_sockfd, &player.swaps_cooldown, sizeof(player.swaps_cooldown), 0);
    send(player_sockfd, &player.freezes_left, sizeof(player.freezes_left), 0);
    send(player_sockfd, &player.freezes_cooldown, sizeof(player.freezes_cooldown), 0);
    send(player_sockfd, &player.rerolls_left, sizeof(player.rerolls_left), 0);
    send(player_sockfd, &player.rerolls_cooldown, sizeof(player.rerolls_cooldown), 0);
    send(player_sockfd, &player.is_frozen_count, sizeof(player.is_frozen_count), 0);
  }

  request = END_REQUEST;
  send(player_sockfd, &request, sizeof(request), 0);
}

void send_finish_game(server_t *server)
{
  for (int i = 0; i < server->num_players; i++)
  {
    request_type_t request = FINISH_GAME;
    send(server->player_list[i].sockfd, &request, sizeof(request), 0);
    printf("Sent finish game request to player %d\n", i);
  }
}

void receive_game_action(server_t *server, game_t *game, int player_id)
{
  pthread_mutex_lock(&server->mutex);
  
  if (game->has_finished)
  {
    printf("Game has finished\n");
    pthread_mutex_unlock(&server->mutex);
    return;
  }

  int player_sockfd = server->player_list[player_id].sockfd;
  action_t action;
  recv(player_sockfd, &action.action_type, sizeof(int), 0);
  recv(player_sockfd, &action.id, sizeof(int), 0);
  recv(player_sockfd, &action.board_hash, sizeof(int), 0);

  printf("Received action type %d from player %d\n", action.action_type, player_id);
  return_code_t return_code_value = act_player(game, &action, player_id);
  printf("return code value %d\n", return_code_value);
  printf("Finished processing action type %d from player %d\n", action.action_type, player_id);
  
  request_type_t end_request;
  recv(player_sockfd, &end_request, sizeof(int), 0);
  if (end_request != END_REQUEST)
  {
    perror("Invalid end request");
    exit(1);
  }
  printf("Received end request from player %d\n", player_id);

  request_type_t request = SEND_RETURN_CODE;
  send(player_sockfd, &request, sizeof(request), 0);
  send(player_sockfd, &return_code_value, sizeof(int), 0); 

  for (int i=0; i < server->num_players; i++)
  {
    printf("Sending game state to player %d\n", i);
    send_game_state(server, game, i);
    printf("Sent game state to player %d\n", i);
  }

  if (game->has_finished)
  {
    printf("The game has finished\n");
    send_finish_game(server);
  }

  pthread_mutex_unlock(&server->mutex);
}


void destroy_server(server_t *server)
{
  close(server->sockfd);
  free(server->player_list);
  free(server->player_threads);
}
