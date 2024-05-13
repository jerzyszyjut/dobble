#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

void* player_thread(void *arg)
{
  player_thread_args_t *args = (player_thread_args_t *)arg;
  printf("Player %d connected\n", args->player_connection->player_id);

  char name[20] = {0};
  player_connection_t *player_connection = args->player_connection;
  recv(player_connection->socket, name, 20, 0);
  
  printf("Player %d name: %s\n", player_connection->player_id, name);

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

void start_server(server_t *server)
{
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  int new_socket;
  int connections_count = 0;
  
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

    pthread_t thread;
    pthread_create(&thread, NULL, player_thread, args);
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
}

void destroy_server(server_t *server)
{
  close(server->socket);
  destroy_game(server->game);
  free(server->players);
  free(server);
}
