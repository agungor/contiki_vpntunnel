import socket
import select

# Buffer size
BUFFER = 1024

# Listen on port 5678
# (to all IP addresses on this system)

#listen_addr = ("",5678)

sockets = []

for port in range(6000,6101):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.bind(('', port))
    sockets.append(server_socket)

while True:
    readable, writable, exceptional = select.select(sockets, [], [])
    for s in readable:
         (data, addr) = s.recvfrom(BUFFER)
         s.sendto(data, addr)

for s in sockets:
   s.close()
