# Web Crawler

This project implements a basic web crawler that traverses a social network website called "Fakebook."
The goal is to find 5 "secret flags" that appear randomly in the website.

## Implementation
- The `Clawler` class is responsible for website login and traversal. After logging into the fakebook website,
it starts a breadth-first search to all the internal links in the webpages. The search terminates if either all five 
flags are found, or there is no more webpage to visit.
- The `send_get`, `send_post`, and `recv_webpage` methods in the `Clawler` class are used to send HTTP requests and 
receive response from the server. The receiver repeatedly invokes the `recv` system call until the entire webpage is downloaded,
and then pass the HTML document to the parser. The sender requests pages in the fakebook, echoing all cookies set by the server.
- The `MyHTMLParser` class inherits the `HTMLParser` class in the Python `html` library. It handles 
`<a>`, `<h3>` and `<input>` tags in the webpage feed, which can then be read by the crawler.

## Challenges
The basic concept and algorithm of the web crawler is straight-forward and easy to understand.
The primary challenge was implementing the specification, options, and formatting of the HTTP protocol.

## Testing
Testing the web crawler mainly relied on using print statements to display all HTTP responses from the server. 
Once the crawler found all five flags, I refactored the error handling code to enhance reliability.

## Reference
Python 3.13 official documentations in [TLS/SSL](https://docs.python.org/3/library/ssl.html) 
and [HTML Parser](https://docs.python.org/3/library/html.parser.html).
