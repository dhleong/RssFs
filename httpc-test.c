#include <stdio.h>
#include "httpc.h"

int
main(int argc, char **argv) {
    char buffer[1024];
    int fp = httpc_open("http://whatismyip.org");
    if (fp < 0) {
        printf("Couldn't connect to host\n");
        return 1;
    }

    if (httpc_read(fp, buffer, sizeof(buffer)) < 0) {
        printf("Couldn't read from server\n");
        return 1;
    }

    printf("read:%s\n", buffer);

    httpc_close(fp);
}
