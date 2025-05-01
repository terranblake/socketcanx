# SocketCANx - Virtual CAN Pipe for macOS

This project provides a simple userspace simulation of a CAN broadcast bus on macOS, similar in concept to Linux's `vcan` virtual interface, but implemented using UDP multicast instead of kernel-level SocketCAN.

It allows multiple local processes to exchange CAN frames without requiring physical CAN hardware or native SocketCAN kernel support.

## Components

*   `canxsend`: A command-line tool to send a single CAN or CAN FD frame onto the virtual bus.
*   `canxrecv`: A command-line tool to listen for and display CAN/CAN FD frames received from the virtual bus.
*   `x/can/lib.c`: A library containing helper functions for parsing and printing CAN(FD) frames (originally intended for SocketCAN).
*   `x/can/*.h`: Header files defining CAN structures and constants (originally from Linux SocketCAN headers).

## How it Works

Since macOS lacks native SocketCAN support (the `PF_CAN` protocol family), this project uses standard UDP networking on the loopback interface:

1.  **UDP Multicast:** A specific multicast group (`239.192.1.1`) and port (`55555`) are used as the communication channel.
2.  **Sender (`canxsend`):**
    *   Parses a CAN frame string provided as a command-line argument (e.g., `123#DEADBEEF`).
    *   Opens a UDP socket.
    *   Sends the raw `struct canfd_frame` data as a UDP packet to the multicast group and port.
3.  **Receiver (`canxrecv`):**
    *   Opens a UDP socket.
    *   Sets the `SO_REUSEPORT` option, allowing multiple receivers to listen on the same port.
    *   Binds the socket to the multicast port (`55555`).
    *   Joins the multicast group (`239.192.1.1`).
    *   Listens for incoming UDP packets.
    *   When a packet arrives, it assumes the payload is a `struct canfd_frame`, casts it, and prints it using `sprint_long_canframe`.

This creates a one-to-many broadcast pipe entirely in userspace.

## Build Instructions

Ensure you have a C compiler like `gcc` or `clang` installed (standard on macOS with Xcode Command Line Tools).

```bash
make
```

This will compile `x/can/lib.c` into an object file (`x/can/lib.o`) and then build two executables: `canxsend` and `canxrecv`.

To clean the build artifacts:

```bash
make clean
```

## Usage Example

1.  **Open Terminal 1** and start the receiver:
    ```bash
    ./canxrecv
    # Output: Listening for CAN frames on UDP multicast group 239.192.1.1:55555...
    ```

2.  **Open Terminal 2** and send a standard CAN frame:
    ```bash
    ./canxsend ignored 123#DEADBEEF
    # Output: CAN frame sent via UDP multicast to 239.192.1.1:55555
    ```
    *(Terminal 1 should now show the received frame)*

3.  **Open Terminal 3** and send a CAN FD frame:
    ```bash
    ./canxsend ignored 456##1122334455667788
    # Output: CAN frame sent via UDP multicast to 239.192.1.1:55555
    ```
    *(Terminal 1 should now show the received FD frame)*

4.  You can run multiple `./canxrecv` instances simultaneously; all will receive the frames sent by `canxsend`.

*(Note: The first argument to `canxsend` is ignored in the current UDP implementation but was kept for compatibility with the original command structure.)*

## Limitations

*   **Not SocketCAN:** This does *not* provide the true SocketCAN API. Programs need to be written specifically for this UDP multicast mechanism (or adapted like `canxsend`/`canxrecv` were).
*   **Simple Serialization:** Sends the raw C `struct canfd_frame`. This is not robust against endianness differences or struct padding variations if used across different machine types (though generally safe on localhost).
*   **No Filtering:** The receiver gets all frames; there's no CAN ID filtering equivalent to SocketCAN filters.
*   **Basic Error Handling:** Error handling is minimal.