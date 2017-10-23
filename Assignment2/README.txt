# HTTP based TCP Web-Server

A web server is a program that receives requests from a client and sends the
processed result back to the client as a response to the command.
A client is usually a user trying to access the server using a web browser
(Google Chrome, Firefox).

The project directory has 3 files: webserver.c makefile ws.conf and
1 sub-directory www. The contents on these are as follows:

webserver.c: Code file for the project
makefile: Make recipe for the code
ws.conf: Server configuration file
www: Directory having the server website contents

To start the server first run:
$make   #then,
$./webserver    #without any parameters

The webserver will fetch the port, directory root and default index from the
ws.conf file

The HTTP methods implemented are:
1. GET - This method is used to fetch files from the server and display them on the browser
    To test this use $(echo -en "GET / HTTP/1.1")|nc 127.0.0.1 port

2. POST - This method is used to post files to the server
    To test this use $(echo -en "POST / HTTP/1.1\n\nPOST-DATA")|nc 127.0.01 port

3. HEAD, DELETE, OPTIONS will return 501 Not Implemented error

4. Any other request will return 400 Bad Request Error


Error Handling:

1. 400 Bad Request
    The response Status Code “400” represents that the request message is in a
    wrong format and hence it cannot be responded by the server.
    It can handle Invalid Method, Invalid URL and Invalid HTTP-Version

2. 404 Not Found
    The response Status Code “404” informs the client that the requested URL
    doesn’t exist (file does not exist in the document root)

3. 501 Not Implemented
    The response Status Code “501” represents that the requested file type is
    not supported by the server and thus it cannot be delivered to the client.

4. 500 Internal Server error
    The response Status Code "500" informs the client that there was some
    unexpected error in the server and the it is not the client's fault. The
    client may try again with the exact same request
