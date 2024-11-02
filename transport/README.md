# Reliable Transport Protocol

This project implements a simple transport protocol that provides reliable datagram service on top of UDP.

## Implementation
Per instruction, this project abstracts network transportation as a sender and a receiver. The sender transmits data 
that it reads from standard input, while the receiver writes the data to standard output, only sending an acknowledgement
message back.
- The sender maintains a sequence number, incremented each time it reads from input, and a hash map of unacknowledged packets 
that maps from sequence number to the data packet. When a packet is acknowledged by the receiver, it is removed from the 
outstanding hash map. When the number of unacknowledged packets reached the window size, the sender waits for ACKs from the receiver. 
- For congestion control, the sender dynamically adjusts the window size and slow start threshold. With each acknowledgement,
the congestion window size doubles until either a packet loss is detected, or slow start threshold is reached.
If a packet is lost, the sender sets a half of the current congestion window as `ssthresh` and as new `cwnd`, mimicking 
the congestion avoidance algorithm of TCP Reno.
- The receiver maintains a set of sequence number to detect duplicate packets, a priority queue to store out-of-order
packets, and an expected sequence number that should be received subsequently. If the packet received is corrupt or duplicate
(already in the sequence number set), the receiver drop the packet without further processing. If the packet has a sequence number
that is larger than the expected number, the whole packet is pushed to the priority queue, the root of which always has 
the smallest sequence number. A `process_heap` function is called in each iteration so that packets in the correct order
can be retrieved with efficiency.

## Challenges
- Design: There are many real-world TCP implementations that I can learn from. Since this project is higher-level than
the network transport layer, I used a hash function instead of checksum to detect corruption, which in turn can be easily
added to the JSON object (rather than a struct-like object).
- Another challenge was to handle the interleave of stdin and socket descriptor. After introducing
a sliding window, somewhat surprisingly, this part of implementation resulted in quite a few bugs that were difficult to trace.

## Testing
The project was mainly tested against the configuration file provided as a part of the starter code. The `log` method, which 
outputs text to stderr, was very helpful to compare packets on the sender and receiver's ends.
