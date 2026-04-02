import socket
import time

PORT = 6667  # Change to your server's port

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", PORT))

print("Sending first half of a command...")
s.sendall(b"NICK us")
time.sleep(2)  # The server should do NOTHING yet!

print("Sending the rest of the command, plus half of the next...")
s.sendall(b"va\r\nUSER usva 0 * ")
time.sleep(2)  # The server should process NICK, but wait on USER!

print("Finishing it up...")
s.sendall(b":Real Name\r\n")

time.sleep(1)
s.close()
print("Done!")
