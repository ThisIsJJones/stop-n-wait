#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include "socket.h"
#include <string.h>
#include "frame_sender.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cmath>
#include <boost/crc.hpp>  // for boost::crc_32_type
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>

using namespace std;

sockaddr_in getServerAddress(){
    //destination address
    struct sockaddr_in address;
    memset(&address, '0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(9605);
    
    // Setup the server host address
    struct hostent *server;
    server = gethostbyname("thing3.cs.uwec.edu");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
//    memcpy(&(address.sin_addr.s_addr), server->h_addr, server->h_length);  /// dest, src, size
    
    address.sin_addr = *((struct in_addr *)server->h_addr);
    memset(&(address.sin_zero), '\0', 8); // zero the rest of the struct
    
    
    
//    address.sin_addr.s_addr = htonl(INADDR_ANY);
    //"10.35.195.22"
    return address;
}

bool initalizeProtocol(int sockfd, int maxPayloadSize, int fileSize){
    //destination address
    struct sockaddr_in address = getServerAddress();
    
    Init_Frame frame;
    frame.payloadSize = maxPayloadSize;
    frame.fileSize = fileSize;
    
    //send to server
    int bytesSent = sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *) &address, sizeof(address));
    
    if(bytesSent >= 0 ){
        cout << "Init sent:"<< "\n";
        cout << "\tBytes sent: " << bytesSent << "\n";
        cout << "\tPayload Size: " << maxPayloadSize << "\n";
        cout << "\tFile size: " << fileSize << "\n";
        cout << "\n";

    }else{
        cout << "Init process failed" << "\n";
    }
    return true;
}

bool rcv_ack(int sockfd, Ack_Frame* frame, sockaddr_in* address, socklen_t* length){
    
    int bytesRCV = recvfrom(sockfd, frame, sizeof(frame), 0, (struct sockaddr *) address, length);
    
    if(bytesRCV >= 0 ){
        cout << "Ack rcv: " << frame->ack << "\n";
        cout << "\tbytes: " <<  bytesRCV << "\n";

    }else{
        cout << "failed to receive ack\n";
    }
    
    return true;
}

int send_frame(int sockfd, Frame frame){
    //destination address
    struct sockaddr_in address = getServerAddress();
    
    //send to server
    int bytesSent = sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *) &address, sizeof(address));
    
    if(bytesSent >= 0 ){
        if(frame.ack != 2){//not last packet
            cout << "Frame succeeded:"<< "\n";
            cout << "\tSeq#: " << frame.seq << "\n";
//            cout << "\tframe contents: " << frame.packet.data << "\n";
            cout << "\tBytes sent: " << frame.packet.dataLength << "\n";
            cout << "\tframe ack: " << frame.ack << "\n";
            cout << "\tframe crc: " << frame.crc << "\n";
            cout << "\n";

        }
    }else{
        cout << "Send process failed" << "\n";
    }
    return bytesSent;
}

int getFilesSize(string filename){
    ifstream  ifs( filename, std::ios_base::binary | ios::ate );
    int size = ifs.tellg();
    ifs.close();
    return size;
}

int main(int argc, char** argv){
    
    char useDefault = 'y';
    cout << "Would you like to use default values (payload=1024, max sequence=5) (y or n): ";
    cin >> useDefault;
    
    int packetSizeInput = 1024;
    int sequenceMaxInput = 5;
    
    if(useDefault == 'n'){
        do{
            cout << "Packet Payload Size in Bytes(0-8192): ";
            cin >> packetSizeInput;
        }while(packetSizeInput > 8192 || packetSizeInput <= 0);
        
        do{
            cout << "Max Sequence Number (0-100): ";
            cin >> sequenceMaxInput;
        }while(sequenceMaxInput > 100 || sequenceMaxInput <= 0);
    }
    
    if(!argv[1]){
        cout << "GIVE A FILE\n";
        return -1;
    }
    
    int MAX_PAYLOAD = packetSizeInput;
    int FILE_SIZE = getFilesSize(argv[1]);
    int totalPacketsToBeSent = ceil(FILE_SIZE / packetSizeInput);
    
    ifstream  ifs( argv[1], std::ios_base::binary );
    
    // Get a socket of the right type
    int connection = socket(AF_INET, SOCK_DGRAM, 0);
    if (connection < 0) {
        printf("ERROR opening socket");
        exit(1);
    }

    initalizeProtocol(connection, MAX_PAYLOAD, FILE_SIZE);
    
    
    Ack_Frame ack_frame;
    //blank address initialized to who sent us a message
    struct sockaddr_in client_address;
    memset(&client_address, '0', sizeof(client_address));
    //init rcv frame
    Init_Frame init_frame;
    //length of address
    socklen_t length = sizeof(&client_address);
    
    rcv_ack(connection, &ack_frame, &client_address, &length);
    
    //START SENDING FILE OVER
    cout << "\n";
    
    int characterCount = 0;
    int ascii;
    
    bool sending_file = true;
    
    int expectedAck = 0;
    int sequenceNumber = 0;
    
    
    ofstream myfile("clientValidation.txt",ios::trunc | ios_base::binary );
//    myfile.open("clientValidation.txt", ios::trunc);
    boost::crc_32_type  result;
    
    if ( ifs ){
        do{
            uint8_t* sendingBuffer;
            uint8_t  buffer[ MAX_PAYLOAD ];
            
            //clear the buffer
            memset(&buffer, '\0', sizeof(buffer));
            
            //read the files contents up to payload size
            ifs.read( reinterpret_cast< char*>(buffer), MAX_PAYLOAD );
            
            sendingBuffer = buffer;
            //did we actually get the amount we wanted
            uint8_t smallerBuffer[ ifs.gcount() ];
            if(ifs.gcount() != MAX_PAYLOAD){
                
                //fill buff
                memcpy(&smallerBuffer, &buffer, ifs.gcount());
                sendingBuffer = smallerBuffer;
            }
            //get crc
            result.process_bytes( sendingBuffer, ifs.gcount() );
            
            //validation
            myfile.write(reinterpret_cast< char*>(sendingBuffer), ifs.gcount());
            
            //build up frame
            Packet packet;
            
            memcpy(reinterpret_cast< char*>(packet.data), reinterpret_cast< char*>(sendingBuffer), ifs.gcount());
            
            packet.dataLength = ifs.gcount();
            Frame frame;
            memcpy(&(frame.packet), &packet, sizeof(packet));
            frame.seq = sequenceNumber;
            frame.ack = expectedAck;
            frame.crc = result.checksum();
            cout << "Packets left to send: " << totalPacketsToBeSent << "\n";
            
            clock_t start = clock();
            
            send_frame(connection, frame);
            
            Ack_Frame ack_frame;
            rcv_ack(connection, &ack_frame, NULL, NULL);
            
            clock_t end =  clock();
//            double timeTaken = difftime(end, start) * 1000.0;
             double timeTaken = (double)(end-start) / CLOCKS_PER_SEC * 1000.0;

            cout << "Round Trip Time: " << timeTaken << "\n";
            
            if(ack_frame.ack == expectedAck){
                cout << "got ack we expected\n";
                expectedAck = expectedAck^1;
                sequenceNumber++;
                sequenceNumber = sequenceNumber%sequenceMaxInput;
                totalPacketsToBeSent--;
            }else{
                cout << "wrong ack, nice one Mr. Nordell\n";
            }
            
        } while ( ifs );
        
        Frame frame;
        frame.seq = 0;
        frame.ack = 2;
        send_frame(connection, frame);
        
    }else{
        std::cerr << "Failed to open file '" << argv[1] << "'."
        << std::endl;
    }
    
}
