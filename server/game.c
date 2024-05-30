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

void set_player_card(game_t *game, player_state_t *player_state)
{
  int flag = 1, random_number, shared_number;
  shared_number = game->current_top_card[rand() % SYMBOLS_PER_CARD];
  
  player_state->current_card[0] = shared_number;

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

void act_player(game_t *game, action_t *action, int current_player_id)
{
  player_state_t *player_state = get_player_state_by_id(game, action->id);

  switch (action->action_type)
  {
  case CARD:
    checking_guess(game, action, current_player_id);
    break;
  case SWAP:
    if (player_state->swaps_left > 0)
    {
      swap_cards(player_state, get_player_state_by_id(game, current_player_id));
      player_state->swaps_left--;
    }
    break;
  case FREEZE:
    if (player_state->freezes_left > 0)
    {
      player_state->is_frozen_count++;
      player_state->freezes_left--;
    }
    break;
  case REROLL:
    if (player_state->rerolls_left > 0)
    {
      set_player_card(game, player_state);
      player_state->rerolls_left--;
    }
    break;
  }
}


void swap_cards(player_state_t *acting_player_state, player_state_t *target_player_state)
{
  int temp_card[SYMBOLS_PER_CARD];

  for (int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    temp_card[i] = acting_player_state->current_card[i];
    acting_player_state->current_card[i] = target_player_state->current_card[i];
    target_player_state->current_card[i] = temp_card[i];
  }
}

void checking_guess(game_t *game, action_t *action, int current_player_id)
{
  player_state_t *player_state = get_player_state_by_id(game, current_player_id);
  for(int i = 0; i < SYMBOLS_PER_CARD; i++)
  {
    if(action->id == game->current_top_card[i])
    {
      player_state->cards_in_hand_count--;
      set_player_card(game, player_state);
    }
  }
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
  player_state->swaps_left = DEFAULT_SWAPS_COUNT;
  player_state->swaps_cooldown = SWAPS_COOLDOWN;
  player_state->freezes_left = DEFAULT_FREEZES_COUNT;
  player_state->freezes_cooldown = FREEZES_COOLDOWN;
  player_state->rerolls_left = DEFAULT_REROLLS_COUNT;
  player_state->rerolls_cooldown = REROLLS_COOLDOWN;
  player_state->is_frozen_count = 0;
}

void init_game(game_t *game, int *player_ids, int players_count)
{
  srand(time(NULL));

  game->players_count = players_count;
  game->player_states = (player_state_t *)malloc(sizeof(player_state_t) * players_count);
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
