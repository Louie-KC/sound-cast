#include "foo.h"
#include "logger.h"
#include "assert.h"
#include "networking.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        LOG_FATAL("no args");
    }
    connection_t conn;
    if (argv[1][0] == 's') {
        sc_socket_server_init(&conn);

        datagram_t data = {
            .header = {
                .kind = SERVER_AUDIO,
                .sequence = 11,
                .timestamp = {
                    1111111111, 111111
                }
            },
            .payload = { 0 }
        };

        LOG_DEBUG("Server: Advertising");
        for (int i = 0; i < 1; i++) {
            sc_network_server_advertise(&conn);
            LOG_DEBUG("Advertised");
            // sleep(1);
            sc_network_send(&conn, &data, sizeof(datagram_t));
            LOG_DEBUG("Sent");
            LOG_DEBUG("len: %d", sizeof(datagram_t));
        }
    }
    if (argv[1][0] == 'c') {
        datagram_t recv;
        sc_socket_client_init(&conn);
        LOG_DEBUG("Client: Receiving datagram");
        sc_network_receive(&conn, &recv, true);
        LOG_DEBUG("header: { %d, %d, { %d, %d } }", recv.header.kind,
            recv.header.sequence, recv.header.timestamp.tv_sec,
            recv.header.timestamp.tv_usec);
        if (recv.header.kind == SERVER_AD) {
            LOG_DEBUG("advertised group: %.*s", INET_ADDRSTRLEN, recv.payload.group_addr);
            LOG_DEBUG("received from: %.*s", INET_ADDRSTRLEN, conn.other_addr);
            if (sc_socket_client_join(&conn, recv.payload.group_addr)) {
                LOG_INFO("Client joined multicast group");
            }
            sc_network_receive(&conn, &recv, false);
            LOG_DEBUG("header: { %d, %d, { %d, %d } }", recv.header.kind,
                recv.header.sequence, recv.header.timestamp.tv_sec,
                recv.header.timestamp.tv_usec);
            
            if (sc_socket_client_leave(&conn)) {
                LOG_INFO("Client left multicast group");
            }
        }
        else if (recv.header.kind == SERVER_AUDIO) {
            LOG_DEBUG("Audio received");
        }
    }
    LOG_DEBUG("Closing socket");
    sc_socket_close(&conn);

    // foo();

    // LOG_FATAL("fatal test %d test", 5);
    // LOG_ERROR("error test %f test test %u, %d", 1.52f, 0, 321);
    // LOG_WARN("warn test %s aaa", "ree");
    // LOG_INFO("info test");
    // LOG_DEBUG("debug test");

    // CORE_ASSERT(2 == 2);
    // CORE_ASSERT_MSG(1 == 0, "test msg");

    // LOG_INFO("post assert log");


}
