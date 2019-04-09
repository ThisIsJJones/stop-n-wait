make : server client
	make server client

server : socket.o server.cc
	g++ -std=c++11 -o ./server socket.o server.cc

client : socket.o client.cc
	g++ -std=c++11 -o ./client socket.o client.cc

socket.o : socket.h socket.cc
	g++ -std=c++11 -c socket.h socket.cc


clean : 

	rm *.gch
	rm socket.o
	rm server
	rm client
