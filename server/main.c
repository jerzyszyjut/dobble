#include "game.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
  player_t *players = malloc(sizeof(player_t) * 4);
  
  for (int i = 0; i < 4; i++)
  {
    players[i].name = malloc(sizeof(char) * MAX_NAME_LENGTH);
    players[i].id = i;
    sprintf(players[i].name, "Player %d", i);
  }

  game_t *game = initialize_game(players, 4);

  print_game_state(game);
}
