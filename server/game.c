#include "game.h"
#include <stdlib.h>
#include <stdio.h>

symbol_t *get_random_card()
{
  symbol_t *card = malloc(sizeof(symbol_t) * CARD_SYMBOLS_COUNT);
  
  if (card == NULL)
  {
    printf("Failed to allocate memory for card\n");
    return NULL;
  }

  for (int i = 0; i < CARD_SYMBOLS_COUNT; i++)
  {
    int symbol = rand() % SYMBOLS_COUNT;
    card[i] = symbol;

    for (int j = 0; j < i; j++)
    {
      if (card[j] == symbol)
      {
        i--;
        break;
      }
    }
  }

  return card;
}

symbol_t *get_random_card_for_player(game_t *game)
{
  symbol_t *card = malloc(sizeof(symbol_t) * CARD_SYMBOLS_COUNT);
  
  if (card == NULL)
  {
    printf("Failed to allocate memory for card\n");
    return NULL;
  }

  symbol_t common_symbol = game->top_card[rand() % CARD_SYMBOLS_COUNT];
  
  card[0] = common_symbol;

  for (int i = 1; i < CARD_SYMBOLS_COUNT; i++)
  {
    int flag = 0;
    int symbol = rand() % SYMBOLS_COUNT;
    card[i] = symbol;

    for (int j = 0; j < i; j++)
    {
      if (card[j] == symbol)
      {
        i--;
        flag = 1;
        break;
      }
    }

    if (!flag)
    {
      for (int j = 0; j < CARD_SYMBOLS_COUNT; j++)
      {
        if (game->top_card[j] == symbol)
        {
          i--;
          break;
        }
      }
    }
  }

  return card;
}

player_state_t *get_player_state(game_t *game, int player_id)
{
  for (size_t i = 0; i < game->players_count; i++)
  {
    if (game->player_states[i]->id == player_id)
    {
      return game->player_states[i];
    }
  }

  return NULL;
}

game_t *initialize_game(player_t *players, size_t players_count)
{
  game_t *game = malloc(sizeof(game_t));
  
  if (game == NULL)
  {
    printf("Failed to allocate memory for game\n");
    return NULL;
  }

  game->players = players;
  game->player_states = malloc(sizeof(player_state_t *) * players_count);
  game->players_count = players_count;
  game->top_card = get_random_card();

  for (size_t i = 0; i < players_count; i++)
  {
    player_state_t *player_state = malloc(sizeof(player_state_t));
    
    if (player_state == NULL)
    {
      printf("Failed to allocate memory for player state\n");
      return NULL;
    }

    player_state->id = players[i].id;
    player_state->abilities_status.swaps_left = DEFAULT_SWAP_COUNT;
    player_state->abilities_status.swap_cooldown = DEFAULT_SWAP_COOLDOWN;
    player_state->abilities_status.rerolls_left = DEFAULT_REROLL_COUNT;
    player_state->abilities_status.reroll_cooldown = DEFAULT_REROLL_COOLDOWN;
    player_state->abilities_status.freezes_left = DEFAULT_FREEZE_COUNT;
    player_state->abilities_status.freeze_cooldown = DEFAULT_FREEZE_COOLDOWN;
    player_state->cards_count = DEFAULT_STARTING_CARDS_COUNT;
    player_state->card = get_random_card_for_player(game);

    game->player_states[i] = player_state;
  }

  return game;
}

CODES process_action(game_t *game, action_t *action)
{
  if (action->type == PLAY_CARD)
  {
    player_state_t *player_state = get_player_state(game, action->player_id);

    if (player_state == NULL)
    {
      printf("Player not found\n");
      return PLAYER_NOT_FOUND;
    }

    symbol_t *symbol = (symbol_t *)action->data;

    int flag = 0;
    for(int i=0; i<CARD_SYMBOLS_COUNT; i++)
    {
      if (player_state->card[i] == *symbol)
      {
        flag = 1;
        break;
      }
    }

    if (!flag)
    {
      printf("Player does not have the card\n");
      return PLAYER_DOES_NOT_HAVE_CARD;
    }

    flag = 0;

    for (int i = 0; i < CARD_SYMBOLS_COUNT; i++)
    {
      if (game->top_card[i] == *symbol)
      {
        flag = 1;
        break;
      }
    }

    if (!flag)
    {
      printf("Card not in top card\n");
      return SYMBOL_DOES_NOT_MATCH_CARD_ON_TOP;
    }

    free(game->top_card);
    game->top_card = player_state->card;
    player_state->card = get_random_card_for_player(game);
    player_state->cards_count--;

    if (player_state->cards_count == 0)
    {
      game->winner_id = player_state->id;
    }

    return SUCCESS;
  }
  else if (action->type == SWAP)
  {
    player_state_t *player_state = get_player_state(game, action->player_id);

    if (player_state == NULL)
    {
      printf("Player not found\n");
      return PLAYER_NOT_FOUND;
    }

    if (player_state->abilities_status.swaps_left == 0)
    {
      printf("No swaps left\n");
      return NO_CHARGES_LEFT;
    }

    if (player_state->abilities_status.swap_cooldown > 0)
    {
      printf("Swap on cooldown\n");
      return COOLDOWN_ACTIVE;
    }

    int* target_player_id = (int*)action->data;
    player_state_t *target_player_state = get_player_state(game, *target_player_id);

    if (target_player_state == NULL)
    {
      printf("Target player not found\n");
      return PLAYER_NOT_FOUND;
    }

    symbol_t *temp = player_state->card;
    player_state->card = target_player_state->card;
    target_player_state->card = temp;

    player_state->abilities_status.swaps_left--;
    player_state->abilities_status.swap_cooldown = DEFAULT_SWAP_COOLDOWN;

    return SUCCESS;
  }
  else if (action->type == REROLL)
  {
    player_state_t *player_state = get_player_state(game, action->player_id);

    if (player_state == NULL)
    {
      printf("Player not found\n");
      return PLAYER_NOT_FOUND;
    }

    if (player_state->abilities_status.rerolls_left == 0)
    {
      printf("No rerolls left\n");
      return NO_CHARGES_LEFT;
    }

    if (player_state->abilities_status.reroll_cooldown > 0)
    {
      printf("Reroll on cooldown\n");
      return COOLDOWN_ACTIVE;
    }

    free(player_state->card);
    player_state->card = get_random_card_for_player(game);

    player_state->abilities_status.rerolls_left--;
    player_state->abilities_status.reroll_cooldown = DEFAULT_REROLL_COOLDOWN;

    return SUCCESS;
  }
  else if (action->type == FREEZE)
  {
    player_state_t *player_state = get_player_state(game, action->player_id);

    if (player_state == NULL)
    {
      printf("Player not found\n");
      return PLAYER_NOT_FOUND;
    }

    if (player_state->abilities_status.freezes_left == 0)
    {
      printf("No freezes left\n");
      return NO_CHARGES_LEFT;
    }

    if (player_state->abilities_status.freeze_cooldown > 0)
    {
      printf("Freeze on cooldown\n");
      return COOLDOWN_ACTIVE;
    }

    int *target_player_id = (int *)action->data;
    player_state_t *target_player_state = get_player_state(game, *target_player_id);

    if (target_player_state == NULL)
    {
      printf("Target player not found\n");
      return PLAYER_NOT_FOUND;
    }

    target_player_state->abilities_status.is_frozen = 1;
    player_state->abilities_status.freezes_left--;

    return SUCCESS;
  }
  return INCORRECT_ACTION;
}

CODES make_action(game_t *game, action_t *action)
{
  int player_id = action->player_id;
  int is_frozen = get_player_state(game, player_id)->abilities_status.is_frozen;

  if (is_frozen)
  {
    return PLAYER_IS_FROZEN;
  }

  CODES code = process_action(game, action);

  if (code == SUCCESS)
  {
    if (action->type == PLAY_CARD)
    {
      for (size_t i = 0; i < game->players_count; i++)
      {
        if (game->player_states[i]->abilities_status.is_frozen)
        {
          game->player_states[i]->abilities_status.is_frozen = 0;
        }
        else
        {
          game->player_states[i]->abilities_status.freeze_cooldown--;
        }

        game->player_states[i]->abilities_status.swap_cooldown--;
        game->player_states[i]->abilities_status.reroll_cooldown--;
      }
    }
  }

  return code;
}

void destroy_game(game_t *game)
{
  for (size_t i = 0; i < game->players_count; i++)
  {
    free(game->player_states[i]->card);
    free(game->player_states[i]);
  }

  free(game->top_card);
  free(game);
}

void print_game_state(game_t *game)
{
  printf("Players count: %ld\n", game->players_count);
  printf("Winner id: %d\n", game->winner_id);
  printf("Top card: ");
  for (int i = 0; i < CARD_SYMBOLS_COUNT; i++)
  {
    printf("%d ", game->top_card[i]);
  }
  printf("\n");

  for (size_t i = 0; i < game->players_count; i++)
  {
    printf("Player %d: ", game->players[i].id);
    printf("Name: %s\n", game->players[i].name);
    printf("Cards count: %ld\n", game->player_states[i]->cards_count);
    printf("Card symbols: ");
    for (int j = 0; j < CARD_SYMBOLS_COUNT; j++)
    {
      printf("%d ", game->player_states[i]->card[j]);
    }
    printf("\n");

    printf("Swaps left: %d\n", game->player_states[i]->abilities_status.swaps_left);
    printf("Swap cooldown: %d\n", game->player_states[i]->abilities_status.swap_cooldown);
    printf("Rerolls left: %d\n", game->player_states[i]->abilities_status.rerolls_left);
    printf("Reroll cooldown: %d\n", game->player_states[i]->abilities_status.reroll_cooldown);
    printf("Freezes left: %d\n", game->player_states[i]->abilities_status.freezes_left);
    printf("Freeze cooldown: %d\n", game->player_states[i]->abilities_status.freeze_cooldown);
    printf("Is frozen: %d\n", game->player_states[i]->abilities_status.is_frozen);
  }
}
