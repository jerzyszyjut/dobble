#include <netinet/in.h>
#include <unistd.h>
#include <asm-generic/socket.h>
#include <pthread.h>

#define MAX_PLAYER_NAME_LENGTH 32
#define MAX_PLAYERS 4
#define PORT 8080
#define START_GAME_PIPE 0
#define PLAYER_PIPES_START 1
#define PIPE_WRITE 1
#define PIPE_READ 0

typedef struct {
  int player_id;
  char name[MAX_PLAYER_NAME_LENGTH];
  int sockfd;
} player_t;

typedef struct {
  player_t* player_list;
  int num_players;
  int sockfd;
  struct sockaddr_in address;
  pthread_t* player_threads;
  int pipe_fds[PLAYER_PIPES_START + MAX_PLAYERS][2];
} server_t;

typedef struct {
  server_t* server;
  player_t* player;
} player_thread_args_t;

void *player_thread(void *arg);

void init_server(server_t *server);

void run_server(server_t *server);

void wait_for_players(server_t *server);

void destroy_server(server_t *server);
