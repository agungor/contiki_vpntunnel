Document version: 2018-01-07

Distributed Border Router (DBR)
6LowPAN End Point (6EP)
===================================

This project is heavily based on the rpl-border-router in the example directory
of avr-rss2. enc28j60 is used for ethernet driver. 

The packets which needs to be routed to internet are sent to
tunnel. IPv6 header and its payload packed into an IPv4 packet and sent to Internet
End Point (IEP). IEP code is available under /IoT_internal/iep_tun.

A UDP tunnel is established between 6EP and IEP.
