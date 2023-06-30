#include <netinet/in.h>
#include <ngtcp2/ngtcp2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int main(int argc, char **argv) {
  printf("Server");

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* Set up server address */
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(12345);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* Bind the socket to the server address */
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  ngtcp2_settings *sett;
  ngtcp2_settings_default(sett);

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
      ngtcp2_conn_server_new(&conn, &dcid, &scid, NULL, NGTCP2_PROTO_VER_V1,
                             NULL, sett, NULL, NULL, NULL);

  if (ret) {
    printf("=====Out of memory=====");
  }

  return 0;
}
