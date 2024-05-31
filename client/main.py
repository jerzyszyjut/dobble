from enum import Enum
import socket
from typing import List
from PyQt5.QtWidgets import (
    QApplication,
    QWidget,
    QGridLayout,
    QLabel,
    QVBoxLayout
)
import sys
import os
import random
from PyQt5.QtCore import Qt, QRectF, QPointF, QRect, pyqtSignal
from PyQt5.QtGui import QPainter, QBrush, QPen, QPixmap, QFont
from math import sin, cos, pi


ADDRESS = "127.0.0.1"
PORT = 8080
MAX_PLAYER_NAME_LENGTH = 32
SYMBOLS_PER_CARD = 8
INT_SIZE = 4
BYTE_ORDER = "little"

class SymbolLabel(QLabel):
    clicked = pyqtSignal(str)

    def __init__(self, image_path, symbol_name, size=50, is_clickable=False, parent=None):
        super().__init__(parent)
        self.image_path = image_path
        self.symbol_name = symbol_name
        self.is_clickable = is_clickable
        self.setPixmap(QPixmap(image_path).scaled(size, size, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        self.setAlignment(Qt.AlignCenter)
        self.setFixedSize(size, size)
        self.setStyleSheet("border: 2px solid transparent;")

    def enterEvent(self, event):
        if self.is_clickable:
            self.setStyleSheet("border: 2px solid red;")
        super().enterEvent(event)

    def leaveEvent(self, event):
        if self.is_clickable:
            self.setStyleSheet("border: 2px solid transparent;")
        super().leaveEvent(event)

    def mousePressEvent(self, event):
        if self.is_clickable and event.button() == Qt.LeftButton:
            self.clicked.emit(str(self.symbol_name))
        super().mousePressEvent(event)
        
    def rescale(self, size):
        self.setPixmap(QPixmap(self.image_path).scaled(size, size, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        self.setFixedSize(size, size)

class DobbleCardWidget(QWidget):
    def __init__(self, card_name, cards=None, game_client=None, is_clickable=False):
        super().__init__()
        self.image_dir = "symbols"
        self.is_clickable = is_clickable
        self.card_name = card_name
        self.cards = cards
        self.game_client = game_client
        self.images = self.load_images()
        self.symbol_labels = []
        self.initUI()

    def load_images(self):
        image_files = [(os.path.join(self.image_dir, f), int(f.split(".")[0])) for f in os.listdir(self.image_dir) if f.endswith('.png')]
        image_files = sorted(image_files, key=lambda x: x[1])
        images = []
        for image_id in self.cards:
            images.append(image_files[image_id - 1])
        return images

    def initUI(self):
        self.setGeometry(100, 100, 200, 200)

        self.layout = QVBoxLayout(self)
        self.title_label = QLabel(self.card_name)
        self.title_label.setAlignment(Qt.AlignHCenter | Qt.AlignTop)
        self.layout.addWidget(self.title_label)
        
        self.card_widget = QWidget(self)
        self.card_layout = QVBoxLayout(self.card_widget)
        self.layout.addWidget(self.card_widget)

        self.create_symbols()
        self.show()

    def create_symbols(self):
        for image_path, name in self.images:
            symbol_label = SymbolLabel(image_path, str(name), is_clickable=self.is_clickable, parent=self)
            symbol_label.clicked.connect(self.symbol_clicked)
            self.symbol_labels.append(symbol_label)
        self.update_symbol_positions()

    def update_symbol_positions(self):
        y_offset = 20

        center = QPointF(self.width() / 2, self.height() / 2 + y_offset)
        radius = min(self.width(), self.height() - y_offset) / 2 - 10

        ratio = min(self.width(), self.height() - y_offset) / 300
        new_image_size = int(50 * ratio)

        angle_step = 360 / (len(self.symbol_labels) - 1)
        image_radius = new_image_size / 2

        for i, symbol_label in enumerate(self.symbol_labels):
            if i != len(self.symbol_labels) - 1:
                angle = angle_step * i
                radians = angle * (pi / 180)
                x = center.x() + (radius - image_radius - new_image_size / 4) * cos(radians) - image_radius
                y = center.y() + (radius - image_radius - new_image_size / 4) * sin(radians) - image_radius
            else:
                x = center.x() - image_radius
                y = center.y() - image_radius
                
            x, y = int(x), int(y)

            symbol_label.move(x, y)
            symbol_label.rescale(new_image_size)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        y_offset = 20

        center = QPointF(self.width() / 2, self.height() / 2 + y_offset)
        radius = min(self.width(), self.height() - y_offset) / 2 - 10
        painter.setBrush(QBrush(Qt.white, Qt.SolidPattern))
        painter.setPen(QPen(Qt.black, 2))
        painter.drawEllipse(center, radius, radius)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.update_symbol_positions()

    def symbol_clicked(self, symbol_name):
        print(f"Symbol clicked: {symbol_name}")       
        
        
class DobbleMainWindow(QWidget):
    def __init__(self, game_client):
        super().__init__()
        self.game_client = game_client
        self.init_ui()

    def init_ui(self):
        self.setWindowTitle("Dobble")
        self.setGeometry(100, 100, 800, 600)

        layout = QGridLayout()
        
        top_card = DobbleCardWidget("Top card", cards=self.game_client.game.current_top_card)
        layout.addWidget(top_card, 0, 0, 2, 2)
        
        my_card = DobbleCardWidget("Your card", game_client=self.game_client, cards=self.game_client.game.player_states[self.game_client.my_id].current_card, is_clickable=True)    
        layout.addWidget(my_card, 2, 0, 2, 2)

        other_players = self.game_client.game.player_states.copy()
        other_players = [player for player in other_players if player.player_id != self.game_client.my_id]
        
        for i, player in enumerate(other_players):
            player_card = DobbleCardWidget(f"Player {player.player_id}", cards=player.current_card)
            layout.addWidget(player_card, i // 2, i % 2 + 2)
            

        self.setLayout(layout)
        self.show()

class RequestType(Enum):
    SEND_GAME_STATE = 0
    END_REQUEST = 1
    SEND_GAME_METADATA = 2
    MAKE_ACTION = 3

class GameAction(Enum):
    CARD = 0
    SWAP = 1
    FREEZE = 2
    REROLL = 3



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

    def __init__(
        self,
        player_id: int = 0,
        current_card: List[int] = None,
        cards_in_hand_count: int = 13,
        swaps_left: int = 1,
        swaps_cooldown: int = 3,
        freezes_left: int = 1,
        freezes_cooldown: int = 3,
        rerolls_left: int = 1,
        rerolls_cooldown: int = 3,
        is_frozen: bool = False,
    ):
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

        if current_card is None:
            self.current_card = [i for i in range(1, SYMBOLS_PER_CARD + 1)]


class Game:
    player_states: List[PlayerState]
    players_count: int
    current_top_card: List[int]

    def __init__(
        self,
        player_states: List[PlayerState] = None,
        players_count: int = 4,
        current_top_card: List[int] = None,
    ):
        self.player_states = player_states
        self.players_count = players_count
        self.current_top_card = current_top_card
        if player_states is None:
            self.player_states = [PlayerState() for _ in range(players_count)]
        if current_top_card is None:
            self.current_top_card = [i for i in range(1, SYMBOLS_PER_CARD + 1)]

    def update(
        self,
        player_states: List[PlayerState] = None,
        players_count: int = None,
        current_top_card: List[int] = None,
    ):
        if player_states is not None:
            self.player_states = player_states
        if players_count is not None:
            self.players_count = players_count
        if current_top_card is not None:
            self.current_top_card = current_top_card


class Client:
    socket_client: socket.socket
    game: Game
    started: bool
    finished: bool
    my_id: int

    def __init__(self):
        self.socket_client = None
        self.game = Game()
        self.started = False
        self.finished = False

    def run(self):
        self.socket_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket_client.connect((ADDRESS, PORT))

        self._receive_communication_metadata()
        
        self._get_and_send_username()

        request_type = self._receive_message(int)
        self._receive_game_metadata()

        request_type = self._receive_message(int)
        self._receive_game_state()

        self._sent_game_action(RequestType.MAKE_ACTION, GameAction.CARD, 1, 0)
                        
        self.socket_client.close()

    def _receive_communication_metadata(self):
        global INT_SIZE, BYTE_ORDER
        
        message = self.socket_client.recv(1)
        INT_SIZE = ord(message)
        message = self.socket_client.recv(1)
        BYTE_ORDER = "little" if bool(ord(message)) else "big"
        
    def _receive_game_metadata(self):
        global SYMBOLS_PER_CARD
        SYMBOLS_PER_CARD = self._receive_message(int)
        self.my_id = self._receive_message(int)
        end_request_message = self._receive_message(int)
        if RequestType(end_request_message) != RequestType.END_REQUEST:
            raise Exception("Invalid end request")

    def _receive_game_state(self):
        SYMBOLS_PER_CARD = self._receive_message(int)
        current_top_card = []
        for _ in range(SYMBOLS_PER_CARD):
            current_top_card.append(self._receive_message(int))
        players_count = self._receive_message(int)
        player_states = []
        for _ in range(players_count):
            player_id = self._receive_message(int)
            current_card = []
            for _ in range(SYMBOLS_PER_CARD):
                current_card.append(self._receive_message(int))
            cards_in_hand_count = self._receive_message(int)
            swaps_left = self._receive_message(int)
            swaps_cooldown = self._receive_message(int)
            freezes_left = self._receive_message(int)
            freezes_cooldown = self._receive_message(int)
            rerolls_left = self._receive_message(int)
            rerolls_cooldown = self._receive_message(int)
            is_frozen = bool(self._receive_message(int))
            player_states.append(
                PlayerState(
                    player_id,
                    current_card,
                    cards_in_hand_count,
                    swaps_left,
                    swaps_cooldown,
                    freezes_left,
                    freezes_cooldown,
                    rerolls_left,
                    rerolls_cooldown,
                    is_frozen,
                )
            )

        game_end = self._receive_message(int)
        if RequestType(game_end) != RequestType.END_REQUEST:
            raise Exception("Invalid end request")

        self.game.update(player_states, players_count, current_top_card)
        self.finished = True

    def _receive_message(self, message_type=int):
        if message_type == int:
            message = self.socket_client.recv(INT_SIZE)
            return int.from_bytes(message, byteorder=BYTE_ORDER)
        else:
            raise Exception("Invalid message type")
        
    def _send_message(self, message, message_type=int):
        if message_type == int:
            self.socket_client.send(message.to_bytes(INT_SIZE, byteorder=BYTE_ORDER))
        else:
            raise Exception("Invalid message type")
        
    def _get_and_send_username(self):
        username = None
        while username is None or len(username) > MAX_PLAYER_NAME_LENGTH - 1:
            username = input("Enter your username: ")

        self.socket_client.send(username.encode())

    def _sent_game_action(self, request_type: RequestType, action: GameAction, id, hash):
        self._send_message(request_type.value)
        self._send_message(action.value)
        self._send_message(id)
        self._send_message(hash)
        self._send_message(RequestType.END_REQUEST.value)
        new_request_type = self._receive_message(int)
        if RequestType(new_request_type) != RequestType.SEND_GAME_STATE:
            raise Exception("Expected game state")
        self._receive_game_state()

 

def main():
    # app = QApplication(sys.argv)
    client = Client()
    client.run()
    #window = DobbleMainWindow(client)
    #sys.exit(app.exec_())
    

if __name__ == "__main__":
    main()
