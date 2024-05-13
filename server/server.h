#include "game.h"

typedef struct player_connection {
  int player_id;
  int socket;
} player_connection_t;

typedef struct server {
  int socket;
  player_connection_t *players;
  size_t players_count;
  game_t *game;
} server_t;

typedef struct player_thread_args {
  player_connection_t *player_connection;
  server_t *server;
} player_thread_args_t;

void* player_thread(void *arg);

server_t *initialize_server(int port, size_t players_count);

void start_server(server_t *server);

void destroy_server(server_t *server);

