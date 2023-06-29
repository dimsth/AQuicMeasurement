#include <ngtcp2/ngtcp2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Callback function for processing received messages*/
int on_recv_callback(ngtcp2_conn *conn, uint32_t flags, int64_t stream_id,
                     uint64_t offset, const uint8_t *data, size_t datalen,
                     void *user_data, void *stream_user_data) {

  /* Cast the user_data pointer to the desired type */
  int *client_fd = (int *)user_data;

  /* Process the received message */
  printf("Received message: %s\n", data);

  return 0;
}

int main(int argc, char **argv) {
  printf("Server");

  ngtcp2_callbacks server_cb;
  memset(&server_cb, 0, sizeof(ngtcp2_callbacks));
  server_cb.recv_stream_data = on_recv_callback;

  ngtcp2_settings *sett;
  ngtcp2_settings_default(sett);

  ngtcp2_transport_params *param;
  ngtcp2_transport_params_default(param);

  const ngtcp2_mem *mem = ngtcp2_mem_default();

  return 0;
}
