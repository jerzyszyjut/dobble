#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "game.h"

const int CARDS[SYMBOLS_COUNT][SYMBOLS_PER_CARD] = {
    {8, 34, 49, 38, 53, 5, 19, 23},
    {23, 0, 27, 24, 26, 22, 25, 28},
    {9, 12, 11, 0, 13, 8, 10, 14},
    {4, 30, 53, 27, 14, 17, 40, 43},
    {33, 25, 17, 41, 2, 9, 49, 50},
    {46, 44, 0, 43, 47, 45, 48, 49},
    {45, 4, 32, 42, 55, 9, 19, 22},
    {36, 26, 53, 48, 6, 21, 31, 9},
    {30, 11, 15, 56, 5, 26, 41, 45},
    {32, 14, 38, 20, 26, 7, 50, 44},
    {30, 0, 34, 31, 35, 33, 32, 29},
    {21, 38, 54, 13, 30, 46, 22, 2},
    {55, 23, 39, 2, 47, 14, 31, 15},
    {34, 51, 10, 43, 18, 42, 26, 2},
    {52, 16, 4, 13, 49, 26, 29, 39},
    {3, 12, 32, 43, 21, 41, 52, 23},
    {1, 16, 51, 9, 23, 37, 44, 30},
    {0, 53, 50, 52, 56, 54, 55, 51},
    {39, 8, 33, 7, 21, 27, 51, 45},
    {39, 25, 53, 11, 32, 18, 46, 1},
    {53, 10, 16, 22, 41, 47, 35, 7},
    {52, 34, 15, 28, 7, 9, 40, 46},
    {3, 48, 39, 30, 28, 50, 19, 10},
    {37, 19, 25, 56, 13, 31, 7, 43},
    {6, 51, 14, 24, 19, 29, 41, 46},
    {35, 48, 38, 51, 4, 25, 12, 15},
    {52, 35, 36, 19, 2, 27, 11, 44},
    {55, 17, 35, 26, 8, 46, 37, 3},
    {16, 45, 14, 25, 36, 34, 3, 54},
    {18, 29, 9, 56, 3, 38, 47, 27},
    {40, 18, 35, 45, 13, 50, 6, 23},
    {6, 11, 43, 33, 16, 38, 28, 55},
    {13, 41, 20, 55, 34, 1, 27, 48},
    {18, 31, 4, 44, 41, 54, 28, 8},
    {42, 23, 54, 17, 29, 11, 7, 48},
    {3, 33, 44, 42, 15, 24, 53, 13},
    {3, 4, 5, 6, 0, 7, 1, 2},
    {3, 11, 22, 31, 40, 51, 20, 49},
    {35, 20, 43, 24, 54, 39, 5, 9},
    {27, 10, 37, 54, 15, 32, 6, 49},
    {34, 6, 17, 22, 12, 44, 39, 56},
    {18, 12, 7, 49, 30, 55, 24, 36},
    {32, 56, 48, 40, 24, 8, 16, 2},
    {22, 14, 5, 48, 52, 37, 33, 18},
    {5, 31, 42, 46, 27, 12, 50, 16},
    {40, 54, 12, 26, 33, 1, 19, 47},
    {11, 24, 47, 50, 34, 4, 21, 37},
    {36, 56, 20, 23, 10, 4, 33, 46},
    {40, 36, 41, 42, 0, 38, 39, 37},
    {20, 52, 6, 25, 47, 8, 42, 30},
    {40, 55, 5, 29, 10, 21, 44, 25},
    {37, 2, 20, 53, 45, 28, 12, 29},
    {10, 17, 45, 38, 52, 1, 31, 24},
    {19, 18, 21, 0, 16, 15, 20, 17},
    {42, 28, 56, 49, 1, 14, 35, 21},
    {50, 15, 22, 1, 29, 36, 43, 8}};

int used_cards_starting_index, used_cards_count;

int calculate_board_hash(game_t *game)
{
  int check_sum = 0;
  for (int i=0; i<SYMBOLS_PER_CARD; i++)
  {
    check_sum += game->current_top_card[i] * (i+1);
    check_sum %= CHECKSUM_MODULO;
  }
  for (int i=0; i<game->players_count; i++)
  {
    for (int j=0; j<SYMBOLS_PER_CARD; j++)
    {
      check_sum += game->player_states[i].current_card[j] * (i+1) * (j+1);
      check_sum %= CHECKSUM_MODULO;
    }
  }
  for (int i=0; i<game->players_count; i++)
  {
    check_sum += (game->player_states[i].swaps_left * (i+1) * 100) % CHECKSUM_MODULO;
    check_sum += (game->player_states[i].swaps_cooldown * (i+1) * 1000) % CHECKSUM_MODULO;
    check_sum += (game->player_states[i].freezes_left * (i+1) * 10000) % CHECKSUM_MODULO;
    check_sum += (game->player_states[i].freezes_cooldown * (i+1) * 100000) % CHECKSUM_MODULO;
    check_sum += (game->player_states[i].rerolls_left * (i+1) * 1000000) % CHECKSUM_MODULO;
    check_sum += (game->player_states[i].rerolls_cooldown * (i+1) * 10000000) % CHECKSUM_MODULO;
  }
  return check_sum;
}

void set_game_card(game_t *game)
{
  for (int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    game->current_top_card[i] = CARDS[(used_cards_starting_index + used_cards_count) % SYMBOLS_COUNT][i];
  }
  used_cards_count++;
}

void set_player_card(player_state_t *player_state)
{
  for (int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    player_state->current_card[i] = CARDS[(used_cards_starting_index + used_cards_count) % SYMBOLS_COUNT][i];
  }
  used_cards_count++;
}

return_code_t act_player(game_t *game, action_t *action, int current_player_id)
{
  player_state_t *player_state = get_player_state_by_id(game, current_player_id);
  return_code_t return_code;

  if (player_state->is_frozen_count > 0)
  {
    return PLAYER_IS_FROZEN;
  }

  switch (action->action_type)
  {
  case CARD:
    return_code = checking_guess(game, action, current_player_id);
    make_post_turn_actions(game);
    break;
  case SWAP:
    if (player_state->swaps_left > 0 && player_state->swaps_cooldown == 0)
    {
      player_state_t *target_player_state = get_player_state_by_id(game, action->id);
      return_code = swap_cards(player_state, target_player_state);
      player_state->swaps_left--;
      return_code = SUCCESS;
    }
    else
    {
      return_code = ABILITY_NOT_AVAILABLE;
    }
    break;
  case FREEZE:
    if (player_state->freezes_left > 0 && player_state->freezes_cooldown == 0)
    {
      player_state_t *target_player_state = get_player_state_by_id(game, action->id);
      target_player_state->is_frozen_count++;
      player_state->freezes_left--;
      return_code = SUCCESS;
    }
    else
    {
      return_code = ABILITY_NOT_AVAILABLE;
    }
    break;
  case REROLL:
    if (player_state->rerolls_left > 0 && player_state->rerolls_cooldown == 0)
    {
      set_player_card(player_state);
      player_state->rerolls_left--;
      return_code = SUCCESS;
    }
    else
    {
      return_code = ABILITY_NOT_AVAILABLE;
    }
    break;
  }

  return return_code;
}

void make_post_turn_actions(game_t *game)
{
  for (int i = 0; i < game->players_count; i++)
  {
    if (game->player_states[i].swaps_cooldown > 0)
    {
      game->player_states[i].swaps_cooldown--;
    }
    if (game->player_states[i].freezes_cooldown > 0)
    {
      game->player_states[i].freezes_cooldown--;
    }
    if (game->player_states[i].rerolls_cooldown > 0)
    {
      game->player_states[i].rerolls_cooldown--;
    }
    if (game->player_states[i].is_frozen_count > 0)
    {
      game->player_states[i].is_frozen_count--;
    }

    if (game->player_states[i].cards_in_hand_count <= 0)
    {
      game->has_finished = 1;
    }
  }
}

return_code_t swap_cards(player_state_t *acting_player_state, player_state_t *target_player_state)
{
  int temp_card[SYMBOLS_PER_CARD];

  for (int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    temp_card[i] = acting_player_state->current_card[i];
    acting_player_state->current_card[i] = target_player_state->current_card[i];
    target_player_state->current_card[i] = temp_card[i];
  }

  return SUCCESS;
}

return_code_t checking_guess(game_t *game, action_t *action, int current_player_id)
{
  player_state_t *player_state = get_player_state_by_id(game, current_player_id);
  for (int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    if (action->id == player_state->current_card[i])
    {
      break;
    }
    else if (i == SYMBOLS_PER_CARD - 1)
    {
      return PLAYER_DOES_NOT_HAVE_THIS_SYMBOL;
    }
  }
  for (int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    if (action->id == game->current_top_card[i])
    {
      break;
    }
    else if (i == SYMBOLS_PER_CARD - 1)
    {
      return SYMBOL_DOES_NOT_MATCH_WITH_TOP_CARD;
    }
  }
  player_state->cards_in_hand_count = player_state->cards_in_hand_count - 1;

  for (int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    game->current_top_card[i] = player_state->current_card[i];
  }

  set_player_card(player_state);

  return SUCCESS;
}

player_state_t *get_player_state_by_id(game_t *game, int id)
{
  for (int i = 0; i < game->players_count; i++)
  {
    if (game->player_states[i].player_id == id)
    {
      return &game->player_states[i];
    }
  }

  fprintf(stderr, "Tried to access player with id %d who does not exist", id);

  return NULL;
}

void init_game_player(game_t *game, int index, int id)
{
  player_state_t *player_state = &game->player_states[index];

  player_state->player_id = id;
  set_player_card(player_state);
  player_state->cards_in_hand_count = DEFAULT_STARTING_CARDS_COUNT;
  player_state->swaps_left = DEFAULT_SWAPS_COUNT;
  player_state->swaps_cooldown = 0;
  player_state->freezes_left = DEFAULT_FREEZES_COUNT;
  player_state->freezes_cooldown = 0;
  player_state->rerolls_left = DEFAULT_REROLLS_COUNT;
  player_state->rerolls_cooldown = 0;
  player_state->is_frozen_count = 0;
}

void init_game(game_t *game, int *player_ids, int players_count)
{
  srand(time(NULL));

  used_cards_count = 0;
  used_cards_starting_index = rand() % SYMBOLS_COUNT;

  game->players_count = players_count;
  game->player_states = (player_state_t *)malloc(sizeof(player_state_t) * players_count);
  game->has_finished = 0;
  set_game_card(game);

  for (int i = 0; i < players_count; i++)
  {
    init_game_player(game, i, player_ids[i]);
  }
}

void destroy_game(game_t *game)
{
  free(game->player_states);
}
