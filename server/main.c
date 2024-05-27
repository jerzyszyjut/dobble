#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "game.h"

int main(void)
{
  // server_t server;

  // init_server(&server);
  // run_server(&server);
  // destroy_server(&server);
  
  game_t game;
  int player_ids[] = {1, 2, 3, 4};
  init_game(&game, player_ids, 4);
  destroy_game(&game);

  return 0;
}
