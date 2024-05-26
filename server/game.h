#include <stdlib.h>

#define SYMBOLS_COUNT 57
#define CARD_SYMBOLS_COUNT 8
#define MAX_PLAYERS 1
#define MAX_NAME_LENGTH 20
#define DEFAULT_SWAP_COUNT 1
#define DEFAULT_SWAP_COOLDOWN 3
#define DEFAULT_REROLL_COUNT 1
#define DEFAULT_REROLL_COOLDOWN 3
#define DEFAULT_FREEZE_COUNT 1
#define DEFAULT_FREEZE_COOLDOWN 3
#define DEFAULT_STARTING_CARDS_COUNT 13

typedef int symbol_t;

typedef enum CODES
{
  SUCCESS,
  PLAYER_NOT_FOUND,
  PLAYER_DOES_NOT_HAVE_CARD,
  SYMBOL_DOES_NOT_MATCH_CARD_ON_TOP,
  NO_CHARGES_LEFT,
  COOLDOWN_ACTIVE,
  INCORRECT_ACTION,
  PLAYER_IS_FROZEN,
} CODES;

enum ACTION_TYPE
{
  PLAY_CARD,
  SWAP,
  REROLL,
  FREEZE
};

typedef struct action
{
  int player_id;
  enum ACTION_TYPE type;
  void *data;
} action_t;

typedef struct abilities_status
{
  int swaps_left;
  int swap_cooldown;
  int rerolls_left;
  int reroll_cooldown;
  int freezes_left;
  int freeze_cooldown;
  int is_frozen;
} abilities_status_t;

typedef struct player
{
  int id;
  char *name;
} player_t;

typedef struct player_state
{
  int id;
  symbol_t* card;
  abilities_status_t abilities_status;
  size_t cards_count;
} player_state_t;

typedef struct game
{
  player_t *players;
  player_state_t **player_states;
  size_t players_count;
  symbol_t* top_card;
  int winner_id;
} game_t;

symbol_t *get_random_card();

symbol_t *get_random_card_for_player(game_t *game);

player_state_t *get_player_state(game_t *game, int player_id);

game_t *initialize_game(player_t *players, size_t players_count);

CODES process_action(game_t *game, action_t *action);

CODES make_action(game_t *game, action_t *action);

void destroy_game(game_t *game);

void print_game_state(game_t *game);
