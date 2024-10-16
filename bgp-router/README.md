# BGP Router

This project simulates the behaviour of Border Gateway Protocol (BGP) router in Python. 
It handles BGP route update, withdraw, data messages, maintains a forwarding table using a trie data structure, 
and performs route aggregation and deaggregation when needed.

## Implementation
- The Router class handles the core functionality of a BGP router. Compared to the provided starter code, it maintains two additional data structure:
a routing table using a Trie object, and a history of update messages using a simple list.
- The `run` function uses the `select` system call to handle multiple sockets that our router connects to. When a message is received, it calls the corresponding handler of the message type.
- When an `update` or `withdraw` packet is received, the router processes the message, maintains the forwarding table, and propagate the message to the appropriate neighbors.
- When a data message is received, we find the best route to the given IP in the forwarding table (the Trie data structure), which allows finding the longest prefix in constant time.
- The `dump` message invokes a depth-first search to the binary Trie, which returns the entire forwarding table.
- The aggregation of forwarding table entries is handled by the Trie class.

## Challenges
- Efficient data structure: I tried using a hash map or a sorted list to maintain the forwarding table, both turned out to be messy and error-prone. 
Finding the longest prefix match gives me the hint of using Trie, which is much more efficient and easy to maintain.
- Withdrawal and deaggregation: Correctly handling route withdrawals, especially when routes have been aggregated, is difficult to keep track of.
I tried to add extra field in the TrieNode class to handle deaggregation, but it also turned out to be error-prone. Instead, the current implementation rebuild the entire routing table
when a withdrawal message is received. It is easy to implement, requires minimal code, but clearly not efficient.

## Testing
This project is mainly tested against the configuration file provided as a part of the starter code. It offers clear logging information 
for each error, which is helpful for debugging. Sometimes, adding a call to `dump` also helps to trace any programming error.
