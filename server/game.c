#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "game.h"

void set_starting_card(game_t *game)
{
  int flag = 1, random_number;

  for (int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    while (flag)
    {
      flag = 0;
      random_number = rand() % SYMBOLS_COUNT;
      for (int j = 0; j < i; j++)
      {
        if (game->current_top_card[j] == random_number)
        {
          flag = 1;
        }
      }
    }
    flag = 1;
    game->current_top_card[i] = random_number;
  }
}

void set_game_card(game_t *game)
{
  int flag = 1, random_number, number_from_player;

  for (int i = 0; i < game->players_count; i++)
  {
    while (flag)
    {
      flag = 0;
      number_from_player = game->player_states[i].current_card[rand() % SYMBOLS_PER_CARD];
      for (int j = 0; j < i; j++)
      {
        if (game->current_top_card[j] == number_from_player)
        {
          flag = 1;
        }
      }
    }
    flag = 1;
    game->current_top_card[i] = number_from_player;
  }

  for (int i = game->players_count; i < SYMBOLS_PER_CARD; i++)
  {
    while (flag)
    {
      flag = 0;
      random_number = rand() % SYMBOLS_COUNT;
      for (int j = 0; j < i; j++)
      {
        if (game->current_top_card[j] == random_number)
        {
          flag = 1;
        }
      }
    }
    flag = 1;
    game->current_top_card[i] = random_number;
  }
}

void set_player_card(game_t *game, player_state_t *player_state)
{
  int flag = 1, random_number;
  
  player_state->current_card[0] = game->current_top_card[rand() % SYMBOLS_PER_CARD];

  for (int i = 1; i < SYMBOLS_PER_CARD; i++)
  {
    while (flag)
    {
      flag = 0;
      random_number = rand() % SYMBOLS_COUNT;
      for (int j = 0; j < SYMBOLS_PER_CARD; j++)
      {
        if (game->current_top_card[j] == random_number)
        {
          flag = 1;
        }
      }
      for (int j = 0; j < i; j++)
      {
        if (player_state->current_card[j] == random_number)
        {
          flag = 1;
        }
      }
    }
    flag = 1;
    player_state->current_card[i] = random_number;
  }
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
      set_player_card(game, player_state);
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
  for(int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    if(action->id == game->current_top_card[i])
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

  set_player_card(game, player_state);

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
  set_player_card(game, player_state);
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

  game->players_count = players_count;
  game->player_states = (player_state_t *)malloc(sizeof(player_state_t) * players_count);
  game->has_finished = 0;
  set_starting_card(game);

  for (int i = 0; i < players_count; i++)
  {
    init_game_player(game, i, player_ids[i]);
  }
}

void destroy_game(game_t *game)
{
  free(game->player_states);
}
