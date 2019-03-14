make : server client
	make server client

server : socket.o server.cc
	g++ -o ./server socket.o server.cc

client : socket.o client.cc
	g++ -o ./client socket.o client.cc

socket.o : socket.h socket.cc
	g++ -c socket.h socket.cc


clean : 

	rm *.gch
	rm socket.o
	rm server
	rm client
