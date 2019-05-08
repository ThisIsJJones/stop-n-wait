//
//#ifndef frame_sender_hpp
//#define frame_sender_hpp
//
//#include <stdio.h>
//
//#endif /* frame_sender_hpp */

typedef struct {
    
    uint8_t data[8192];
    int dataLength;
    
}Packet;

typedef struct {
    int seq;
    uint32_t crc;
    Packet packet;
}Frame;

typedef struct {
    int ack;
}Ack_Frame;

typedef struct {
    int payloadSize;
    int maxSeqNumber;
    int windowSizeOfServer;
}Init_Frame;
