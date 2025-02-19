#pragma once

#include "defines.h"
#include "types.h"

#define MULTICAST_TEMP_GROUP "224.0.0.1"
#define MULTICAST_TEMP_PORT 6000
#define BROADCAST_TEMP_PORT 6001
#define UNICAST_TEMP_PORT 6002

#define IPV4_ADDR_MAX_LEN 16

// Initialise server sockets
CORE_API void sc_socket_server_init(connection_t *conn);

// Initialise client sockets
CORE_API void sc_socket_client_init(connection_t *conn);

// Shutdown server/client sockets
CORE_API void sc_socket_close(connection_t *conn);

// Broadcast server availability.
// Returns true if successful, false otherwise.
CORE_API uint8_t sc_network_server_advertise(connection_t *conn);

// Send a datagram. Unicast, multicast, or broadcast based on `conn` and `dragm` header.
// Returns true if successful, false otherwise.
CORE_API uint8_t sc_network_send(connection_t *conn, datagram_t *dgram, uint32_t len);

// Receive a datagram via multicast (audio) socket or aux socket per `aux` param.
// Returns true if successful, false otherwise.
CORE_API uint8_t sc_network_receive(connection_t *conn, datagram_t *dest, uint8_t aux);
