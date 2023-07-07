#include <msquic.h>
#include <msquic_posix.h>
#include <stdint.h>
#include <stdio.h>

#define PORT 8800

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

QUIC_API_TABLE *Quic_api;
HQUIC configuration;

//
// The clients's callback for connection events from MsQuic.
//
_IRQL_requires_max_(DISPATCH_LEVEL)
    _Function_class_(QUIC_CONNECTION_CALLBACK) QUIC_STATUS QUIC_API
    ClientConnectionCallback(_In_ HQUIC Connection, _In_opt_ void *Context,
                             _Inout_ QUIC_CONNECTION_EVENT *Event) {
  UNREFERENCED_PARAMETER(Context);
  switch (Event->Type) {
  case QUIC_CONNECTION_EVENT_CONNECTED:
    //
    // The handshake has completed for the connection.
    //
    printf("[conn][%p] Connected\n", Connection);
    // ClientSend(Connection);
    break;
  case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
    //
    // The connection has been shut down by the transport. Generally, this
    // is the expected way for the connection to shut down with this
    // protocol, since we let idle timeout kill the connection.
    //
    if (Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status ==
        QUIC_STATUS_CONNECTION_IDLE) {
      printf("[conn][%p] Successfully shut down on idle.\n", Connection);
    } else {
      printf("[conn][%p] Shut down by transport, 0x%x\n", Connection,
             Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
    }
    break;
  case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
    //
    // The connection was explicitly shut down by the peer.
    //
    printf("[conn][%p] Shut down by peer, 0x%llu\n", Connection,
           (unsigned long long)Event->SHUTDOWN_INITIATED_BY_PEER.ErrorCode);
    break;
  case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
    //
    // The connection has completed the shutdown process and is ready to be
    // safely cleaned up.
    //
    printf("[conn][%p] All done\n", Connection);
    if (!Event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
      Quic_api->ConnectionClose(Connection);
    }
    break;
  case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED:
    //
    // A resumption ticket (also called New Session Ticket or NST) was
    // received from the server.
    //
    printf("[conn][%p] Resumption ticket received (%u bytes):\n", Connection,
           Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength);
    for (uint32_t i = 0;
         i < Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength; i++) {
      printf("%.2X",
             (uint8_t)Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicket[i]);
    }
    printf("\n");
    break;
  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
}

int main(int argc, char **argv) {

  QUIC_STATUS status;
  int err;
  const char *ResumptionTicketString = NULL;
  HQUIC conn = NULL;

  status = MsQuicOpen2(&Quic_api);

  if (status != QUIC_STATUS_SUCCESS) {
    printf("Failed to open msquic library.\n");
    err = 0;
    goto Error;
  }

  QUIC_REGISTRATION_CONFIG regConfig = {0};
  HQUIC reg;
  QUIC_SETTINGS settings = {0};
  status =
      ((QUIC_REGISTRATION_OPEN_FN)Quic_api->RegistrationOpen)(&regConfig, &reg);
  if (status != QUIC_STATUS_SUCCESS || reg == NULL) {
    printf("Failed to make QUIC_Registrations.\n");
    err = 1;
    goto Error;
  }

  QUIC_BUFFER buf = {sizeof("sample") - 1, (uint8_t *)"sample"};
  status = ((QUIC_CONFIGURATION_OPEN_FN)Quic_api->ConfigurationOpen)(
      reg, &buf, 1, &settings, sizeof(settings), NULL, &configuration);

  if (status != QUIC_STATUS_SUCCESS) {
    printf("Failed to make QUIC_Configuration.\n");
    err = 2;
    goto Error;
  }

  status = Quic_api->ConnectionOpen(reg, ClientConnectionCallback, NULL, &conn);

  if (status != QUIC_STATUS_SUCCESS) {
    printf("Failed to open Connection.\n");
    err = 3;
    goto Error;
  }

  printf("[conn][%p] Connecting...\n", conn);
  fflush(stdout);

  status = Quic_api->ConnectionStart(conn, configuration,
                                     QUIC_ADDRESS_FAMILY_UNSPEC, argv[1], PORT);
  if (status != QUIC_STATUS_SUCCESS) {
    printf("Failed to start Connection.\n");
    err = 4;
    goto Error;
  }

Error:

  if (err > 4)
    Quic_api->ConnectionShutdown(conn, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
  if (err > 3)
    Quic_api->ConnectionClose(conn);
  if (err > 2)
    Quic_api->ConfigurationClose(configuration);
  if (err > 1)
    Quic_api->RegistrationClose(reg);
  if (err >= 0)
    MsQuicClose(Quic_api);

  return 0;
}
