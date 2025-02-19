#include "networking.h"

#include "types.h"
#include "logger.h"
#include "assert.h"

#include <string.h>      // memset(...)
#include <netinet/in.h>  // sockaddr_in, AF_INET
#include <arpa/inet.h>   // htonl(...)
#include <sys/socket.h>
#include <time.h>        // struct timeval, gettimeofday(...)
#include <errno.h>

#define SOCKET_CLOSED_FD 0

ssize_t _broadcast(connection_t *conn, uint8_t *buffer, uint32_t len) {
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    addr.sin_port = htons(BROADCAST_TEMP_PORT);

    ssize_t bytes_sent = sendto(
        conn->socket_aux_fd,
        buffer,
        len,
        0,
        (struct sockaddr *) &addr,
        sizeof(struct sockaddr_in)
    );
    
    if (bytes_sent == 0) {
        LOG_ERROR("broadcast: 0 bytes sent");
    }
    if (bytes_sent < 0) {
        LOG_ERROR("broadcast: send_to failed. errno [%d] %s", errno, strerror(errno));
    }
    return bytes_sent;
}

ssize_t _multicast(connection_t *conn, uint8_t *buffer, uint32_t len) {
    struct sockaddr_in multicast_addr_group;

    memset(&multicast_addr_group, 0, sizeof(struct sockaddr_in));
    multicast_addr_group.sin_family = AF_INET;
    multicast_addr_group.sin_addr.s_addr = inet_addr(MULTICAST_TEMP_GROUP);
    multicast_addr_group.sin_port = htons(MULTICAST_TEMP_PORT);

    ssize_t bytes_sent = sendto(
        conn->socket_audio_fd,
        buffer,
        len,
        0,
        (struct sockaddr *) &multicast_addr_group,
        sizeof(struct sockaddr_in)
    );

    if (bytes_sent == 0) {
        LOG_ERROR("multicast: 0 bytes sent");
    }
    if (bytes_sent < 0) {
        LOG_ERROR("multicast: send_to failed. errno [%d] %s", errno, strerror(errno));
    }
    return bytes_sent;
}

ssize_t _unicast(connection_t *conn, uint8_t *buffer, uint32_t len) {
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(conn->dest_addr);
    addr.sin_port = htons(UNICAST_TEMP_PORT);

    ssize_t bytes_sent = sendto(
        conn->socket_aux_fd,
        buffer,
        len,
        0,
        (struct sockaddr *) &addr,
        sizeof(struct sockaddr_in)
    );

    if (bytes_sent == 0) {
        LOG_ERROR("unicast: 0 bytes sent");
    }
    if (bytes_sent < 0) {
        LOG_ERROR("unicast: send_to failed. Errno [%d] %s", errno, strerror(errno));
    }
    return bytes_sent;
}

uint8_t _sc_network_send_valid_params(datagram_t *dgram, uint32_t len) {
    // Validity check
    if (len > sizeof(datagram_t)) {
        LOG_WARN("Attempted to send datagram with too large `len` (%d)", len);
        return false;
    }
    if (dgram->header.kind > 2) {  // 2 = max enum value (SERVER_AUDIO)
        LOG_WARN("Attempted to send datagram with invalid header kind ('%d')", dgram->header.kind);
        return false;
    }
    if (len < sizeof(datagram_header)) {
        LOG_WARN("Attempted to send datagram less than header");
        return false;
    }
    if (dgram->header.kind != SERVER_AUDIO && len > sizeof(datagram_header)) {
        LOG_WARN("Datagram size (%d) exceeds limit", len);
        return false;
    }
    if (dgram->header.kind == SERVER_AD) {
        LOG_WARN("sc_network_send called with server ad. Use sc_network_server_advertise");
        return false;
    }
    return true;
}

void sc_socket_server_init(connection_t *conn) {
    int32_t opt_ret;
    int32_t boardcast_enable = 1;  // must be 32 bit else Errno 22 Invalid arg

    // Create socket for sending multicast audio
    conn->socket_audio_fd = socket(AF_INET, SOCK_DGRAM, 0);
    CORE_ASSERT(conn->socket_audio_fd >= 0);

    // Create socket for auxiliary messages
    conn->socket_aux_fd = socket(AF_INET, SOCK_DGRAM, 0);
    CORE_ASSERT(conn->socket_aux_fd >= 0);

    // Enable broadcasting on auxiliary socket
    opt_ret = setsockopt(
        conn->socket_aux_fd,
        SOL_SOCKET,
        SO_BROADCAST,
        &boardcast_enable,
        sizeof(int32_t)
    );
    if (opt_ret < 0) {
        LOG_ERROR("sc_socket_init: Failed to enable broadcasting. Errno [%d] %s", errno, strerror(errno));
    }

    // Set sequence and flag(s)
    conn->send_sequence = 0;
    conn->recv_sequence = 0;
    conn->is_server = true;
    strncpy(conn->dest_addr, MULTICAST_TEMP_GROUP, IPV4_ADDR_MAX_LEN);
}

void sc_socket_client_init(connection_t *conn) {
    // Create socket for receiving multicast audio
    conn->socket_audio_fd = socket(AF_INET, SOCK_DGRAM, 0);
    CORE_ASSERT(conn->socket_audio_fd >= 0);

    // Create socket for sending and receiving auxiliary messages
    conn->socket_aux_fd = socket(AF_INET, SOCK_DGRAM, 0);
    CORE_ASSERT(conn->socket_aux_fd >= 0);

    struct sockaddr_in addr_audio;
    memset(&addr_audio, 0, sizeof(struct sockaddr_in));
    addr_audio.sin_family = AF_INET;
    addr_audio.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_audio.sin_port = htons(MULTICAST_TEMP_PORT);
    CORE_ASSERT(bind(conn->socket_audio_fd, (struct sockaddr *) &addr_audio, sizeof(struct sockaddr_in)) >= 0);
    LOG_INFO("Client multi socket bound");

    // Bind receiving auxiliary message socket
    struct sockaddr_in addr_aux;
    memset(&addr_aux, 0, sizeof(struct sockaddr_in));
    addr_aux.sin_family = AF_INET;
    addr_aux.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_aux.sin_port = htons(BROADCAST_TEMP_PORT);
    CORE_ASSERT(bind(conn->socket_aux_fd, (struct sockaddr *) &addr_aux, sizeof(struct sockaddr_in)) >= 0);
    LOG_INFO("Client aux socket bound");

    // Join multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_TEMP_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    CORE_ASSERT(setsockopt(conn->socket_audio_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                            (char*) &mreq, sizeof(mreq)) >= 0);
    LOG_INFO("Joined multicast group: %s", MULTICAST_TEMP_GROUP);

    // Set sequence and flag(s)
    conn->send_sequence = 0;
    conn->recv_sequence = 0;
    conn->is_server = false;
    memset(&conn->dest_addr, '\0', IPV4_ADDR_MAX_LEN);
}

void sc_socket_close(connection_t *conn) {
    if (conn->socket_audio_fd > 0 && shutdown(conn->socket_audio_fd, 2) < 0) {
        LOG_ERROR("sc_socket_close: multi socket shutdown error. errno [%d] %s", errno, strerror(errno));
    }
    if (conn->socket_aux_fd > 0 && shutdown(conn->socket_aux_fd, 2) < 0) {
        LOG_ERROR("sc_socket_close: aux socket shutdown error. errno [%d] %s", errno, strerror(errno));
    }
    conn->socket_audio_fd = SOCKET_CLOSED_FD;
    conn->socket_aux_fd = SOCKET_CLOSED_FD;
}

uint8_t sc_network_server_advertise(connection_t *conn) {
    struct timeval time;
    gettimeofday(&time, NULL);

    datagram_header header = {
        .kind = SERVER_AD,
        .sequence = conn->send_sequence++,
        .timestamp = time
    };

    return _broadcast(conn, (uint8_t *) &header, sizeof(datagram_header)) > 0;
}

uint8_t sc_network_send(connection_t *conn, datagram_t *dgram, uint32_t len) {
    if (!_sc_network_send_valid_params(dgram, len)) {
        return false;
    }
    
    ssize_t bytes_sent = 0;
    uint8_t *bytes = (uint8_t *) dgram;
    if (conn->is_server) {
        if (dgram->header.kind == SERVER_AUDIO) {
            bytes_sent = _multicast(conn, bytes, len);
        } else {
            bytes_sent = _broadcast(conn, bytes, len);
        }
    } else {
        bytes_sent = _unicast(conn, bytes, len);
    }
    if (bytes_sent <= 0) {
        return false;
    }
    conn->send_sequence++;
    return true;
}

uint8_t sc_network_receive(connection_t *conn, datagram_t *dest, uint8_t aux) {
    uint8_t buffer[sizeof(datagram_t)];
    int32_t socket_fd;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    if (aux) {
        addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        addr.sin_port = htons(BROADCAST_TEMP_PORT);

        socket_fd = conn->socket_aux_fd;
    } else {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(MULTICAST_TEMP_PORT);
        
        socket_fd = conn->socket_audio_fd;
    }

    uint32_t addr_len = sizeof(struct sockaddr_in);

    ssize_t bytes_received = recvfrom(
        socket_fd,
        buffer,
        sizeof(datagram_t),
        0,
        (struct sockaddr *) &addr,
        &addr_len
    );
    if (bytes_received < 0) {
        LOG_ERROR("sc_network_receive: errno [%d] %s", errno, strerror(errno));
        return false;
    }
    if (bytes_received == 0) {
        LOG_ERROR("sc_network_receive: socket has been shutdown");
        return false;
    }
    
    datagram_t *recv_dgram = (datagram_t *) buffer;
    if (recv_dgram->header.kind > SERVER_AUDIO) {
        LOG_WARN("Invalid header on received datagram");
        return false;
    }
    dest->header.kind      = recv_dgram->header.kind;
    dest->header.sequence  = recv_dgram->header.sequence;
    dest->header.timestamp = recv_dgram->header.timestamp;
    if (recv_dgram->header.kind == SERVER_AUDIO) {
        memcpy(dest->payload, recv_dgram->payload, DATAGRAM_PAYLOAD_MAX_SIZE);
    }

    conn->recv_sequence++;
    return true;
}
