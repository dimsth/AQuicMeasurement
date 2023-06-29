#include <ngtcp2/ngtcp2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  printf("Server");

  ngtcp2_settings *sett;
  ngtcp2_settings_default(sett);

  return 0;
}
