from enum import Enum
import socket
from typing import List

ADDRESS = "127.0.0.1"
PORT = 8080
MAX_PLAYER_NAME_LENGTH = 32
SYMBOLS_PER_CARD = None
INT_SIZE = 4
BYTE_ORDER = 'little'

class RequestType(Enum):
  SEND_GAME_STATE = 0
  END_REQUEST = 1

class PlayerState:
  player_id: int
  current_card: List[int]
  cards_in_hand_count: int
  swaps_left: int
  swaps_cooldown: int
  freezes_left: int
  freezes_cooldown: int
  rerolls_left: int
  rerolls_cooldown: int
  is_frozen: bool
  
  def __init__(self, player_id: int, current_card: List[int], cards_in_hand_count: int, swaps_left: int, swaps_cooldown: int, freezes_left: int, freezes_cooldown: int, rerolls_left: int, rerolls_cooldown: int, is_frozen: bool):
    self.player_id = player_id
    self.current_card = current_card
    self.cards_in_hand_count = cards_in_hand_count
    self.swaps_left = swaps_left
    self.swaps_cooldown = swaps_cooldown
    self.freezes_left = freezes_left
    self.freezes_cooldown = freezes_cooldown
    self.rerolls_left = rerolls_left
    self.rerolls_cooldown = rerolls_cooldown
    self.is_frozen = is_frozen
  

class Game:
  player_states: List[PlayerState]
  players_count: int
  current_top_card: int  

  def __init__(self, player_states: List[PlayerState], players_count: int, current_top_card: int):
    self.player_states = player_states
    self.players_count = players_count
    self.current_top_card = current_top_card

class Client:
  socket_client: socket.socket
  game: Game
  
  def __init__(self):
    pass

  def run(self):
    self.socket_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket_client.connect((ADDRESS, PORT))
    
    print("Connected to server")
    
    username = None
    while username is None or len(username) > MAX_PLAYER_NAME_LENGTH - 1:
      username = input("Enter your username: ")  
      
    self.socket_client.send(username.encode())
    print("Sent username to server")
   
    request_type = self.socket_client.recv(INT_SIZE)
    message = int.from_bytes(request_type, byteorder=BYTE_ORDER)
    
    if message == RequestType.SEND_GAME_STATE.value:
      self.receive_game_state()
      
    self.socket_client.close()
    print("Disconnected from server")

  def receive_game_state(self):
    message = self.socket_client.recv(INT_SIZE)
    SYMBOLS_PER_CARD = int.from_bytes(message, byteorder=BYTE_ORDER)
    cuurent_top_card = []
    for _ in range(SYMBOLS_PER_CARD):
      message = self.socket_client.recv(INT_SIZE)
      cuurent_top_card.append(int.from_bytes(message, byteorder=BYTE_ORDER))
    message = self.socket_client.recv(INT_SIZE)
    players_count = int.from_bytes(message, byteorder=BYTE_ORDER)
    player_states = []
    for _ in range(players_count):
      message = self.socket_client.recv(INT_SIZE)
      player_id = int.from_bytes(message, byteorder=BYTE_ORDER)
      current_card = []
      for _ in range(SYMBOLS_PER_CARD):
        message = self.socket_client.recv(INT_SIZE)
        current_card.append(int.from_bytes(message, byteorder=BYTE_ORDER))
      message = self.socket_client.recv(INT_SIZE)
      cards_in_hand_count = int.from_bytes(message, byteorder=BYTE_ORDER)
      message = self.socket_client.recv(INT_SIZE)
      swaps_left = int.from_bytes(message, byteorder=BYTE_ORDER)
      message = self.socket_client.recv(INT_SIZE)
      swaps_cooldown = int.from_bytes(message, byteorder=BYTE_ORDER)
      message = self.socket_client.recv(INT_SIZE)
      freezes_left = int.from_bytes(message, byteorder=BYTE_ORDER)
      message = self.socket_client.recv(INT_SIZE)
      freezes_cooldown = int.from_bytes(message, byteorder=BYTE_ORDER)
      message = self.socket_client.recv(INT_SIZE)
      rerolls_left = int.from_bytes(message, byteorder=BYTE_ORDER)
      message = self.socket_client.recv(INT_SIZE)
      rerolls_cooldown = int.from_bytes(message, byteorder=BYTE_ORDER)
      message = self.socket_client.recv(INT_SIZE)
      is_frozen = bool(int.from_bytes(message, byteorder=BYTE_ORDER))
      player_states.append(PlayerState(player_id, current_card, cards_in_hand_count, swaps_left, swaps_cooldown, freezes_left, freezes_cooldown, rerolls_left, rerolls_cooldown, is_frozen))
    message = self.socket_client.recv(INT_SIZE)
    game_end = int.from_bytes(message, byteorder=BYTE_ORDER)
    if game_end != RequestType.END_REQUEST.value:
      raise Exception("Invalid end request")
    self.game = Game(player_states, players_count, cuurent_top_card)
    

def main():
  client = Client()
  client.run() 

if __name__ == '__main__':
  main()
  