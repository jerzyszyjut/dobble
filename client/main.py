import socket
from game import Game

def main():
    game_instance = Game()
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('localhost', 8080))
    s.sendall(b'Hello, world')
    signal = None
    while signal != 1:
        signal = ord(s.recv(1).decode())
        print(signal)
        
    game_instance.get_game_state_from_server(s)
    s.close()


if __name__ == '__main__':
    main()
    