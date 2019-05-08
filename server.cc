#include <iostream>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include "socket.h"
#include <string.h>
#include <errno.h>
#include <math.h>
#include <cmath>
#include "frame_sender.h"
#include <boost/crc.hpp>  // for boost::crc_32_type

using namespace std;

bool rcv_protocol(int sockfd, Init_Frame* frame, sockaddr_in* address, socklen_t* length){
    
    int bytesRCV = recvfrom(sockfd, frame, sizeof((*frame)), 0, (struct sockaddr *) address, length);
    
    if(bytesRCV >= 0 ){
        
        cout << "Init rcvd:"<< "\n";
        cout << "\tbytes rcv: " <<  bytesRCV << "\n";
        cout << "\tpayload size: " << frame->payloadSize << "\n";
        cout << "\tMax Seq Number: " << frame->maxSeqNumber << "\n";
        cout << "\tWindow size: " << frame->windowSizeOfServer << "\n";
        cout << "\n";

    }else{
        cout << "failed to receive init\n";
    }
    return true;
}

int rcv_frame(int sockfd, Frame* frame, sockaddr_in* address, socklen_t* length){
    
    int bytesRCV = recvfrom(sockfd, frame, sizeof(*frame), 0, (struct sockaddr *) address, length);
    
    if(bytesRCV >= 0 ){
        cout << "Rcv frame seq: " << frame->seq << "\n";
//        cout << "\tbytes rcv: " <<  bytesRCV << "\n";
////        cout << frame->packet.data << "\n";
////        cout << "\tframe contents: " << frame->packet.data << "\n";
//        cout << "\tframe crc: " << frame->crc<< "\n";
//        cout << "\n";

    }else{
        cout << "failed to receive init\n";
    }
    return bytesRCV;
}

bool send_ack(int sockfd, sockaddr_in client_address, socklen_t length, int ack_number){
    
    Ack_Frame ack_frame;
    ack_frame.ack = ack_number;
    
    //send ack back to client
    int bytesSent = sendto(sockfd, &ack_frame, sizeof(ack_frame), 0, (struct sockaddr *) &client_address, length);
    
    if(bytesSent >= 0 ){
        cout << "Sent Ack " << ack_number << "\n";
//        cout << "\tBytes sent: "<< bytesSent << "\n";
//        cout << "\n";
    }else{
        cout << "Ack process failed sending ack: " << ack_number << "\n";
    }
    return true;
}

int min(int a, int b){
    return a > b ? b : a;
}

int dist(int a, int b, int maxSeqNum){
    return min(fmod(maxSeqNum+b-a, maxSeqNum),
               fmod(maxSeqNum+a-b, maxSeqNum));
}

int calcThreshold(int currentSeqNum, int maxSequenceNumber){
    int distance = ((maxSequenceNumber + 1)/2)-1;
    int threshold = 0;
    
    if(currentSeqNum - distance < 0){
        threshold = maxSequenceNumber + (currentSeqNum - distance);
    }else{
        threshold = currentSeqNum - distance;
    }
    
    return threshold;
}

void moveWindow(int windowSize, int maxSequenceNumber, bool* ackWindow, int* seqWindow, Frame* frameWindow, ofstream* myfile){
    int index = 0;
    while(index < windowSize && ackWindow[index]){
        //print payload to file
        (*myfile).write(reinterpret_cast<const char*>(frameWindow[index].packet.data), frameWindow[index].packet.dataLength);
        index++;
    }
    
    cout << "INDEX DURING MOVE: " << index << "\n";
    
    if(index <= windowSize){
        for(int i = 0; i < windowSize; i++){
            if(i + index < windowSize){
                seqWindow[i] = seqWindow[index+i];
                frameWindow[i] = frameWindow[index+i];
                ackWindow[i] = ackWindow[index+i];
            }else{
                seqWindow[i] = (seqWindow[i] + index) % maxSequenceNumber;
                ackWindow[i] = false;
                frameWindow[i].seq = -2;
            }
        }
    }
}

int main(int argc, char** argv){
    //set up communication and bind to port
    int connection = setupServerSocket(9605, SOCK_DGRAM);
    
    //blank address initialized to who sent us a message
    struct sockaddr_in client_address;
    memset(&client_address, '0', sizeof(client_address));
    //init rcv frame
    Init_Frame init_frame;
    //length of address
    socklen_t length = sizeof(&client_address);
    //receive protocol
    rcv_protocol(connection, &init_frame, &client_address, &length);
    send_ack(connection, client_address, length, 1);
    
    
    ///start receiving file
    cout << "\n";
    
    int MAX_PAYLOAD = init_frame.payloadSize;
    int maxSequenceNumber = init_frame.maxSeqNumber;
    int windowSize = init_frame.windowSizeOfServer;

    int expectedAck=0;
    
    bool continueRcving = true;
    ofstream myfile("server.txt",ios::trunc | ios_base::binary );
//    myfile.open("server.txt", ios::trunc);
    
    
    int packetsRcvd = 0;
    
    int seqWindow[windowSize];
    bool ackWindow[windowSize];
    Frame frameWindow[windowSize];
    
    //init all window sequence numbers
    for(int i = 0; i < windowSize; i++){
        seqWindow[i] = i;
        ackWindow[i] = false;
    }
    
    int threshold = 0;
    while(continueRcving){
            
        //blank address initialized to who sent us a message
        struct sockaddr_in client_address;
        memset(&client_address, '0', sizeof(client_address));
        //received frame
        Frame received_frame;
        //length of address
        socklen_t length = sizeof(&client_address);

        //blocking call to rcv frame
        rcv_frame(connection, &received_frame, &client_address, &length);

        
        cout << "RCV PAYLOAD\n";
        cout << received_frame.packet.data << "\n";
        
       
        
        if(received_frame.seq == -1){
          continueRcving = false;
        }else{
            if(dist(seqWindow[0], received_frame.seq, maxSequenceNumber) < windowSize &&
               dist(seqWindow[windowSize-1], received_frame.seq, maxSequenceNumber) < windowSize){
                
//                cout << "in our window size\n";
                //in window size
                int index = dist(seqWindow[0], received_frame.seq, maxSequenceNumber);//distance from first sequence number
                boost::crc_32_type  result;
                result.process_bytes( received_frame.packet.data, received_frame.packet.dataLength);
                uint32_t crc = result.checksum();

//                cout << "frames seq: " << received_frame.seq << "\n";
                if(crc == received_frame.crc){
                    frameWindow[index] = received_frame;
                    //SEND ACK HERE
                    send_ack(connection, client_address, length, received_frame.seq);
                    ackWindow[index] = true;
                
                
                cout << "BEFORE MOVING WINDOW\n[";
                for(int i=0; i<windowSize; i++){
                    cout << seqWindow[i] << ", ";
                }
                cout<<"]\n";
                    
                    //packetsRcvd++;
                    //cout << "Packets Rcvd: " << packetsRcvd << "\n";
        
                    if(index == 0){//if the first number was rcvd
                        cout << "MOVING WINDOW\n";
                        moveWindow(windowSize, maxSequenceNumber, ackWindow, seqWindow, frameWindow, &myfile);
                        threshold = calcThreshold(seqWindow[0], maxSequenceNumber);
                    }
                cout << "AFTER MOVING WINDOW\n[";
                for(int i=0; i<windowSize; i++){
                    cout << seqWindow[i] << ", ";
                }
                cout<<"]\n";
                } //end of crc
            }else if(received_frame.seq >= threshold && received_frame.seq < seqWindow[0]){
                //past frame
                //send ACK
                cout << "PAST FRAME\n";
                send_ack(connection, client_address, length, received_frame.seq);
            }else{
                //in future
                cout << "FUTURE FRAME\n";
                //DONT CARE -- for now
                //send ack back of seqWindow[0]??
            }
        }
        
        
        //OLD CODE BELOW
        
        
        
//        //blank address initialized to who sent us a message
//        struct sockaddr_in client_address;
//        memset(&client_address, '0', sizeof(client_address));
//        //received frame
//        Frame received_frame;
//        //length of address
//        socklen_t length = sizeof(&client_address);
//
//        rcv_frame(connection, &received_frame, &client_address, &length);
//
//
//        if(received_frame.ack == 2){
//            continueRcving = false;
//        }else{
//            bool shouldSendAck = true;
//            if(received_frame.ack == expectedAck){
//
//                result.process_bytes( received_frame.packet.data, received_frame.packet.dataLength);
//                uint32_t crc = result.checksum();
//
//                //will need once we have timeouts
////                if(crc == received_frame.crc){
//                    expectedAck = expectedAck ^1;
//                    myfile.write(reinterpret_cast<const char*>(received_frame.packet.data), received_frame.packet.dataLength);
//                    packetsRcvd++;
//                    cout << "Packets Rcvd: " << packetsRcvd << "\n";
//
////                }else{
////                    shouldSendAck = false;
////                }
//                //save received frame data
//            }
//            if(shouldSendAck){
//                send_ack(connection, client_address, length, received_frame.ack);
//            }
//        }
    }
    myfile.flush();

}

