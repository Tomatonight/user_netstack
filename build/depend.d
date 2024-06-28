arp.o: src/arp.cpp src/arp.h src/ether.h src/skbuff.h src/route.h \
 src/netdev.h src/pcap.h
bitmap.o: src/bitmap.cpp src/bitmap.h
check.o: src/check.cpp src/check.h
icmp.o: src/icmp.cpp src/icmp.h src/skbuff.h src/sock.h src/ip.h \
 src/arp.h src/ether.h src/check.h
interface.o: src/interface.cpp src/interface.h src/socket.h src/sock.h \
 src/ip.h src/arp.h src/ether.h src/skbuff.h src/bitmap.h src/udp.h \
 src/tcp.h src/timer.h src/netdev.h src/pcap.h src/route.h
ip.o: src/ip.cpp src/ip.h src/arp.h src/ether.h src/skbuff.h src/check.h \
 src/sock.h src/udp.h src/bitmap.h src/tcp.h src/timer.h src/route.h \
 src/netdev.h src/pcap.h src/icmp.h
main.o: src/main.cpp src/interface.h
netdev.o: src/netdev.cpp src/netdev.h src/ether.h src/skbuff.h src/pcap.h \
 src/arp.h src/ip.h
pcap.o: src/pcap.cpp src/pcap.h
route.o: src/route.cpp src/route.h src/netdev.h src/ether.h src/skbuff.h \
 src/pcap.h
skbuff.o: src/skbuff.cpp src/skbuff.h
socket.o: src/socket.cpp src/socket.h src/sock.h src/ip.h src/arp.h \
 src/ether.h src/skbuff.h src/bitmap.h src/udp.h src/tcp.h src/timer.h
tcp.o: src/tcp.cpp src/tcp.h src/sock.h src/ip.h src/arp.h src/ether.h \
 src/skbuff.h src/bitmap.h src/timer.h src/check.h
tcp_sock.o: src/tcp_sock.cpp src/tcp.h src/sock.h src/ip.h src/arp.h \
 src/ether.h src/skbuff.h src/bitmap.h src/timer.h src/check.h \
 src/netdev.h src/pcap.h
timer.o: src/timer.cpp src/timer.h
udp.o: src/udp.cpp src/udp.h src/ip.h src/arp.h src/ether.h src/skbuff.h \
 src/sock.h src/bitmap.h src/check.h src/netdev.h src/pcap.h
