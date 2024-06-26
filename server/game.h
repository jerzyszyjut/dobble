#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#define SYMBOLS_PER_CARD 8
#define SYMBOLS_COUNT 56
#define DEFAULT_STARTING_CARDS_COUNT 12
#define DEFAULT_SWAPS_COUNT 2
#define SWAPS_COOLDOWN 3
#define DEFAULT_FREEZES_COUNT 2
#define FREEZES_COOLDOWN 3
#define DEFAULT_REROLLS_COUNT 2
#define REROLLS_COOLDOWN 3
#define CHECKSUM_MODULO 21372137

typedef enum return_code {
  SUCCESS,
  SYMBOL_DOES_NOT_MATCH_WITH_TOP_CARD,
  PLAYER_DOES_NOT_HAVE_THIS_SYMBOL,
  ABILITY_NOT_AVAILABLE,
  PLAYER_IS_FROZEN,
  ERROR,
  INCORRECT_BOARD_HASH,
} return_code_t;

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
  int is_frozen_count;
} player_state_t;

typedef struct game
{
  player_state_t *player_states;
  int players_count;
  int current_top_card[SYMBOLS_PER_CARD];
  int has_finished;
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

int calculate_board_hash(game_t *game);

void set_starting_card(game_t *game);

void set_player_card(player_state_t *player_state);

return_code_t act_player(game_t *game, action_t *action, int current_player_id);

void make_post_turn_actions(game_t *game);

return_code_t swap_cards(player_state_t *acting_player_state, player_state_t *target_player_state);

return_code_t checking_guess(game_t *game, action_t *action, int current_player_id);

player_state_t *get_player_state_by_id(game_t *game, int id);

void init_game_player(game_t *game, int index, int id);

void init_game(game_t *game, int *player_ids, int players_count);

void destroy_game(game_t *game);
