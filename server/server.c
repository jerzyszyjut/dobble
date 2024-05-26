#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

char endline = '\n';

void* player_thread(void *arg)
{
  player_thread_args_t *args = (player_thread_args_t *)arg;
  printf("Player %d connected\n", args->player_connection->player_id);

  int val;
  read(*args->pipefd[0], &val, sizeof(val));

  char buffer[1024] = {0};
  player_connection_t *player_connection = args->player_connection;
  server_t *server = args->server;
  player_t *player = &server->game->players[player_connection->player_id];
  printf("Player %d name: %s\n", player_connection->player_id, player->name);

  while (server->game->winner_id == -1)
  {
    recv(player_connection->socket, buffer, 1024, 0);
    if (strcmp(buffer, "GET_GAME_STATE"))
    {
      send_game_state(server, player_connection);
    }
  }

  close(player_connection->socket);
  return NULL;
}

server_t *initialize_server(int port, size_t players_count) 
{
  server_t *server = malloc(sizeof(server_t));
  
  if (server == NULL)
  {
    printf("Failed to allocate memory for server\n");
    return NULL;
  }

  server->players = malloc(sizeof(player_connection_t) * players_count);
  
  if (server->players == NULL)
  {
    printf("Failed to allocate memory for players\n");
    free(server);
    return NULL;
  }

  server->players_count = players_count;
  server->game = NULL;
  
  server->socket = socket(AF_INET, SOCK_STREAM, 0);
  
  if (server->socket == -1)
  {
    printf("Failed to create socket\n");
    destroy_server(server);
    return NULL;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  
  if (bind(server->socket, (struct sockaddr *)&address, sizeof(address)) == -1)
  {
    printf("Failed to bind socket\n");
    destroy_server(server);
    return NULL;
  }

  if (listen(server->socket, 3) == -1)
  {
    printf("Failed to listen on socket\n");
    destroy_server(server);
    return NULL;
  }

  return server;
}

void send_game_state(server_t *server, player_connection_t *player_connection)
{
  player_state_t *player_state = get_player_state(server->game, player_connection->player_id);
  send(player_connection->socket, &server->game->players_count, 4, 0);
  send(player_connection->socket, &endline, 1, 0);
  for(size_t player_index=0; player_index < server->game->players_count; player_index++)
  {
    player_t *player = &server->game->players[player_index];
    send(player_connection->socket, &player->id, 4, 0);
    send(player_connection->socket, &endline, 1, 0);
    send(player_connection->socket, player->name, MAX_NAME_LENGTH, 0);
    send(player_connection->socket, &endline, 1, 0);
    for (int card_symbol_index = 0; card_symbol_index < CARD_SYMBOLS_COUNT; card_symbol_index++)
    {
      send(player_connection->socket, &player_state->card[card_symbol_index], 4, 0);
      send(player_connection->socket, &endline, 1, 0);
    }
    send(player_connection->socket, &player_state->abilities_status.swaps_left, 4, 0);
    send(player_connection->socket, &endline, 1, 0);
    send(player_connection->socket, &player_state->abilities_status.swap_cooldown, 4, 0);
    send(player_connection->socket, &endline, 1, 0);
    send(player_connection->socket, &player_state->abilities_status.rerolls_left, 4, 0);
    send(player_connection->socket, &endline, 1, 0);
    send(player_connection->socket, &player_state->abilities_status.reroll_cooldown, 4, 0);
    send(player_connection->socket, &endline, 1, 0);
    send(player_connection->socket, &player_state->abilities_status.freezes_left, 4, 0);
    send(player_connection->socket, &endline, 1, 0);
    send(player_connection->socket, &player_state->abilities_status.freeze_cooldown, 4, 0);
    send(player_connection->socket, &endline, 1, 0);
    send(player_connection->socket, &player_state->abilities_status.is_frozen, 4, 0);
    send(player_connection->socket, &endline, 1, 0);

    send(player_connection->socket, &player_state->cards_count, 4, 0);
    send(player_connection->socket, &endline, 1, 0);
  }
  for (int card_symbol_index = 0; card_symbol_index < CARD_SYMBOLS_COUNT; card_symbol_index++)
  {
    send(player_connection->socket, &server->game->top_card[card_symbol_index], 4, 0);
    send(player_connection->socket, &endline, 1, 0);
  }
  send(player_connection->socket, &server->game->winner_id, 4, 0);
    send(player_connection->socket, &endline, 1, 0);
}

void start_server(server_t *server)
{
  pthread_t* client_threads = malloc(sizeof(pthread_t) * server->players_count);
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  int new_socket;
  int connections_count = 0;
  int pipefd[2];
  pipe(pipefd);

  while (connections_count < MAX_PLAYERS)
  {
    new_socket = accept(server->socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    
    if (new_socket == -1)
    {
      printf("Failed to accept connection\n");
      continue;
    }

    player_thread_args_t *args = malloc(sizeof(player_thread_args_t));
    args->player_connection = &server->players[connections_count];
    args->player_connection->player_id = connections_count;
    args->player_connection->socket = new_socket;
    args->server = server;
    args->name = malloc(sizeof(char) * MAX_NAME_LENGTH);
    args->pipefd[0] = &pipefd[0];
    args->pipefd[1] = &pipefd[1];
    recv(new_socket, args->name, MAX_NAME_LENGTH, 0);

    pthread_create(&client_threads[connections_count], NULL, player_thread, args);
    connections_count++;
  }

  printf("All players connected\n");
  player_t *players = malloc(sizeof(player_t) * connections_count);
  if (players == NULL)
  {
    printf("Failed to allocate memory for players\n");
    destroy_server(server);
    return;
  }

  for (int i = 0; i < connections_count; i++)
  {
    players[i].id = server->players[i].player_id;
    players[i].name = malloc(sizeof(char) * MAX_NAME_LENGTH);
  }

  server->game = initialize_game(players, connections_count);

  if (server->game == NULL)
  {
    printf("Failed to initialize game\n");
    destroy_server(server);
    return;
  }

  printf("Game initialized\n");

  int val = 1;
  for (int i = 0; i < connections_count; i++)
  {
    write(pipefd[1], &val, sizeof(val));
  }

  for (int i = 0; i < connections_count; i++)
  {
    pthread_join(client_threads[i], NULL);
  }
}

void destroy_server(server_t *server)
{
  close(server->socket);
  if (server->game != NULL)
  {
    destroy_game(server->game);
  }
  free(server->players);
  free(server);
}
