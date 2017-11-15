udp-client
===========

This is based on example /contiki/examples/ipv6/rpl-udp/udp-client.c

Sends udp packets to host: 192.168.1.151 udp port: 5678 (and also receives udp packets sent to this node)


On the host machine netcat is used to listen & send/receive packets on udp port 5678
"netcat -ul 192.168.1.151 5678"


