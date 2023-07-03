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

  assert(argc == 2);

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

  ngtcp2_settings sett;
  ngtcp2_settings_default(&sett);

  ngtcp2_transport_params param;
  ngtcp2_transport_params_default(&param);

  ngtcp2_conn *conn;
  ngtcp2_cid dcid;
  ngtcp2_cid scid;

  int dest_id = 1;
  int sour_id = 2;

  dcid.datalen = sizeof(dest_id);
  memcpy(dcid.data, &dest_id, dcid.datalen);

  scid.datalen = sizeof(sour_id);
  memcpy(scid.data, &sour_id, scid.datalen);

  int ret =
      ngtcp2_conn_client_new(&conn, &dcid, &scid, NULL, NGTCP2_PROTO_VER_V1,
                             NULL, &sett, &param, NULL, NULL);

  if (ret) {
    printf("=====Out of memory=====");
  }

  return 0;
}
