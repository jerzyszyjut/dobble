#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define SYMBOLS_PER_CARD 8
#define SYMBOLS_COUNT 57
#define DEFAULT_STARTING_CARDS_COUNT 13;
#define DEFAULT_SWAPS_COUNT 1
#define SWAPS_COOLDOWN 3
#define DEFAULT_FREEZES_COUNT 1
#define FREEZES_COOLDOWN 3
#define DEFAULT_REROLLS_COUNT 1
#define REROLLS_COOLDOWN 3

typedef struct player_state
{
  int player_id;
  int current_card[SYMBOLS_PER_CARD];
  int cards_in_hand_count;
  int swaps_left;
  int swaps_cooldown;
  int freezes_left;
  int freezes_cooldown;
  int rerolls_left;
  int rerolls_cooldown;
  int is_frozen;
} player_state_t;

typedef struct game
{
  player_state_t *player_states;
  int players_count;
  int current_top_card[SYMBOLS_PER_CARD];
} game_t;

typedef enum actions {
  CARD,
  SWAP,
  FREEZE,
  REROLL
} actions_type_t;

typedef struct action {
  actions_type_t action_type;
  int id;
  int board_hash;
} action_t;

void set_starting_card(game_t *game);

void set_player_card(game_t *game, player_state_t *player_state);

player_state_t *get_player_state_by_id(game_t *game, int id);

void init_game_player(game_t *game, int index, int id);

void init_game(game_t *game, int *player_ids, int players_count);

void destroy_game(game_t *game);
