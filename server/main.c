#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
  server_t *server = initialize_server(8080, 4);
  
  if (server == NULL)
  {
    printf("Failed to initialize server\n");
    return 1;
  }

  start_server(server);
  print_game_state(server->game);
  destroy_server(server);
  
  while (1)
  {
    sleep(1);
  }
  
  
  return 0;
}
