#pragma once

#include "defines.h"
#include "types.h"

#define MULTICAST_TEMP_GROUP "224.0.0.1"
#define MULTICAST_TEMP_PORT 6000
#define BROADCAST_TEMP_PORT 6001
#define UNICAST_TEMP_PORT 6002

// Initialise server sockets
CORE_API void sc_socket_server_init(connection_t *conn);

// Initialise client aux socket
CORE_API void sc_socket_client_init(connection_t *conn);

// Join a multicast group, storing the address in `conn` on success.
// Returns true if successful, false otherwise.
CORE_API uint8_t sc_socket_client_join(connection_t *conn, char multicast_group[INET_ADDRSTRLEN]);

// Leave a multicast group, clearing the stored group address from `conn` on success.
// Returns true if successful, false otherwise.
CORE_API uint8_t sc_socket_client_leave(connection_t *conn);

// Shutdown server/client sockets
CORE_API void sc_socket_close(connection_t *conn);

// Broadcast server availability.
// Returns true if successful, false otherwise.
CORE_API uint8_t sc_network_server_advertise(connection_t *conn);

// Send a datagram. Unicast, multicast, or broadcast based on `conn` and `dragm` header.
// Returns true if successful, false otherwise.
CORE_API uint8_t sc_network_send(connection_t *conn, datagram_t *dgram, uint32_t len);

// Receive a datagram via multicast (audio) socket or aux socket per `aux` param.
// If a SERVER_AD is received, the datagrams source IP address is stored in `conn`.
// Returns true if successful, false otherwise.
CORE_API uint8_t sc_network_receive(connection_t *conn, datagram_t *dest, uint8_t aux);
