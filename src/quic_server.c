#include <ngtcp2/ngtcp2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  printf("Server");

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

  int ret = ngtcp2_conn_server_new(&conn, &dcid, &scid, NULL, 0, NULL, sett,
                                   NULL, NULL, NULL);

  return 0;
}
