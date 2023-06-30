#include <netinet/in.h>
#include <ngtcp2/ngtcp2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int main(int argc, char **argv) {
  printf("Client");

  int sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* Set up server address */
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_LOCAL;
  server_addr.sin_port = htons(12345);

  return 0;
}
