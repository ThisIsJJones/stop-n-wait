#include <iostream>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include "socket.h"
#include <string.h>
#include <errno.h>
#include "frame_sender.h"


using namespace std;


int main(int argc, char** argv){
    //set up communication and bind to port
//    int connection = setupServerSocket(11332, SOCK_DGRAM);
    
    
    
    // Get a socket of the right type
    int connection = socket(AF_INET, SOCK_DGRAM, 0);
    if (connection < 0) {
        printf("ERROR opening socket\n");
        exit(1);
    }
    
    // port number
    int portno = 11332;
    
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
    if (bind(connection, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR on binding\n");
        exit(1);
    }
    
    
    
    
    
    
    //source address
    struct sockaddr_in client_address;
    memset(&client_address, '0', sizeof(client_address));
    
    //message to buffer
    char msgBuffer[100];
    
    Frame rcv_frame;
    socklen_t length = sizeof(&client_address);
    
    
    int bytesRCV = recvfrom(connection, &rcv_frame, sizeof(rcv_frame), 0, (struct sockaddr *) &client_address, &length);
    
    if(bytesRCV >= 0 ){
        cout << "We got: " <<  bytesRCV << "\n";
        cout << "frame seq: " << rcv_frame.seq << "\n";
        cout << "packet Data: " << rcv_frame.packet.data << "\n";
        
        Frame ack_frame;
        ack_frame.ack = 69;
        
        
        
        int bytesSent = sendto(connection, &rcv_frame, sizeof(rcv_frame), 0, (struct sockaddr *) &client_address, length);
        
        if(bytesSent >= 0 ){
            cout << "We sent: " << bytesSent<<"\n";
        }else{
            cout << "We failed to send: " << errno <<"\n";
        }
        
        
    }else{
        cout << "We failed to got" << errno << "\n";
    }
    
}

