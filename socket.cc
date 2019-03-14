#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "socket.h"
#include <netdb.h>
#include <stdbool.h>


//connectionType -> SOCK_STREAM, SOCK_DGRAM, SOCK_SEQPACKET
int setupServerSocket(int port, int connectionType){ // Like new ServerSocket in Java
    // Get a socket of the right type
    int sockfd = socket(AF_INET, connectionType, 0);
    if (sockfd < 0) {
        printf("ERROR opening socket\n");
        exit(1);
    }

    // port number
    int portno = port;

    // server address structure
    struct sockaddr_in serv_addr;

    // Set all the values in the server address to 0
    memset(&serv_addr, '0', sizeof(serv_addr));

    // Setup the type of socket (internet vs filesystem)
    serv_addr.sin_family = AF_INET;
    
    // Basically the machine we are on...
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Setup the port number
    // htons - is host to network byte order
    // network byte order is most sig bype first
    //   which might be host or might not be
    serv_addr.sin_port = htons(portno);

    // Bind the socket to the given port
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR on binding\n");
        exit(1);
    }

    return sockfd;
}

int callServer(char* host, int port){ // Like new Socket in Java
    // Socket pointer
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr,"ERROR opening socket\n");
        exit(0);
    }

    // port number
    int portno = port;

    // server address structure
    struct sockaddr_in serv_addr;

    // Set all the values in the server address to 0
    memset(&serv_addr, '0', sizeof(serv_addr));

    // Setup the type of socket (internet vs filesystem)
    serv_addr.sin_family = AF_INET;

    // Setup the port number
    // htons - is host to network byte order
    // network byte order is most sig byte first
    //   which might be host or might not be
    serv_addr.sin_port = htons(portno);

    // Setup the server host address
    struct hostent *server;
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);  /// dest, src, size

    // Connect to the server
    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
        exit(0);
    }
    return sockfd;
}

int serverSocketAccept(int serverSocket){ // Like ss.accept() in Java
    // set it up to listen
    listen(serverSocket,4);

    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    // Wait for a call
    printf("waiting for a call...\n");
    newsockfd = accept(serverSocket, (struct sockaddr *) &cli_addr, &clilen);
    printf("connected\n");
    if (newsockfd < 0) {
        printf("ERROR on accept\n");
        exit(1);
    }

    return newsockfd;
}

void writeInt(int x, int socket){ // Write an int over the given socket
    int n = write(socket,&x,5);
    if (n < 0) {
        printf("ERROR writing to socket\n");
        exit(1);
    }
}

int readInt(int socket){ // Read an int from the given socket
    int buffer = 0;
    int n = read(socket,&buffer,5);
    if (n < 0) {
        printf("ERROR reading from socket\n");
        exit(1);
    }

    return buffer;
}


void writeBoolArray(bool* prime, int socket, int arrayLength){
	int n = write(socket, prime, sizeof(bool) * arrayLength);
    if (n < 0){
		printf("ERROR writing to socket\n");
		exit(1);
	}
}

bool* readBoolArray(int socket, int arrayLength){ // Read an bool from the given socket
    bool* buffer = (bool*)malloc(sizeof(bool)*arrayLength);
    
	int n = read(socket, buffer, sizeof(bool) * arrayLength);
	int totalRead = n;
		
	while(totalRead != arrayLength){
		n = read(socket, buffer+totalRead, (sizeof(bool)*(arrayLength))-totalRead);
		totalRead += n;
	}
	
	if (n < 0) {
        printf("ERROR reading from socket\n");
        exit(1);
    }

    return buffer;
}


