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
#include <math.h>
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
//    server = gethostbyname("localhost");
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

bool initalizeProtocol(int sockfd, int maxPayloadSize, int maxSeqNum, int serverWindowSize){
    //destination address
    struct sockaddr_in address = getServerAddress();
    
    Init_Frame frame;
    frame.payloadSize = maxPayloadSize;
    frame.maxSeqNumber = maxSeqNum;
    frame.windowSizeOfServer = serverWindowSize;
    
    //send to server
    int bytesSent = sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *) &address, sizeof(address));
    
    if(bytesSent >= 0 ){
        cout << "Init sent:"<< "\n";
        cout << "\tBytes sent: " << bytesSent << "\n";
        cout << "\tPayload Size: " << maxPayloadSize << "\n";
        cout << "\tMax seq Number: " << maxSeqNum << "\n";
        cout << "\tWindow size: " << serverWindowSize << "\n";
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
//        cout << "\tbytes: " <<  bytesRCV << "\n";

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
        if(frame.seq != 1){//not last packet
//            cout << "Frame succeeded:"<< "\n";
//            cout << "\tSeq#: " << frame.seq << "\n";
            cout << "FRAME SENT: " << frame.seq << "\n";
//            cout << "\tframe contents: " << frame.packet.data << "\n";
//            cout << "\tBytes sent: " << frame.packet.dataLength << "\n";
//            cout << "\tframe crc: " << frame.crc << "\n";
//            cout << "\n";

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

int min(int a, int b){
    return a > b ? b : a;
}

int dist(int a, int b, int maxSeqNum){
    return min(fmod(maxSeqNum+b-a, maxSeqNum),
        fmod(maxSeqNum+a-b, maxSeqNum));
}

Frame readInNextFrame(ifstream* ifs, int seqNum, int MAX_PAYLOAD){
    uint8_t* sendingBuffer;
    uint8_t  buffer[ MAX_PAYLOAD ];
    //clear the buffer
    memset(&buffer, '\0', sizeof(buffer));
    
    //read the files contents up to payload size
    (*ifs).read( reinterpret_cast< char*>(buffer), MAX_PAYLOAD );
    
    sendingBuffer = buffer;
    //did we actually get the amount we wanted
    uint8_t smallerBuffer[ (*ifs).gcount() ];
    if((*ifs).gcount() != MAX_PAYLOAD){
        //fill buff
        memcpy(&smallerBuffer, &buffer, (*ifs).gcount());
        sendingBuffer = smallerBuffer;
    }
    //get crc
    boost::crc_32_type  result;
    result.process_bytes( sendingBuffer, (*ifs).gcount() );
    
    //build up frame
    Packet packet;
    
    memcpy(reinterpret_cast< char*>(packet.data), reinterpret_cast< char*>(sendingBuffer), (*ifs).gcount());
    
    packet.dataLength = (*ifs).gcount();
    Frame frame;
    memcpy(&(frame.packet), &packet, sizeof(packet));
    frame.seq = seqNum;
    frame.crc = result.checksum();
    return frame;
}

int moveWindow(int windowSize, bool* ackWindow, Frame* frameWindow, int* timeWindow){//Time* timeWindow
    int index = 0;
    while(index < windowSize && ackWindow[index]){
        index++;
    }
    
    if(index <= windowSize){
        for(int i = 0; i < windowSize; i++){
            if(i + index < windowSize){
                frameWindow[i] = frameWindow[index+i];
                ackWindow[i] = ackWindow[index+i];
                timeWindow[i] = timeWindow[index+i];
            }else{
                frameWindow[i].seq = -2;
                ackWindow[i] = false;
                timeWindow[i] = 0;
            }
        }
    }
    return index;
}


int main(int argc, char** argv){
    
    
    if(!argv[1]){
        cout << "GIVE A FILE\n";
        return -1;
    }
    
    int packetSizeInput = 1024;
    int sequenceMaxInput = 12;
    int clientWindowSize = 5;
    int serverWindowSize = 5;

    char useDefault = 'y';
    cout << "Would you like to use default values (payload="<<packetSizeInput
        <<", max sequence="<<sequenceMaxInput<<
        ", client window size="<<clientWindowSize<<", server window size="<<serverWindowSize<<") (y or n): ";
    cin >> useDefault;
    
    if(useDefault == 'n'){
        do{
            cout << "Packet Payload Size in Bytes(0-8192): ";
            cin >> packetSizeInput;
        }while(packetSizeInput > 8192 || packetSizeInput <= 0);
        
        do{
            cout << "Max Sequence Number (0-255): ";
            cin >> sequenceMaxInput;
        }while(sequenceMaxInput > 200 || sequenceMaxInput <= 0);
        
        do{
            cout << "Window Size for client (0-125): ";
            cin >> clientWindowSize;
        }while(clientWindowSize > 125 || clientWindowSize <= 0);
        
        do{
            cout << "Window Size for server (0-125): ";
            cin >> serverWindowSize;
        }while(serverWindowSize > 125 || serverWindowSize <= 0);
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

    initalizeProtocol(connection, MAX_PAYLOAD, sequenceMaxInput, serverWindowSize);
    
    
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
    
    
    ofstream myfile("clientValidation.txt", ios::trunc | ios_base::binary );
//    myfile.open("clientValidation.txt", ios::trunc);

    
    
    bool ackWindow[clientWindowSize];
    Frame frameWindow[clientWindowSize];
    int timeWindow[clientWindowSize];
    
    
    if(ifs){
        
        //init window frames
        int seqNum = 0;
        for(int i = 0; i < clientWindowSize; i++){
            Frame readInframe = readInNextFrame(&ifs, seqNum, MAX_PAYLOAD);
             myfile.write(reinterpret_cast< char*>(readInframe.packet.data), ifs.gcount());
            frameWindow[i] = readInframe;
            seqNum = (seqNum + 1) % sequenceMaxInput;
            //set time
            timeWindow[i] = 0;
            ackWindow[i] = false;
        }
        
        bool framesExist = true;
        while(ifs || framesExist){
            if(ackWindow[0]){
                //if the first ack has been received
                //move window to the next ack not rcvd
                int amountMoved = moveWindow(clientWindowSize, ackWindow, frameWindow, timeWindow);
                
                for(int i = 0; i < amountMoved && ifs; i++){
                    //read in available Frames
                    Frame readInframe = readInNextFrame(&ifs, seqNum, MAX_PAYLOAD);
                    myfile.write(reinterpret_cast< char*>(readInframe.packet.data), ifs.gcount());
                    int index = (clientWindowSize - amountMoved) + i;
                    timeWindow[index] = 0;
                    ackWindow[index] = false;
                    frameWindow[index] = readInframe;
                    seqNum = (seqNum + 1) % sequenceMaxInput;
                }
            }
            
            cout << "CURRENT WINDOW [";
            for(int i=0; i<clientWindowSize; i++){
                cout << frameWindow[i].seq << ", ";
             }
            cout<<"]\n";
            
            for(int i=0; i<clientWindowSize; i++){
                //go through all frames
                
                
                if(!ackWindow[i] && frameWindow[i].seq != -2){

                    if(timeWindow[i] == 0){
                        //if timeout or first time sending
                        //send packet at frame[i]
                        send_frame(connection, frameWindow[i]);
                        //reset time
                        timeWindow[i] = 10;
                    }else{
                        timeWindow[i]--;
                    }
                }
            
                fd_set rfds;
            
                FD_ZERO(&rfds);
                FD_SET(connection, &rfds);
            
                //time out
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 50;
            
                int retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
                if(retval == -1){
                    cout << "error\n";
//                if (retval == −1){
//                    perror("select()");
//                } else if (retval){
                }else if (retval){
                    cout << "Data is available now.\n";
                    /* FD_ISSET(0, &rfds) will be true. */
                    while(retval){
                        Ack_Frame ack_frame;
                        rcv_ack(connection, &ack_frame, NULL, NULL);
                        
                        int index = dist(ack_frame.ack, frameWindow[0].seq, sequenceMaxInput);
                        if(index < clientWindowSize){
                            //SHOULD WE CHECK TO SEE IF
                            //THIS ACK IS IN OUR WINDOW?????
                            ackWindow[index] = true;
                        }
                        
                        tv.tv_sec = 0;
                        tv.tv_usec = 50;
                        
                        //check to see if there is another ack to read
                        retval = select(1, &rfds, NULL, NULL, &tv);
                    }
                   
                } else {
                    cout << "no ack within timeout\n";
//                    printf("No data within five seconds.\n");
                }
            }
            
            if(frameWindow[0].seq == -2){
                framesExist = false;
            }
        }
        
        Frame frame;
        frame.seq = -1;
        send_frame(connection, frame);
        myfile.flush();
    }else{
        std::cerr << "Failed to open file " << argv[1] << "." << std::endl;
    }
    
        
    
    
//
//    if ( ifs ){
//        do{
//            uint8_t* sendingBuffer;
//            uint8_t  buffer[ MAX_PAYLOAD ];
//
//            //clear the buffer
//            memset(&buffer, '\0', sizeof(buffer));
//
//            //read the files contents up to payload size
//            ifs.read( reinterpret_cast< char*>(buffer), MAX_PAYLOAD );
//
//            sendingBuffer = buffer;
//            //did we actually get the amount we wanted
//            uint8_t smallerBuffer[ ifs.gcount() ];
//            if(ifs.gcount() != MAX_PAYLOAD){
//
//                //fill buff
//                memcpy(&smallerBuffer, &buffer, ifs.gcount());
//                sendingBuffer = smallerBuffer;
//            }
//            //get crc
//            result.process_bytes( sendingBuffer, ifs.gcount() );
//
//            //validation
//            myfile.write(reinterpret_cast< char*>(sendingBuffer), ifs.gcount());
//
//            //build up frame
//            Packet packet;
//
//            memcpy(reinterpret_cast< char*>(packet.data), reinterpret_cast< char*>(sendingBuffer), ifs.gcount());
//
//            packet.dataLength = ifs.gcount();
//            Frame frame;
//            memcpy(&(frame.packet), &packet, sizeof(packet));
//            frame.seq = sequenceNumber;
//            frame.ack = expectedAck;
//            frame.crc = result.checksum();
//            cout << "Packets left to send: " << totalPacketsToBeSent << "\n";
//
//            clock_t start = clock();
//
//            send_frame(connection, frame);
//
//            Ack_Frame ack_frame;
//            rcv_ack(connection, &ack_frame, NULL, NULL);
//
//            clock_t end =  clock();
////            double timeTaken = difftime(end, start) * 1000.0;
//             double timeTaken = (double)(end-start) / CLOCKS_PER_SEC * 1000.0;
//
//            cout << "Round Trip Time: " << timeTaken << "\n";
//
//            if(ack_frame.ack == expectedAck){
//                cout << "got ack we expected\n";
//                expectedAck = expectedAck^1;
//                sequenceNumber++;
//                sequenceNumber = sequenceNumber%sequenceMaxInput;
//                totalPacketsToBeSent--;
//            }else{
//                cout << "wrong ack, nice one Mr. Nordell\n";
//            }
//
//        } while ( ifs );
    
//        Frame frame;
//        frame.seq = -1;
//        send_frame(connection, frame);
    
}
