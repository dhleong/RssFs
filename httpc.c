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
httpc_open(char *url) {
    /* Obtain address(es) matching host/port */
    char *port = "80"; // we could offer a version that specifies port
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
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* tcp socket */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;          

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

        close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        return -1;
    }

    freeaddrinfo(result);           /* No longer needed */
    
    // write the request
    int wrote = write(sfd, buffer, cnt);
    //printf("wrote %d\n", wrote);
    //printf(" ->``%s''\n", buffer);

    // read the response header
    memset(&buffer, 0, sizeof(buffer));
    cnt = read(sfd, buffer, 16); // enough for an OK response line
    if (cnt < 0) {
        fprintf(stderr, "Couldn't read from server\n");
        return -1;
    }

    if (strncmp("HTTP", buffer, 4)) {
        // not an HTTP response header!
        fprintf(stderr, "Not an HTTP server...\n");
        fprintf(stderr, "(got: %s)\n", buffer);
        return -1;
    }
    //printf("done\n");
    if (buffer[8] == ' ' && buffer[9] != '2') {
        fprintf(stderr, "Error response: %s\n", buffer);
        return -1;
    }

    // read until end of headers
    //  somewhat hack-ish, but it'll work
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
    close(conn);
}
