import socket
import ipaddress
# A UDP server

# Set up a UDP server
UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

# Buffer size
BUFFER = 1024

# Listen on port 5678
# (to all IP addresses on this system)
listen_addr = ("",9000)
UDPSock.bind(listen_addr)
seq = 0

while True:
    data,addr = UDPSock.recvfrom(BUFFER)
    print('Recvd   from:{} data:{}'.format(addr, data[36:].decode('ascii')))
    src_addr = data[:16]
    src_port = data[16:18]
    dst_port = data[34:36]
    src_addr = ipaddress.ip_address(data[:16])
    dst_addr = ipaddress.ip_address(data[18:34])
    print('source address: ' + str(src_addr))
    print('dst address: ' + str(dst_addr))
    # insert tunnel src/dst data in reverse order to reply to sender
    message = bytearray(dst_addr.packed + dst_port + src_addr.packed + src_port)
    message.extend(("Message from server #" + str(seq)).encode('ascii'))
    print('Sending to  :{} data:{}'.format(addr, message[36:]))
    UDPSock.sendto(message, addr)
    seq += 1