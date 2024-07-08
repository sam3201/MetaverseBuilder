#include "../utils/server.h"


int main(int argc, char **argv) {
  Server_t *server = init_server((char *)argv[1], (char *)argv[2]); 
  start_server(server);
  return 0;
}
