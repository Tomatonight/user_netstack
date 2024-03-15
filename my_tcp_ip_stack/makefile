TRAGET=stack
OBJECT=main.o netdev.o arp.o pcap.o route.o utils.o  timer.o ip.o udp.o socket.o ipc.o  tcp.o tcp_manage.o
LIB= -lpcap
$(TRAGET):$(OBJECT)
	g++  $^ $(LIB) -o $(TRAGET)
%.o:%.cpp
	g++ $^ -c -o $@
clean:
	rm *.o $(TRAGET)