from enum import Enum
import socket
from typing import List
from PyQt5.QtWidgets import (
    QApplication,
    QWidget,
    QGridLayout,
    QLabel,
    QVBoxLayout,
    QLineEdit,
    QPushButton,
    QMessageBox
)
import sys
import os
import random
from PyQt5.QtCore import Qt, QRectF, QPointF, QRect, pyqtSignal, QObject
from PyQt5.QtGui import QPainter, QBrush, QPen, QPixmap, QFont
from math import sin, cos, pi
import threading
import queue


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
        
    def update(self, image_path, symbol_name):
        self.image_path = image_path
        self.symbol_name = symbol_name
        self.setPixmap(QPixmap(image_path).scaled(self.width(), self.height(), Qt.KeepAspectRatio, Qt.SmoothTransformation))
        self.setAlignment(Qt.AlignCenter)
        self.setStyleSheet("border: 2px solid transparent;")
        self.show()
        
class DobbleCardCanvas(QWidget):
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
            images.append(image_files[image_id])
        return images

    def initUI(self):
        self.setGeometry(100, 100, 200, 200)

        self.layout = QVBoxLayout(self)
        self.card_widget = QWidget(self)
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
        center = QPointF(self.width() / 2, self.height() / 2)
        radius = min(self.width(), self.height()) / 2 - 10

        ratio = min(self.width(), self.height()) / 300
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

        center = QPointF(self.width() / 2, self.height() / 2)
        radius = min(self.width(), self.height()) / 2 - 10
        painter.setBrush(QBrush(Qt.white, Qt.SolidPattern))
        painter.setPen(QPen(Qt.black, 2))
        painter.drawEllipse(center, radius, radius)
        
        painter.end()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.update_symbol_positions()
        
    def update(self, cards):
        self.cards = cards
        self.images = self.load_images()
        for i, (image_path, name) in enumerate(self.images):
            self.symbol_labels[i].update(image_path, str(name))
        self.update_symbol_positions()
            
    def symbol_clicked(self, symbol_name):
        if self.game_client is not None:
            self.game_client.send_card_move(int(symbol_name))
     

class DobbleCardWidget(QWidget):
    def __init__(self, card_name, cards=None, game_client=None, is_clickable=False, player_id=None):
        super().__init__()
        self.is_clickable = is_clickable
        self.card_name = card_name
        self.cards = cards
        self.game_client = game_client
        self.player_id = player_id
        self.initUI()

    def initUI(self):
        self.setGeometry(100, 100, 200, 200)

        self.layout = QGridLayout(self)
        self.title_label = QLabel(self.card_name)
        self.layout.addWidget(self.title_label, 0, 0)
        if self.player_id != -1:
            self.cards_in_hand_label = QLabel(f"Cards in hand: {self.game_client.game.player_states[self.game_client.my_id].cards_in_hand_count}")
            self.layout.addWidget(self.cards_in_hand_label, 0, 1)
        
        self.card_widget = DobbleCardCanvas(self.card_name, cards=self.cards, game_client=self.game_client, is_clickable=self.is_clickable)
        self.layout.addWidget(self.card_widget, 1, 0, 2, 2)
        
        if self.player_id != -1:
            self.display_abilities()

        self.show()
        
    def display_abilities(self):
        self.abilities_label = QLabel("Abilities")
        self.layout.addWidget(self.abilities_label, 3, 0)
        is_top_card = self.player_id == -1
        is_main_player_card = self.player_id == self.game_client.my_id
       
        if not is_top_card:
            if not is_main_player_card:
                swaps_left = self.game_client.game.player_states[self.game_client.my_id].swaps_left
                swaps_cooldown = self.game_client.game.player_states[self.game_client.my_id].swaps_cooldown
                swaps_message = f"Swaps left: {swaps_left}, Cooldown: {swaps_cooldown}"
                self.swap_button = QPushButton(swaps_message)
                self.swap_button.clicked.connect(self.handle_swap)
                self.layout.addWidget(self.swap_button, 3, 1)
                
                freezes_left = self.game_client.game.player_states[self.game_client.my_id].freezes_left
                freezes_cooldown = self.game_client.game.player_states[self.game_client.my_id].freezes_cooldown
                freezes_message = f"Freezes left: {freezes_left}, Cooldown: {freezes_cooldown}"
                self.freeze_button = QPushButton(freezes_message)
                self.freeze_button.clicked.connect(self.handle_freeze)
                self.layout.addWidget(self.freeze_button, 3, 2)
            else:
                rerolls_left = self.game_client.game.player_states[self.game_client.my_id].rerolls_left
                rerolls_cooldown = self.game_client.game.player_states[self.game_client.my_id].rerolls_cooldown
                rerolls_message = f"Rerolls left: {rerolls_left}, Cooldown: {rerolls_cooldown}"
                self.reroll_button = QPushButton(rerolls_message)
                self.reroll_button.clicked.connect(self.handle_reroll)
                self.layout.addWidget(self.reroll_button, 3, 1)
        
        self.update_ability_buttons()
            
        self.show()
        
    def update_ability_buttons(self):
        is_top_card = self.player_id == -1
        is_main_player_card = self.player_id == self.game_client.my_id
        if not is_top_card:
            if not is_main_player_card:
                swaps_left = self.game_client.game.player_states[self.game_client.my_id].swaps_left
                swaps_cooldown = self.game_client.game.player_states[self.game_client.my_id].swaps_cooldown
                self.swap_button.setText(f"Swaps left: {swaps_left}, Cooldown: {swaps_cooldown}")
                self.swap_button.setDisabled(self.game_client.game.player_states[self.game_client.my_id].swaps_left == 0 or self.game_client.game.player_states[self.game_client.my_id].swaps_cooldown > 0)
                freezes_left = self.game_client.game.player_states[self.game_client.my_id].freezes_left
                freezes_cooldown = self.game_client.game.player_states[self.game_client.my_id].freezes_cooldown
                self.freeze_button.setText(f"Freezes left: {freezes_left}, Cooldown: {freezes_cooldown}")
                self.freeze_button.setDisabled(self.game_client.game.player_states[self.game_client.my_id].freezes_left == 0 or self.game_client.game.player_states[self.game_client.my_id].freezes_cooldown > 0)
            else:
                rerolls_left = self.game_client.game.player_states[self.game_client.my_id].rerolls_left
                rerolls_cooldown = self.game_client.game.player_states[self.game_client.my_id].rerolls_cooldown
                self.reroll_button.setText(f"Rerolls left: {rerolls_left}, Cooldown: {rerolls_cooldown}")
                self.reroll_button.setDisabled(self.game_client.game.player_states[self.game_client.my_id].rerolls_left == 0 or self.game_client.game.player_states[self.game_client.my_id].rerolls_cooldown > 0)

    def update(self):
        if self.player_id != -1:
            current_player_state = self.game_client.game.player_states[self.player_id]
            cards = current_player_state.current_card
            self.cards_in_hand_label.setText(f"Cards in hand: {current_player_state.cards_in_hand_count}")
            self.update_ability_buttons()
        else:
            cards = self.game_client.game.current_top_card
        if self.player_id != -1 and self.player_id == self.game_client.my_id:
            self.title_label.setText(f"{self.card_name} (You)")
        elif self.player_id != -1:
            player_name = self.game_client.game.player_states[self.player_id].name
            self.title_label.setText(player_name)
        self.card_widget.update(cards)
        
    def handle_swap(self):
        self.game_client.send_swap(self.player_id)
        
    def handle_freeze(self):
        self.game_client.send_freeze(self.player_id)
        
    def handle_reroll(self):
        self.game_client.send_reroll()
            
class DobbleMainWindow(QWidget):
    def __init__(self, game_client):
        super().__init__()
        self.game_client = game_client
        game_client.update_signal.connect(self.update_cards)
        game_client.end_game_signal.connect(self.handle_end_game)
        game_client.update_message.connect(self.display_message)
        self.top_card = None
        self.my_card = None
        self.other_cards = []
        self.init_ui()
        
    def update_cards(self):
        self.top_card.update()
        self.my_card.update()
        other_players = self.game_client.game.player_states.copy()
        other_players = [player for player in other_players if player.player_id != self.game_client.my_id]
        for other_card in self.other_cards:
            other_card.update()

    def load_server_login_ui(self):
        self.server_address_input = QLineEdit()
        self.server_address_input.setPlaceholderText("Server address")
        self.server_address_input.setText(ADDRESS)
        self.layout.addWidget(self.server_address_input, 0, 0)
        
        self.server_port_input = QLineEdit()
        self.server_port_input.setPlaceholderText("Server port")
        self.server_port_input.setText(str(PORT))
        self.layout.addWidget(self.server_port_input, 1, 0)
        
        self.username_input = QLineEdit()
        self.username_input.setPlaceholderText("Username")
        self.layout.addWidget(self.username_input, 2, 0)
        
        self.join_button = QPushButton("Join")
        self.join_button.clicked.connect(self.handle_join)
        self.layout.addWidget(self.join_button, 3, 0)
        
        self.show()
        
    def clear_server_login_ui(self):
        self.layout.removeWidget(self.server_address_input)
        self.layout.removeWidget(self.server_port_input)
        self.layout.removeWidget(self.username_input)
        self.layout.removeWidget(self.join_button)
        self.server_address_input.deleteLater()
        self.server_port_input.deleteLater()
        self.username_input.deleteLater()
        self.join_button.deleteLater() 

    def load_cards_ui(self):
        self.top_card = DobbleCardWidget("Top card", game_client=self.game_client, cards=self.game_client.game.current_top_card, player_id=-1)
        self.layout.addWidget(self.top_card, 0, 0)
        
        self.my_card = DobbleCardWidget("Your card", game_client=self.game_client, cards=self.game_client.game.player_states[self.game_client.my_id].current_card, is_clickable=True, player_id=self.game_client.my_id) 
        self.layout.addWidget(self.my_card, 1, 0)

        other_players = self.game_client.game.player_states.copy()
        other_players = [player for player in other_players if player.player_id != self.game_client.my_id]
        
        for i, player in enumerate(other_players):
            player_card = DobbleCardWidget(player.name, game_client=self.game_client, cards=player.current_card, player_id=player.player_id)
            self.layout.addWidget(player_card, i // 2, i % 2 + 1)
            self.other_cards.append(player_card)

        self.show()
        
    def clear_cards_ui(self):
        self.layout.removeWidget(self.top_card)
        self.layout.removeWidget(self.my_card)
        for card in self.other_cards:
            self.layout.removeWidget(card)
            card.deleteLater()
        self.top_card.deleteLater()
        self.my_card.deleteLater()
        self.other_cards = []
        
    def load_end_game_ui(self):
        self.clear_cards_ui()
        self.end_game_label = QLabel("Game finished")
        self.layout.addWidget(self.end_game_label, 0, 0)
        self.player_scores_table = QGridLayout()
        self.layout.addLayout(self.player_scores_table, 1, 0)
        for i, player in enumerate(self.game_client.game.player_states):
            player_label = QLabel(f"Player {player.player_id}: { player.cards_in_hand_count}")
            self.player_scores_table.addWidget(player_label, i, 0)
        self.close_button = QPushButton("Close")
        self.close_button.clicked.connect(self.close)
        self.layout.addWidget(self.close_button, 2, 0)
        self.show()

    def init_ui(self):
        self.setWindowTitle("Dobble")
        self.setGeometry(100, 100, 800, 600)
        
        self.layout = QGridLayout()
        self.setLayout(self.layout)
        
        self.load_server_login_ui()
        
    def closeEvent(self, event):
        self.game_client.finished = True
        self.game_client._send_finish_game()
        self.game_client.receive_thread.join()
        self.game_client.socket_client.close()
        event.accept()
        
    def handle_end_game(self):
        self.load_end_game_ui()
        
    def handle_join(self):
        self.join_button.setDisabled(True)
        address = self.server_address_input.text()
        port = int(self.server_port_input.text())
        username = self.username_input.text()
        self.game_client.run(address, port, username)
        self.clear_server_login_ui()
        self.load_cards_ui() 
        
    def display_message(self, message: str) -> None:
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Information)
        msg.setText(message)
        msg.setWindowTitle("Message")
        msg.exec_()

class RequestType(Enum):
    SEND_GAME_STATE = 0
    END_REQUEST = 1
    SEND_GAME_METADATA = 2
    MAKE_ACTION = 3
    FINISH_GAME = 4
    SEND_RETURN_CODE = 5

class GameAction(Enum):
    CARD = 0
    SWAP = 1
    FREEZE = 2
    REROLL = 3
    
class ReturnCode(Enum):
    SUCCESS = 0
    SYMBOL_DOES_NOT_MATCH_WITH_TOP_CARD = 1
    PLAYER_DOES_NOT_HAVE_THIS_SYMBOL = 2
    ABILITY_NOT_AVAILABLE = 3
    PLAYER_IS_FROZEN = 4
    ERROR = 5

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
    is_frozen_count: int
    name: str

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
        is_frozen_count: int = 0,
        name: str = "",
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
        self.is_frozen_count = is_frozen_count
        self.name = name

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


class Client(QObject):
    socket_client: socket.socket
    game: Game
    started: bool
    finished: bool
    my_id: int
    update_signal = pyqtSignal()
    end_game_signal = pyqtSignal()
    update_message = pyqtSignal(str)
    receive_thread = None

    def __init__(self):
        super().__init__()
        self.socket_client = None
        self.game = Game()
        self.started = False
        self.finished = False

    def run(self, address=ADDRESS, port=PORT, username="player"):
        self.socket_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        try:
            self.socket_client.connect((address, port))
        except ConnectionRefusedError:
            print("Server is not available")
            self.end_game_signal.emit()
            return

        self._receive_communication_metadata()
        
        self._send_username(username)

        request_type = self._receive_message(int)
        self._receive_game_metadata()

        request_type = self._receive_message(int)
        self._receive_game_state()
       
        self.started = True
        self.finished = False
        
        self.receive_thread = threading.Thread(target=self._receive_data_loop)
        self.receive_thread.start()
        
    def _receive_data_loop(self):
        while not self.finished:
            request_type = self._receive_message(int)
            request_type = RequestType(request_type)
            if request_type == RequestType.SEND_GAME_STATE:
                self._receive_game_state()
            elif request_type == RequestType.FINISH_GAME:
                self.finished = True
                self._send_finish_game()
                self.end_game_signal.emit()
                print("Game finished")
            elif request_type == RequestType.SEND_RETURN_CODE:
                return_code = self._receive_message(int)
                if ReturnCode(return_code) != ReturnCode.SUCCESS:
                    self.update_message.emit(ReturnCode(return_code).name)
            self.update_signal.emit()
        self.socket_client.close()
        
    def _send_finish_game(self):
        if self.finished:
            return
        self._send_message(RequestType.FINISH_GAME.value)

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
            name = self._receive_message(str)
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
            is_frozen_count = self._receive_message(int)
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
                    is_frozen_count,
                )
            )

        game_end = self._receive_message(int)
        if RequestType(game_end) != RequestType.END_REQUEST:
            raise Exception("Invalid end request")

        self.game.update(player_states, players_count, current_top_card)

    def _receive_message(self, message_type=int):
        if message_type == int:
            message = self.socket_client.recv(INT_SIZE)
            return int.from_bytes(message, byteorder=BYTE_ORDER)
        elif message_type == str:
            message = self.socket_client.recv(MAX_PLAYER_NAME_LENGTH)
            return message.decode()
        else:
            raise Exception("Invalid message type")
        
    def _send_message(self, message, message_type=int):
        if message_type == int:
            self.socket_client.send(message.to_bytes(INT_SIZE, byteorder=BYTE_ORDER))
        else:
            raise Exception("Invalid message type")
        
    def _send_username(self, username):
        if len(username) > MAX_PLAYER_NAME_LENGTH:
            username = username[:MAX_PLAYER_NAME_LENGTH]
        self.socket_client.send(username.encode())

    def _send_game_action(self, request_type: RequestType, action: GameAction, id, hash):
        self._send_message(request_type.value)
        self._send_message(action.value)
        self._send_message(id)
        self._send_message(hash)
        self._send_message(RequestType.END_REQUEST.value)
        
    def send_card_move(self, card_id):
        if self.finished:
            return
        self._send_game_action(RequestType.MAKE_ACTION, GameAction.CARD, card_id, 0)

    def send_swap(self, target_player_id):
        self._send_game_action(
            RequestType.MAKE_ACTION,
            GameAction.SWAP,
            target_player_id,
            0 
        )
        
    def send_freeze(self, target_player_id):
        self._send_game_action(
            RequestType.MAKE_ACTION,
            GameAction.FREEZE,
            target_player_id,
            0
        )
        
    def send_reroll(self):
        self._send_game_action(
            RequestType.MAKE_ACTION,
            GameAction.REROLL,
            0,
            0
        )

def main():
    app = QApplication(sys.argv)
    client = Client()
    window = DobbleMainWindow(client)
    sys.exit(app.exec_())
    

if __name__ == "__main__":
    main()
