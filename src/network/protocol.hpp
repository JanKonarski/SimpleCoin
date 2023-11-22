#pragma once

#include <string>

struct Message {
    enum MessageType {
        NANE
    ,   PEER_LIST_REQUEST
    ,   PEER_LIST_RESPONSE
    ,   KEY_EXCHANGE
    ,   BLOCK
    ,   CONTROL_MESSAGE
    ,   CLOSE
    };

    MessageType type;
    char data[1024*1024];

    Message() {
        std::fill(data, data + sizeof(data), '\0');
    }
};

class Protocol {
private:


public:
    Protocol() {}


};
