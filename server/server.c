#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <asm-generic/socket.h>
#include <string.h>

void *player_thread(void *arg) {
  player_thread_args_t *args = (player_thread_args_t*)arg;
  server_t *server = args->server;
  player_t *player = args->player;

  char buffer[128] = {0};
  recv(player->sockfd, buffer, MAX_PLAYER_NAME_LENGTH, 0);  
  strcpy(player->name, buffer);

  if (write(server->pipe_fds[PLAYER_PIPES_START + player->player_id][PIPE_WRITE], player->name, MAX_PLAYER_NAME_LENGTH) < 0) {
    perror("write failed");
    exit(1);
  }

  int opt = 0;
  if (read(server->pipe_fds[START_GAME_PIPE][PIPE_READ], &opt, sizeof(opt)) < 0) {
    perror("read failed");
    exit(1);
  }

  sprintf(buffer, "Welcome to the game, %s!", player->name);
  send(player->sockfd, buffer, strlen(buffer), 0);

  printf("Received game start signal for player %d\n", player->player_id);

  close(player->sockfd);

  return NULL;
}

void init_server(server_t *server) {
  server->num_players = 0;

  server->player_list = (player_t*)malloc(MAX_PLAYERS * sizeof(player_t));

  if (server->player_list == NULL) {
    perror("player list malloc failed");
    exit(1);
  }

  server->player_threads = (pthread_t*)malloc(MAX_PLAYERS * sizeof(pthread_t));
  
  if (server->player_threads == NULL) {
    perror("player threads malloc failed");
    exit(1);
  }
  
  for (int i = 0; i < MAX_PLAYERS + PLAYER_PIPES_START; i++) {
    pipe(server->pipe_fds[i]);
  }
}

void run_server(server_t *server) {
  int opt = 1;

  if ((server->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(1);
  }

  if ((server->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(1);
  }

  if (setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
    exit(1);
  }

  server->address.sin_family = AF_INET;
  server->address.sin_addr.s_addr = INADDR_ANY;
  server->address.sin_port = htons(PORT);

  if (bind(server->sockfd, (struct sockaddr *)&server->address, sizeof(server->address)) < 0) {
    perror("bind failed");
    exit(1);
  }

  if (listen(server->sockfd, 4) < 0) {
    perror("listen failed");
    exit(1);
  }

  wait_for_players(server);
}

void wait_for_players(server_t *server) {
  while (server->num_players < MAX_PLAYERS) {
    int new_socket;
    int addrlen = sizeof(server->address);

    if ((new_socket = accept(server->sockfd, (struct sockaddr *)&server->address, (socklen_t*)&addrlen)) < 0) {
      perror("accept failed");
      exit(1);
    }

    server->player_list[server->num_players].sockfd = new_socket;
    server->player_list[server->num_players].player_id = server->num_players;

    player_thread_args_t *args = (player_thread_args_t*)malloc(sizeof(player_thread_args_t));

    if (args == NULL) {
      perror("player thread args malloc failed");
      exit(1);
    }

    args->server = server;
    args->player = &server->player_list[server->num_players];    

    if (pthread_create(&server->player_threads[server->num_players], NULL, player_thread, (void*)args) != 0) {
      perror("pthread_create failed");
      exit(1);
    }

    server->num_players++;
  }

  char name_buffer[MAX_PLAYER_NAME_LENGTH];

  for (int i = 0; i < MAX_PLAYERS; i++) {
    int player_id = server->player_list[i].player_id;
    if(read(server->pipe_fds[PLAYER_PIPES_START + player_id][PIPE_READ], name_buffer, MAX_PLAYER_NAME_LENGTH) < 0) {
      perror("read failed");
      exit(1);
    }
    printf("Player %d: %s has joined the game\n", i, name_buffer);
  }

  int opt = 1;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (write(server->pipe_fds[START_GAME_PIPE][PIPE_WRITE], &opt, sizeof(opt)) < 0) {
      perror("write failed");
      exit(1);
    }
  }

  for (int i = 0; i < MAX_PLAYERS; i++) {
    pthread_join(server->player_threads[i], NULL);
  }
}

void destroy_server(server_t *server) {
  close(server->sockfd);
  free(server->player_list);
  free(server->player_threads);
}
