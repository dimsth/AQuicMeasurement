#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
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

  const ngtcp2_callbacks callbacks = {
      /* Use the default implementation from ngtcp2_crypto */
      .recv_client_initial = ngtcp2_crypto_recv_client_initial_cb,
      .recv_crypto_data = ngtcp2_crypto_recv_crypto_data_cb,
      .encrypt = ngtcp2_crypto_encrypt_cb,
      .decrypt = ngtcp2_crypto_decrypt_cb,
      .hp_mask = ngtcp2_crypto_hp_mask_cb,
      .recv_retry = ngtcp2_crypto_recv_retry_cb,
      .update_key = ngtcp2_crypto_update_key_cb,
      .delete_crypto_aead_ctx = ngtcp2_crypto_delete_crypto_aead_ctx_cb,
      .delete_crypto_cipher_ctx = ngtcp2_crypto_delete_crypto_cipher_ctx_cb,
      .get_path_challenge_data = ngtcp2_crypto_get_path_challenge_data_cb};

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
