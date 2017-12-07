# HTTP Proxy Server

HTTP is a client-server protocol; the client usually connects directly with the
server. But some time it is useful to introduce an intermediate entity called proxy.
The proxy sits between HTTP client and HTTP server. When the HTTP client requests
resource from HTTP server, it sends the request to the Proxy, the Proxy then
forwards the request to the HTTP server, and then the Proxy receives reply from
the HTTP server, and finally the Proxy sends this reply to the HTTP client.

The project directory has the following files:
1. webproxy.c: Code file for the project
2. makefile:   Make recipe for the code
3. DNScache:   File having the cached IP address for the visited websites
4. BlockedList:File having the list of the blocked websites and IP addresses

To start the proxy server, run
$make   #then,
$./webproxy <PORT> <timeout_in_seconds>

The proxy server will start using the port defined in the arguments and use the
timeout argument as the cached website expiration time.

Open the browser to be used and change the proxy settings under advanced settings
Set proxy IP address as 127.0.0.1 and the port as defined above


The features implemented are:
- Multi-threaded server: The server supports multiple client requests. Only GET
    request is handled, thus only http:// websites are supported.

- Caching:
    1. Hostname IP address cache - If a IP address is resolved for a hostname,
        it is cached in the DNScache file. When the same hostname is requested
        again, the DNS query is skipped and IP address is served from cache

    2. Blocking websites - Websites and IP addresses listed in the BlockedList
        file, when tried to access, return a 403 Forbidden Error.

    3. Page Caching - When a website is retrieved from the remote server, a
        local cached copy is created. On accessing the same website, the cached
        copy is served instead of fetching it again from the remote server.

    4. Timeout Expiration - The 2nd runtime argument defines the time after
        which the cached page expires. Cached pages that are older than the
        timeout value will not be served and will be overwritten when the page
        is fetched from the remote server.

PS: Using openssl library, MD5 of the cached page's URL is used as the filename
for that webpage.
