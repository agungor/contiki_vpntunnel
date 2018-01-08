Document version: 2018-01-07

Distributed Border Router (DBR)
Internet End Point (IEP)
===================================

This project is based http://www.secdev.org/projects/tuntap_udp/files/tunproxy.c
nat64 part is taken from the contiki core/net/ip64 and ported to linux.

IEP uses linux tun interface to route packets to internet. 

The packets which needs to be routed to internet are sent to
tunnel by 6EP to IEP and vice versa. IPv6 header and its payload 
packed into an IPv4 packet and sent to Internet End Point (IEP). 

A UDP tunnel is established between 6EP and IEP.

How to run

Enable IP forwarding:
echo "1" > /proc/sys/net/ipv4/ip_forward
Enable IP Masquerade:
sudo iptables -t nat -A POSTROUTING -s 10.0.0.0/24 -o eth0 -j MASQUERADE
Run the program:
sudo ./iep_tun
Assign an IP address to tun interface:
sudo ifconfig toto0 10.0.0.1 netmask 255.255.255.0
