CC = g++
CPPFLAGS = -DNDEBUG
CFLAGS = -c -std=c++11 -Wall -Wextra -Wvla

all: Server Client

Client.o: whatsappClient.cpp
	$(CC) $(CPPFLAGS) $(CFLAGS) whatsappClient.cpp -o Client.o

Server.o: whatsappServer.cpp
	$(CC) $(CPPFLAGS) $(CFLAGS) whatsappServer.cpp -o Server.o

main.o: main.cpp
	$(CC) $(CPPFLAGS) $(CFLAGS) main.cpp -o main.o


Server: Server.o whatsappio.o
	${CC} Server.o whatsappio.o -o whatsappServer


whatsappio.o: whatsappio.h whatsappio.cpp
	$(CC) $(CPPFLAGS) $(CFLAGS) whatsappio.cpp -o whatsappio.o

Client: Client.o whatsappio.o
	${CC} Client.o whatsappio.o -o whatsappClient

runner: Client
	chmod a+x runner


%.o: %.cpp %.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	rm -rf runner *.o


val: server
	valgrind --leak-check=full --show-possibly-lost=yes --show-reachable=yes\
	 --undef-value-errors=yes server 3333

.PHONY: clean, tar, all, val

