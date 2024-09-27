import socket

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind(('192.168.0.115', 4321))

while True:
    message, address = server_socket.recvfrom(1024)
    print("Message is", message)
    print("Address is", address)
    print("=======================")
