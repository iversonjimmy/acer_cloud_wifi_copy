/* Creates a pair of sockets that are connected to each other.
 */

#ifndef __CONNECTED_SOCKET_PAIR_HPP__
#define __CONNECTED_SOCKET_PAIR_HPP__

#include <vpl_socket.h>

class ConnectedSocketPair {
public:
    ConnectedSocketPair();
    ~ConnectedSocketPair();
    int GetSockets(VPLSocket_t sock[2]);
private:
    int create();
    VPLSocket_t sock[2];
};

#endif // incl guard
