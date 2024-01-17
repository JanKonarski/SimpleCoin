#pragma once

#include <string>
#include <cstring>

enum MessageType{
    TRANSACTION_TRANSFER
,   TRANSACTION_REQUEST
,   BLOCK_TRANSFER
,   BLOCK_REQUEST
,   CHAIN_TRANSFER
,   CHAIN_REQUEST
,   CHAIN_SYNC
,   NULL_RESPONSE
};

struct Message {
    MessageType type;
    char data[1024*16];

    Message() {
        std::fill(data, data + sizeof(data), '\n');
    }

    Message(MessageType type, std::string msg="")
    : type(type)
    {
        Message();
        strcpy(data, msg.c_str());
    }
};