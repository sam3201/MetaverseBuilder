#include "../utils/socketed/cite.h"
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        return 1;
    }
    
    init_cite_server(argv[1], argv[2]);
    start_cite_server();
    
    return 0;
}
