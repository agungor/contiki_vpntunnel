Hello-iot
===========

This adds the hello-iot process in hello-iot.c to the platform build, which
prints "Hello-iot" to stdout on startup.

The entire platform is built, with uip stack, radio drivers, routing, etc.
So it is not usually a simple build! The native platform is the default:

    make
    ./hello-iot.native
    Starting Contiki
    Hello, IOT

When switching between ipv4 and ipv6 builds on a platform,

    make TARGET=<platform> clean

else the library for that platform will contain duplicate or unresolved
modules.

For example, using a loopback interface with the minimal-net platform:

    cd /examples/hello-iot
    make TARGET=minimal-net
    ./hello-iot.minimal-net
    Hello, iot
    IP Address:  10.1.1.1
    Subnet Mask: 255.0.0.0
    Def. Router: 10.1.1.100
    ^C

    make TARGET=minimal-net clean
    make UIP_CONF_IPV6=1 TARGET=minimal-net
    ./hello-iot.minimal-net
    Hello, iot
    IPV6 Address: [fd00::206:98ff:fe00:232]
    IPV6 Address: [fe80::206:98ff:fe00:232]
    ^C

