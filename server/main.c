#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(void)
{
  server_t server;

  init_server(&server);
  run_server(&server);
  destroy_server(&server);

  return 0;
}
