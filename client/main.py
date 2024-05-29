from enum import Enum
import socket
from typing import List
from PyQt5.QtWidgets import (
    QApplication,
    QWidget,
    QGridLayout,
)
import sys
import os
import random
from PyQt5.QtCore import Qt, QRectF, QPointF
from PyQt5.QtGui import QPainter, QBrush, QPen, QPixmap, QTransform
from math import sin, cos


ADDRESS = "127.0.0.1"
PORT = 8080
MAX_PLAYER_NAME_LENGTH = 32
SYMBOLS_PER_CARD = 8
INT_SIZE = 4
BYTE_ORDER = "little"


class DobbleCardWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.image_dir = "symbols"
        self.images = self.load_images()
        self.setGeometry(100, 100, 200, 200)
        self.show()

    def load_images(self):
        image_files = [f for f in os.listdir(self.image_dir) if f.endswith('.png')]
        random.shuffle(image_files)
        image_files = image_files[:8]
        images = [(QPixmap(os.path.join(self.image_dir, img)), img) for img in image_files]
        return images

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        image_coords = []
        # Draw white circle in the center
        center = QPointF(self.width() / 2, self.height() / 2)
        radius = min(self.width(), self.height()) / 2 - 10
        painter.setBrush(QBrush(Qt.white, Qt.SolidPattern))
        painter.setPen(QPen(Qt.black, 2))
        painter.drawEllipse(center, radius, radius)

        ratio = min(self.width(), self.height()) / 300
        new_images_size = (int(50 * ratio), int(50 * ratio))

        angle_step = 360 / (len(self.images) - 1)
        image_radius = new_images_size[0] / 2

        for i, pixmap in enumerate(self.images):
            pixmap, name = pixmap
            if i != len(self.images) - 1:
                angle = angle_step * i
                radians = angle * (3.14159265 / 180)
                x = center.x() + (radius - image_radius - new_images_size[0] / 4) * cos(radians) - image_radius
                y = center.y() + (radius - image_radius - new_images_size[0] / 4) * sin(radians) - image_radius
                pixmap = pixmap.scaled(new_images_size[0], new_images_size[1])
                x, y = int(x), int(y)
                image_coords.append((x, y))
                painter.drawPixmap(x, y, pixmap)
            else:
                pixmap = pixmap.scaled(new_images_size[0], new_images_size[1])
                x = center.x() - image_radius
                y = center.y() - image_radius
                x, y = int(x), int(y)
                painter.drawPixmap(x, y, pixmap)
            image_coords.append((name, (x, y)))
        
        print(image_coords)

class DobbleMainWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.init_ui()

    def init_ui(self):
        self.setWindowTitle("Dobble")
        self.setGeometry(100, 100, 800, 600)

        layout = QGridLayout()
        
        top_card = DobbleCardWidget()
        layout.addWidget(top_card, 0, 0, 2, 2)
        
        my_card = DobbleCardWidget()
        layout.addWidget(my_card, 2, 0, 2, 2)

        for i in range(2):
            for j in range(2):
                card = DobbleCardWidget()
                layout.addWidget(card, i, j + 2)

        self.setLayout(layout)
        self.show()

    def on_click(self):
        print("Button clicked")

class RequestType(Enum):
    SEND_GAME_STATE = 0
    END_REQUEST = 1
    SEND_GAME_METADATA = 2


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

        while not self.finished:
            request_type = self._receive_message(int)

            if RequestType(request_type) == RequestType.SEND_GAME_STATE:
                self._receive_game_state()
            elif RequestType(request_type) == RequestType.SEND_GAME_METADATA:
                self._receive_game_metadata()
                

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
        
    def _get_and_send_username(self):
        username = None
        while username is None or len(username) > MAX_PLAYER_NAME_LENGTH - 1:
            username = input("Enter your username: ")

        self.socket_client.send(username.encode())
 

def main():
    app = QApplication(sys.argv)
    window = DobbleMainWindow()
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
