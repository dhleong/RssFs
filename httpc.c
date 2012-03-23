#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "httpc.h"

/** TODO set errno on fail */
int
httpc_open(const char *url) {
    char *port = "80"; // TODO we could offer a version that specifies port
                        // We may also want to parse the host for
                        // port specification, eg: HOST:PORT
    int sfd, s, i, cnt;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char buffer[256], host[128];

    // extract the hostname
    memset(&host, 0, sizeof(host));
    if (url[4] == 's') {
        fprintf(stderr, "SSL unsupported");
        return -1;
    }
    url += 7; // skip past http://
    for (i=0; i < 128; i++) {
        if (url[i] == '/' || url[i] == 0)
            break;
    }
    strncpy(host, url, i);

    // build the request line
    memset(&buffer, 0, sizeof(buffer));
    if (url[i] != '/') {
        cnt = sprintf(buffer, "GET / HTTP/1.0\r\n\r\n");
    } else {
        cnt = snprintf(buffer, 256, "GET %s HTTP/1.0\r\n\r\n", url + i);
    }

    if (cnt < 0) {
        fprintf(stderr, "Couldn't format header?\n");
        return -1; // shouldn't happen, but... you know
    }
    
    // obtain address(es) matching host/port 
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // allow IPv4 or IPv6 
    hints.ai_socktype = SOCK_STREAM; // tcp socket 
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP; // tcp please

    s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd); // couldn't connect; close it
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        return -1;
    }

    freeaddrinfo(result);           /* No longer needed */
    
    // write the request
    write(sfd, buffer, cnt);

    // read the response status line
    memset(&buffer, 0, sizeof(buffer));
    cnt = read(sfd, buffer, 16); // enough for an OK response line
    if (cnt < 0) {
        fprintf(stderr, "Couldn't read from server\n");
        return -1;
    }

    // make sure it's actually an HTTP response
    if (strncmp("HTTP", buffer, 4)) {
        fprintf(stderr, "Not an HTTP server...\n");
        fprintf(stderr, "(got: %s)\n", buffer);
        return -1;
    }

    // make sure the status code is 200 OK
    // actually we're being lazy, but should be fine
    if (buffer[8] == ' ' && buffer[9] != '2') {
        fprintf(stderr, "Error response: %s\n", buffer);
        return -1;
    }

    // read until end of headers
    //  somewhat hack-ish, but it'll work....
    // nicer would be to read into some external
    //  buffer the we can pull from in httpc_read
    //  before continuing along the stream
    cnt = 0;
    while (read(sfd, buffer, 1) > 0) {
        if (buffer[0] == '\r' || buffer[0] == '\n') {
            if (++cnt == 4)
                break;

        } else cnt = 0;
    }
        
    
    return sfd;
}

int
httpc_read(int conn, char *buffer, int count) {
    // easy :)
    return read(conn, buffer, count);
}

void
httpc_close(int conn) {
    // even easier :)
    close(conn);
}
