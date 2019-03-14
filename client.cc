#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include "socket.h"
#include <string.h>
#include "frame_sender.h"

using namespace std;


int main(int argc, char** argv){
    
    if(!argv[1]){
        cout << "GIVE A FILE\n";
        return -1;
    }
    FILE *fp = fopen(argv[1], "r");
    int characterCount = 0;
    int ascii;
    
    int MAX_PAYLOAD = 1024;
    char buff[MAX_PAYLOAD];
    while((ascii = fgetc(fp)) != EOF && characterCount<MAX_PAYLOAD){
        buff[characterCount] = ascii;
        characterCount++;
    }
    
    Packet packet;
    strcpy(packet.data, buff);
    
    Frame frame;
    memcpy(&(frame.packet), &packet, sizeof(packet));
    frame.seq = 1;
    frame.ack = 1;
    
    // Get a socket of the right type
    int connection = socket(AF_INET, SOCK_DGRAM, 0);
    if (connection < 0) {
        printf("ERROR opening socket");
        exit(1);
    }
    
    //destination address
    struct sockaddr_in address;
    memset(&address, '0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(11332);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    
    //send to server
    int bytesSent = sendto(connection, &frame, sizeof(frame), 0, (struct sockaddr *) &address, sizeof(address));
    
    if(bytesSent >= 0 ){
        cout << "We sent: "<< bytesSent << "\n";
        cout <<"Waiting to rcv\n";
        
        Frame rcv_frame;
        
        //destination address
        struct sockaddr_in address2;
        memset(&address2, '0', sizeof(address2));
        address2.sin_family = AF_INET;
        address2.sin_port = htons(11332);
        address2.sin_addr.s_addr = htonl(INADDR_ANY);
        
        
        
        char a[100];
        socklen_t length;
         int bytesRCV = recvfrom(connection, &rcv_frame, sizeof(rcv_frame), 0, (struct sockaddr *) &address2, &length);
        if(bytesRCV>0){
            
            cout << rcv_frame.packet.data << "\n";
            cout << "recieved\n";
        }else{
            cout << "noooo: "<< errno<<"\n";
        }
    }else{
        cout << "We failed to send\n";
    }
    
    
}

void printBits(size_t const size, void const * const ptr){
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    for (i=size-1; i>=0; i--)
    {
        for (j=7; j>=0; j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
        printf(" ");
    }
    puts("");
}
