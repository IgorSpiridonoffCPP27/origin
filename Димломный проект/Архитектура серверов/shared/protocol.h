#pragma once

enum MessageType {
    CLIENT_DATA,
    LOG_REQUEST,
    STATS_UPDATE
};

struct MessageHeader {
    MessageType type;
    uint32_t size;
};