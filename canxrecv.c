#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "x/can/lib/can.h" // For struct canfd_frame
#include "x/can/lib.h"     // For sprint_long_canframe

#define MULTICAST_GRP "239.192.1.1"
#define MULTICAST_PORT 55555
#define BUF_SIZE 256 // Should be larger than sizeof(struct canfd_frame)

int main(int argc, char *argv[]) {
    int s; // UDP socket
    struct sockaddr_in local_addr;
    struct ip_mreq mreq;
    char buf[BUF_SIZE];
    ssize_t nbytes;
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    int reuseport = 1;

    /* Create UDP socket */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating UDP socket");
        return 1;
    }

    /* Allow multiple sockets to bind to the same port */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport)) < 0) {
        perror("Error setting SO_REUSEPORT");
        close(s);
        return 1;
    }

    /* Set up local address structure */
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(MULTICAST_PORT);

    /* Bind to the local address and port */
    if (bind(s, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("Error binding UDP socket");
        close(s);
        return 1;
    }

    /* Set up multicast group structure */
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GRP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY); // Use default interface

    /* Join the multicast group */
    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Error joining multicast group");
        close(s);
        return 1;
    }

    printf("Listening for CAN frames on UDP multicast group %s:%d...\n", MULTICAST_GRP, MULTICAST_PORT);

    /* Loop receiving packets */
    while (1) {
        nbytes = recvfrom(s, buf, BUF_SIZE, 0, (struct sockaddr *)&src_addr, &addrlen);
        if (nbytes < 0) {
            perror("Error receiving UDP packet");
            continue; // Continue listening
        }

        // Assuming the buffer contains a struct canfd_frame
        if (nbytes >= sizeof(struct can_frame)) { // Check minimum size
            struct canfd_frame *frame = (struct canfd_frame *)buf;
            char frame_str_buf[512]; // Buffer for formatted string

            // Use sprint_long_canframe to format the frame
            // Need to decide on view flags (e.g., 0 for default)
            // Need to determine max datalen (use CANFD_MAX_DLEN)
            sprint_long_canframe(frame_str_buf, frame, 0, CANFD_MAX_DLEN);

            // Get sender IP address
            char sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(src_addr.sin_addr), sender_ip, INET_ADDRSTRLEN);

            printf("Received from %s: %s\n", sender_ip, frame_str_buf);

        } else {
            fprintf(stderr, "Received undersized packet (%zd bytes)\n", nbytes);
        }
    }

    /* Unreachable in this simple example, but good practice: */
    // setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    // close(s);
    return 0;
} 