#include <iostream>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include "socket.h"
#include <string.h>
#include <errno.h>
#include "frame_sender.h"
#include <boost/crc.hpp>  // for boost::crc_32_type

using namespace std;

bool rcv_protocol(int sockfd, Init_Frame* frame, sockaddr_in* address, socklen_t* length){
    
    int bytesRCV = recvfrom(sockfd, frame, sizeof(frame), 0, (struct sockaddr *) address, length);
    
    if(bytesRCV >= 0 ){
        cout << "Init rcvd:"<< "\n";
        cout << "\tbytes rcv: " <<  bytesRCV << "\n";
        cout << "\tpayload size: " << frame->payloadSize << "\n";
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
        cout << "\tbytes rcv: " <<  bytesRCV << "\n";
//        cout << frame->packet.data << "\n";
        cout << "\tframe Ack: " << frame->ack << "\n";
//        cout << "\tframe contents: " << frame->packet.data << "\n";
        cout << "\tframe crc: " << frame->crc<< "\n";
        cout << "\n";

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
        cout << "\tBytes sent: "<< bytesSent << "\n";
        cout << "\n";
    }else{
        cout << "Ack process failed sending ack: " << ack_number << "\n";
    }
    return true;
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
    int FILE_SIZE = init_frame.fileSize;
    
    cout << "FILE SIZE: " << FILE_SIZE << "\n";
    cout << "MAX PAYLOAD: " << MAX_PAYLOAD << "\n";

    int expectedAck=0;
    
    bool continueRcving = true;
    ofstream myfile("server.txt",ios::trunc | ios_base::binary );
//    myfile.open("server.txt", ios::trunc);
    boost::crc_32_type  result;
    
    int packetsRcvd = 0;
    while(continueRcving){
        //blank address initialized to who sent us a message
        struct sockaddr_in client_address;
        memset(&client_address, '0', sizeof(client_address));
        //received frame
        Frame received_frame;
        //length of address
        socklen_t length = sizeof(&client_address);
        
        rcv_frame(connection, &received_frame, &client_address, &length);
        
        
        if(received_frame.ack == 2){
            continueRcving = false;
        }else{
            bool shouldSendAck = true;
            if(received_frame.ack == expectedAck){
                
                result.process_bytes( received_frame.packet.data, received_frame.packet.dataLength);
                uint32_t crc = result.checksum();

                //will need once we have timeouts
//                if(crc == received_frame.crc){
                    expectedAck = expectedAck ^1;
                    myfile.write(reinterpret_cast<const char*>(received_frame.packet.data), received_frame.packet.dataLength);
                    packetsRcvd++;
                    cout << "Packets Rcvd: " << packetsRcvd << "\n";
               
//                }else{
//                    shouldSendAck = false;
//                }
                //save received frame data
            }
            if(shouldSendAck){
                send_ack(connection, client_address, length, received_frame.ack);
            }
        }
    }
    
    
}

