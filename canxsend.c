#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h> // For perror

/* Network specific header files */
// #include <net/if.h>      // Not needed for UDP multicast sending
// #include <sys/ioctl.h>   // Not needed for UDP multicast sending
#include <sys/socket.h>
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h>  // For inet_addr

#include "x/can/lib/can.h" // Keep for parsing
#include "x/can/lib.h"     // Keep for parsing

#define MULTICAST_GRP "239.192.1.1"
#define MULTICAST_PORT 55555

int main(int argc, char **argv)
{
    int s; /* UDP socket */
    int required_mtu;
    // int mtu; // Not relevant for UDP sending logic as implemented
    // int enable_canfd = 1; // Not relevant for UDP sending logic

    /* UDP Multicast address */
    struct sockaddr_in mcast_addr;

    /* CAN frame storage */
    struct canfd_frame frame;
    // struct ifreq ifr; // Not needed

    /* Check arguments */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <dummy_iface_ignored> <can_frame>\n", argv[0]);
        fprintf(stderr, "Sends CAN frame via UDP multicast to %s:%d\n", MULTICAST_GRP, MULTICAST_PORT);
        return 1;
    }

    /* Parse CAN frame using existing lib function */
    required_mtu = parse_canframe(argv[2], &frame);
    if (!required_mtu) {
        fprintf(stderr, "\nWrong CAN-frame format!\n\n");
        // It's better to exit if parsing fails
        return 1;
    }
    // Add null termination check for frame data if needed by subsequent code,
    // but for raw sending, it's not strictly necessary here.

    /* Create UDP socket */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating UDP socket");
        return 1;
    }

    /* Set up destination address structure */
    memset(&mcast_addr, 0, sizeof(mcast_addr));
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_addr.s_addr = inet_addr(MULTICAST_GRP);
    mcast_addr.sin_port = htons(MULTICAST_PORT);

    /* Calculate the actual size of the frame to send */
    /* Note: This sends the whole struct, including potential padding.
       A more robust method would serialize fields explicitly. */
    size_t frame_size_to_send;
    if (frame.flags & CANFD_BRS) { // Assuming CANFD_BRS implies potential 64 byte payload
         // Size calculation should ideally use can_dlc2len from your lib if adapted,
         // or just calculate based on frame.len
         frame_size_to_send = sizeof(frame.can_id) + sizeof(frame.len) + sizeof(frame.flags) + frame.len;
         // Ensure we don't exceed the struct size (though frame.len should handle this)
         frame_size_to_send = (frame_size_to_send > sizeof(struct canfd_frame)) ? sizeof(struct canfd_frame) : frame_size_to_send;

    } else {
        // Standard CAN 2.0 frame
         frame_size_to_send = sizeof(frame.can_id) + sizeof(frame.len) + sizeof(frame.flags) + frame.len;
         // Ensure we don't exceed the standard frame part of the struct
         // offsetof might be safer, but this assumes layout
         size_t max_standard_size = sizeof(struct can_frame); // Assuming can_frame represents standard part
         frame_size_to_send = (frame_size_to_send > max_standard_size) ? max_standard_size : frame_size_to_send;
    }
     // Simpler fixed-size approach for now (sending the whole struct):
     frame_size_to_send = sizeof(struct canfd_frame);


    /* Send the CAN frame data */
    if (sendto(s, &frame, frame_size_to_send, 0,
               (struct sockaddr *)&mcast_addr, sizeof(mcast_addr)) < 0) {
        perror("Error sending UDP multicast packet");
        close(s);
        return 1;
    }

    printf("CAN frame sent via UDP multicast to %s:%d\n", MULTICAST_GRP, MULTICAST_PORT);

    close(s);
    return 0; // Success
}
