# DNS Server

This project implement a recursive DNS server. It is the authoritative server for a specified domain, 
and responsible for serving as a recursive resolver for a number of local clients.

## Implementation

- The `4700dns` program starts by parsing the given zone file and establishing a socket connection that listens to client requests.
- A `dns_resolver` function first attempts to handle the request within its authoritative domain. If the request is
within the domain, a response can be returned right away. Otherwise, it tries to look up the requested domain name 
in the cache to find a valid result. The recursive resolver is called only if it is not cached.
- To handle concurrent client requests, a `handle_request` function uses threading to process network I/O without blocking.
Any writes to the shared cache must be protected by a lock to ensure safety.


## Challenges

- This project relies heavily on `DNSlib` in Python to pack, unpack, and parse DNS messages.
While useful, it is also complex (multiple levels of inheritance) and poorly documented. Understanding 
the specification of this library took a lot of time.
- To handle multiple DNS requests in parallel is also challenging. After finishing the recursive resolver,
it was unclear to me how to handle parallel request in an event-driven way. Instead, I used multi-threading to 
achieve concurrency. A lock is used to protect shared data structures (in this case, the DNS record cache).


## Testing
This project was mainly tested against the configuration file provided as a part of the starter code. 
The log method, which outputs text to stderr, was very helpful to check intermediate results.
