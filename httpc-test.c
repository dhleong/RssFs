#include <stdio.h>
#include "httpc.h"

int
main(int argc, char **argv) {
    char buffer1[1024], buffer2[1024];
    int fp1 = httpc_open("http://whatismyip.org");
    int fp2 = httpc_open("http://whatismyip.org");

    if (fp1 < 0 || fp2 < 0) {
        printf("Couldn't connect to host\n");
        return 1;
    }

    // interleave the reads
    if (httpc_read(fp1, buffer1, 2) < 0) {
        printf("Couldn't read from server\n");
        return 1;
    }

    if (httpc_read(fp2, buffer2, 2) < 0) {
        printf("Couldn't read from server\n");
        return 1;
    }

    if (httpc_read(fp1, buffer1+2, sizeof(buffer1)) < 0) {
        printf("Couldn't read from server\n");
        return 1;
    }

    if (httpc_read(fp2, buffer2+2, sizeof(buffer2)) < 0) {
        printf("Couldn't read from server\n");
        return 1;
    }

    printf("Reads on two sockets interleaved\n");
    printf("The two values below should be identical:\n");
    printf("read1:\"%s\"\n", buffer1);
    printf("read2:\"%s\"\n", buffer2);

    httpc_close(fp1);
    httpc_close(fp2);
}
