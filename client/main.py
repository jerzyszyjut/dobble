import socket

ADDRESS = "127.0.0.1"
PORT = 8080
MAX_PLAYER_NAME_LENGTH = 32


class Client:
  socket_client: socket.socket
  
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
   
    message = self.socket_client.recv(128).decode()
    print(message)
    
    self.socket_client.close()
    print("Disconnected from server")
  

def main():
  client = Client()
  client.run() 

if __name__ == '__main__':
  main()
  