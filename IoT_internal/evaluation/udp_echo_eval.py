import socket
import ipaddress
# A UDP server

# Set up a UDP Socket
UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

# Buffer size
BUFFER = 1024

# Listen on port 5678
# (to all IP addresses on this system)
listen_addr = ("",5678)
UDPSock.bind(listen_addr)
seq = 0

while True:
    data,addr = UDPSock.recvfrom(BUFFER)
    UDPSock.sendto(data, addr)
    seq += 1
