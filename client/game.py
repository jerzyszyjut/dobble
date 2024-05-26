from enum import Enum
from typing import List
import socket


SYMBOLS_COUNT = 57
CARD_SYMBOLS_COUNT = 8
MAX_PLAYERS = 1
MAX_NAME_LENGTH = 20
DEFAULT_SWAP_COUNT = 1
DEFAULT_SWAP_COOLDOWN = 3
DEFAULT_REROLL_COUNT = 1
DEFAULT_REROLL_COOLDOWN = 3
DEFAULT_FREEZE_COUNT = 1
DEFAULT_FREEZE_COOLDOWN = 3
DEFAULT_STARTING_CARDS_COUNT = 13

def receive_message(socket: socket.socket) -> str:
  message = ''
  while True:
    message += socket.recv(1).decode()
    if message[-1] == '\n':
      break
  return message

class Symbol(int):
  pass

class Codes(Enum):
  SUCCESS = 0
  PLAYER_NOT_FOUND = 1
  PLAYER_DOES_NOT_HAVE_CARD = 2
  SYMBOL_DOES_NOT_MATCH_CARD_ON_TOP = 3
  NO_CHARGES_LEFT = 4
  COOLDOWN_ACTIVE = 5
  INCORRECT_ACTION = 6
  PLAYER_IS_FROZEN = 7

class Actions(Enum):
  PLAY_CARD = 0
  SWAP = 1
  REROLL = 2
  FREEZE = 3
  
class AbilitiesStatus:
  swaps_left: int
  swap_cooldown: int
  rerolls_left: int
  reroll_cooldown: int
  freezes_left: int
  freeze_cooldown: int
  is_frozen: bool
  
  def __init__(
    self,
    swaps_left: int = DEFAULT_SWAP_COUNT,
    swap_cooldown: int = DEFAULT_SWAP_COOLDOWN,
    rerolls_left: int = DEFAULT_REROLL_COUNT,
    reroll_cooldown: int = DEFAULT_REROLL_COOLDOWN,
    freezes_left: int = DEFAULT_FREEZE_COUNT,
    freeze_cooldown: int = DEFAULT_FREEZE_COOLDOWN,
    is_frozen: bool = False
  ):
    self.swaps_left = swaps_left
    self.swap_cooldown = swap_cooldown
    self.rerolls_left = rerolls_left
    self.reroll_cooldown = reroll_cooldown
    self.freezes_left = freezes_left
    self.freeze_cooldown = freeze_cooldown
    self.is_frozen = is_frozen
  
class Player:
  id: int
  name: str
  
  def __init__(self, id: int, name: str):
    self.id = id
    self.name = name
    
class PlayerState:
  id: int
  card: List[Symbol]
  abilities_status: AbilitiesStatus
  cards_count: int
  
  def __init__(
    self,
    id: int,
    card: List[Symbol],
    abilities_status: AbilitiesStatus,
    cards_count: int
  ):
    self.id = id
    self.card = card
    self.abilities_status = abilities_status
    self.cards_count = cards_count

class Game:
  players: List[Player]
  players_state: List[PlayerState]
  players_count: int
  top_card: List[Symbol]
  winner_id: int
  
  def __init__(self):
    self.players = []
    self.players_state = []
    self.players_count = 0
    self.top_card = []
    self.winner_id = -1
    
  def get_game_state_from_server(self, socket):
    socket.send(b'GET_GAME_STATE')
    players_count_message = receive_message(socket)
    players_count = int(players_count_message.decode())
    for _ in range(players_count):
      player_id = int(socket.recv(4).decode())
      player_name = socket.recv(MAX_NAME_LENGTH)
      player = Player(player_id, player_name)
      self.players.append(player)
      player_card = []
      for _ in range(CARD_SYMBOLS_COUNT):
        player_card.append(Symbol(int(socket.recv(4))))
      abilities_status = AbilitiesStatus(
        swaps_left=int(socket.recv(4)),
        swap_cooldown=int(socket.recv(4)),
        rerolls_left=int(socket.recv(4)),
        reroll_cooldown=int(socket.recv(4)),
        freezes_left=int(socket.recv(4)),
        freeze_cooldown=int(socket.recv(4)),
        is_frozen=bool(socket.recv(4))
      )
      cards_count = int(socket.recv(4))
      player_state = PlayerState(player_id, player_card, abilities_status, cards_count)
      self.players_state.append(player_state)
    top_card = []
    for _ in range(CARD_SYMBOLS_COUNT):
      top_card.append(Symbol(int(socket.recv(4))))
    self.top_card = top_card
    self.winner_id = int(socket.recv(4))
    