#pragma once

#include "defines.h"
#include "networking.h"
#include <sys/time.h>  // struct timeval
#include <netinet/in.h>

#define DATAGRAM_PAYLOAD_MAX_SIZE 1  // TODO: revise

typedef enum {
    SERVER_AD = 0,
    SERVER_CLOSE = 1,
    SERVER_AUDIO = 2
} datagram_kind;

typedef struct __attribute__((__packed__)) {
    datagram_kind kind;
    uint32_t sequence;
    struct timeval timestamp;
} datagram_header;

typedef union __attribute__((__packed__)) {
    char group_addr[INET_ADDRSTRLEN];          // SERVER_AD
    uint8_t audio[DATAGRAM_PAYLOAD_MAX_SIZE];  // SERVER_AUDIO
} datagram_payload;

typedef struct __attribute__((__packed__)) {
    datagram_header  header;
    datagram_payload payload;
} datagram_t;

typedef struct {
    int32_t  socket_audio_fd;
    int32_t  socket_aux_fd;
    uint32_t send_sequence;
    uint32_t recv_sequence;
    uint8_t  is_server;
    char     group_addr[INET_ADDRSTRLEN];
    char     other_addr[INET_ADDRSTRLEN];  // Client: server addr. Server: unused.
} connection_t;
