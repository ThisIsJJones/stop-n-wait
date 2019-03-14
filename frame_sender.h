//
//#ifndef frame_sender_hpp
//#define frame_sender_hpp
//
//#include <stdio.h>
//
//#endif /* frame_sender_hpp */

typedef struct {
    char data[1024];
}Packet;

typedef struct {
    int seq;
    int ack;
    Packet packet;
}Frame;
