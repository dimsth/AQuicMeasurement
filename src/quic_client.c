#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <ngtcp2/ngtcp2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#define PORT 8800

int main(int argc, char **argv) {

  assert(argc == 1);

  printf("Client");

  printf("Server to connect: %s", argv[1]);

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* Set up server address */
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);

  /* Convert IPv4 and IPv6 addresses from text to binary form */
  if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  return 0;
}
