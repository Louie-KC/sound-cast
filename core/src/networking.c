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
    multicast_addr_group.sin_addr.s_addr = inet_addr(conn->group_addr);
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
    addr.sin_addr.s_addr = inet_addr(conn->other_addr);
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
    if (dgram->header.kind == SERVER_AD) {
        LOG_WARN("sc_network_send called with server ad. Use sc_network_server_advertise");
        return false;
    }
    if (dgram->header.kind == SERVER_CLOSE && len > sizeof(datagram_header)) {
        LOG_WARN("Datagram size (%d) exceeds limit for SERVER_CLOSE", len);
        return false;
    }
    if (dgram->header.kind == SERVER_AUDIO && len > sizeof(datagram_t)) {
        LOG_WARN("Datagram size (%d) exceeds limit for SERVER_AD", len);
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
    strncpy(conn->group_addr, MULTICAST_TEMP_GROUP, INET_ADDRSTRLEN);
    memset(&conn->other_addr, '\0', INET_ADDRSTRLEN);
}

void sc_socket_client_init(connection_t *conn) {
    // Create socket for receiving multicast audio
    conn->socket_audio_fd = socket(AF_INET, SOCK_DGRAM, 0);
    CORE_ASSERT(conn->socket_audio_fd >= 0);

    // Create socket for sending and receiving auxiliary messages
    conn->socket_aux_fd = socket(AF_INET, SOCK_DGRAM, 0);
    CORE_ASSERT(conn->socket_aux_fd >= 0);

    // Bind aux socket to pick up all broadcast traffic
    struct sockaddr_in all_addr;
    memset(&all_addr, 0, sizeof(struct sockaddr_in));
    all_addr.sin_family = AF_INET;
    all_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    all_addr.sin_port = htons(BROADCAST_TEMP_PORT);
    CORE_ASSERT(bind(conn->socket_aux_fd, (struct sockaddr *) &all_addr, sizeof(struct sockaddr_in)) >= 0);
    LOG_INFO("Client aux socket bound");

    // Bind audio/group socket to multicast port
    struct sockaddr_in group_addr;
    memset(&group_addr, 0, sizeof(struct sockaddr_in));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    group_addr.sin_port = htons(MULTICAST_TEMP_PORT);
    CORE_ASSERT(bind(conn->socket_audio_fd, (struct sockaddr *) &group_addr, sizeof(struct sockaddr_in)) >= 0);
    LOG_INFO("Client group socket bound");

    // Set sequence and flag(s)
    conn->send_sequence = 0;
    conn->recv_sequence = 0;
    conn->is_server = false;
    memset(&conn->group_addr, '\0', INET_ADDRSTRLEN);
    memset(&conn->other_addr, '\0', INET_ADDRSTRLEN);
}

uint8_t sc_socket_client_join(connection_t *conn, char multicast_group[INET_ADDRSTRLEN]) {
    struct ip_mreq mreq;
    int32_t opt_ret;

    mreq.imr_multiaddr.s_addr = inet_addr(multicast_group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    opt_ret = setsockopt(
        conn->socket_audio_fd,
        IPPROTO_IP,
        IP_ADD_MEMBERSHIP,
        (char *) &mreq, sizeof(struct ip_mreq)
    );
    if (opt_ret == 0) {
        // Store joined group address
        memcpy(conn->group_addr, multicast_group, INET_ADDRSTRLEN);
    } else {
        LOG_ERROR("Failed to join multicast group %.*s. Errno [%d] %s", INET_ADDRSTRLEN,
            multicast_group, errno, strerror(errno));
    }
    return opt_ret == 0;
}

uint8_t sc_socket_client_leave(connection_t *conn) {
    struct ip_mreq mreq;
    int32_t opt_ret;

    mreq.imr_multiaddr.s_addr = inet_addr(conn->group_addr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    opt_ret = setsockopt(
        conn->socket_audio_fd,
        IPPROTO_IP,
        IP_DROP_MEMBERSHIP,
        (char *) &mreq,
        sizeof(struct ip_mreq)
    );
    if (opt_ret == 0) {
        memset(&conn->group_addr, '\0', INET_ADDRSTRLEN);
    } else {
        LOG_ERROR("Failed to leave multicast group %.*s. Errno [%d] %s", INET_ADDRSTRLEN,
            conn->group_addr, errno, strerror(errno));
    }
    return opt_ret == 0;
}

void sc_socket_close(connection_t *conn) {
    datagram_t close_notif;
    struct timeval time;
    uint64_t dgram_size;

    if (conn->is_server && conn->group_addr[0] != '\0') {
        gettimeofday(&time, NULL);
        close_notif.header.kind = SERVER_CLOSE;
        close_notif.header.sequence = conn->send_sequence;
        close_notif.header.timestamp = time;
        memcpy(&close_notif.payload.group_addr, conn->group_addr, INET_ADDRSTRLEN);
        LOG_INFO("Broadcasting group close notification");
        // Try broadcasting closure up to 5 times
        dgram_size = sizeof(datagram_header) + INET_ADDRSTRLEN;
        for (int i = 0; i < 5 && !sc_network_send(conn, &close_notif, dgram_size); i++) {
            // empty
        }
    }
    if (conn->socket_audio_fd > 0 && shutdown(conn->socket_audio_fd, 2) < 0) {
        LOG_ERROR("sc_socket_close: multi socket shutdown error. errno [%d] %s", errno, strerror(errno));
    }
    if (conn->socket_aux_fd > 0 && shutdown(conn->socket_aux_fd, 2) < 0) {
        LOG_ERROR("sc_socket_close: aux socket shutdown error. errno [%d] %s", errno, strerror(errno));
    }
    conn->socket_audio_fd = SOCKET_CLOSED_FD;
    conn->socket_aux_fd = SOCKET_CLOSED_FD;
    memset(&conn->group_addr, '\0', INET_ADDRSTRLEN);
    memset(&conn->other_addr, '\0', INET_ADDRSTRLEN);
}

uint8_t sc_network_server_advertise(connection_t *conn) {
    struct timeval time;
    gettimeofday(&time, NULL);

    datagram_t datagram = {
        .header = {
            .kind = SERVER_AD,
            .sequence = conn->send_sequence++,
            .timestamp = time
        },
        .payload = { 0 }  // written with below memcpy
    };
    memcpy(datagram.payload.group_addr, conn->group_addr, INET_ADDRSTRLEN);

    return _broadcast(conn, (uint8_t *) &datagram, sizeof(datagram_header) + INET_ADDRSTRLEN) > 0;
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
    datagram_t *recv_dgram;
    uint8_t buffer[sizeof(datagram_t)];
    struct sockaddr_in src_addr;
    uint32_t addr_len = sizeof(struct sockaddr_in);
    char src_ip_buffer[INET_ADDRSTRLEN];
    int32_t socket_fd = aux ? conn->socket_aux_fd : conn->socket_audio_fd;

    ssize_t bytes_received = recvfrom(
        socket_fd,
        buffer,
        sizeof(datagram_t),
        0,
        (struct sockaddr *) &src_addr,
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
    
    recv_dgram = (datagram_t *) buffer;
    if (recv_dgram->header.kind > SERVER_AUDIO) {
        LOG_WARN("Invalid header on received datagram");
        return false;
    }
    // Copy to `dest` datagram
    dest->header.kind      = recv_dgram->header.kind;
    dest->header.sequence  = recv_dgram->header.sequence;
    dest->header.timestamp = recv_dgram->header.timestamp;
    if (recv_dgram->header.kind == SERVER_AD) {
        // Advertised multicast group address
        memcpy(dest->payload.group_addr, recv_dgram->payload.group_addr, INET_ADDRSTRLEN);
        
        // Store the source IP address for the received datagram
        inet_ntop(AF_INET, &src_addr.sin_addr, src_ip_buffer, INET_ADDRSTRLEN);
        memcpy(conn->other_addr, src_ip_buffer, INET_ADDRSTRLEN);
    }
    else if (recv_dgram->header.kind == SERVER_AUDIO) {
        // Sent audio payload
        memcpy(dest->payload.audio, recv_dgram->payload.audio, DATAGRAM_PAYLOAD_MAX_SIZE);
    }

    conn->recv_sequence++;
    return true;
}
