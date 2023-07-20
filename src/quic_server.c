#include <msquic.h>
#include <stdint.h>
#include <stdio.h>

#define PORT 8800

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

QUIC_API_TABLE *Quic_api;
HQUIC configuration;

typedef struct QUIC_CREDENTIAL_CONFIG_HELPER {
  QUIC_CREDENTIAL_CONFIG CredConfig;
  union {
    QUIC_CERTIFICATE_HASH CertHash;
    QUIC_CERTIFICATE_HASH_STORE CertHashStore;
    QUIC_CERTIFICATE_FILE CertFile;
    QUIC_CERTIFICATE_FILE_PROTECTED CertFileProtected;
  };
} QUIC_CREDENTIAL_CONFIG_HELPER;

_IRQL_requires_max_(DISPATCH_LEVEL)
    _Function_class_(QUIC_CONNECTION_CALLBACK) QUIC_STATUS QUIC_API
    ServerConnectionCallback(_In_ HQUIC Connection, _In_opt_ void *Context,
                             _Inout_ QUIC_CONNECTION_EVENT *Event) {
  UNREFERENCED_PARAMETER(Context);
  switch (Event->Type) {
  case QUIC_CONNECTION_EVENT_CONNECTED:
    //
    // The handshake has completed for the connection.
    //
    printf("[conn][%p] Connected\n", Connection);
    Quic_api->ConnectionSendResumptionTicket(
        Connection, QUIC_SEND_RESUMPTION_FLAG_NONE, 0, NULL);
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
    Quic_api->ConnectionClose(Connection);
    break;
  case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
    //
    // The peer has started/created a new stream. The app MUST set the
    // callback handler before returning.
    //
    printf("[strm][%p] Peer started\n", Event->PEER_STREAM_STARTED.Stream);
    // Quic_api->SetCallbackHandler(Event->PEER_STREAM_STARTED.Stream,
    // (void*)ServerStreamCallback, NULL);
    break;
  case QUIC_CONNECTION_EVENT_RESUMED:
    //
    // The connection succeeded in doing a TLS resumption of a previous
    // connection's session.
    //
    printf("[conn][%p] Connection resumed!\n", Connection);
    break;
  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
}

//
// The server's callback for listener events from MsQuic.
//
_IRQL_requires_max_(PASSIVE_LEVEL)
    _Function_class_(QUIC_LISTENER_CALLBACK) QUIC_STATUS QUIC_API
    ServerListenerCallback(_In_ HQUIC Listener, _In_opt_ void *Context,
                           _Inout_ QUIC_LISTENER_EVENT *Event) {
  UNREFERENCED_PARAMETER(Listener);
  UNREFERENCED_PARAMETER(Context);
  QUIC_STATUS Status = QUIC_STATUS_NOT_SUPPORTED;
  switch (Event->Type) {
  case QUIC_LISTENER_EVENT_NEW_CONNECTION:
    //
    // A new connection is being attempted by a client. For the handshake to
    // proceed, the server must provide a configuration for QUIC to use. The
    // app MUST set the callback handler before returning.
    //
    Quic_api->SetCallbackHandler(Event->NEW_CONNECTION.Connection,
                                 (void *)ServerConnectionCallback, NULL);
    Status = Quic_api->ConnectionSetConfiguration(
        Event->NEW_CONNECTION.Connection, configuration);
    break;
  default:
    break;
  }
  return Status;
}

int main(int argc, char **argv) {
  QUIC_STATUS status;
  int err;

  const char *Key = "../../cert/server.key";
  const char *Cert = "../../cert/server.cert";

  status = MsQuicOpen2(&Quic_api);

  if (status != QUIC_STATUS_SUCCESS) {
    printf("Failed to open msquic library.\n");
    err = 0;
    goto Error;
  }

  // Create the configuration
  QUIC_REGISTRATION_CONFIG regConfig = {0};
  HQUIC registration;
  status = ((QUIC_REGISTRATION_OPEN_FN)Quic_api->RegistrationOpen)(
      &regConfig, &registration);
  if (status != QUIC_STATUS_SUCCESS || registration == NULL) {
    printf("Failed to make QUIC_Registrations.\n");
    err = 1;
    goto Error;
  }

  QUIC_SETTINGS settings = {0};
  settings.IdleTimeoutMs = 1000;
  settings.IsSet.IdleTimeoutMs = TRUE;

  settings.ServerResumptionLevel = QUIC_SERVER_RESUME_AND_ZERORTT;
  settings.IsSet.ServerResumptionLevel = TRUE;

  settings.PeerBidiStreamCount = 1;
  settings.IsSet.PeerBidiStreamCount = TRUE;

  QUIC_BUFFER buf = {sizeof("sample") - 1, (uint8_t *)"sample"};
  status = ((QUIC_CONFIGURATION_OPEN_FN)Quic_api->ConfigurationOpen)(
      registration, &buf, 1, &settings, sizeof(settings), NULL, &configuration);

  if (status != QUIC_STATUS_SUCCESS) {
    printf("Failed to make QUIC_Configuration.\n");
    err = 2;
    goto Error;
  }

  QUIC_CREDENTIAL_CONFIG_HELPER Config;
  memset(&Config, 0, sizeof(Config));
  Config.CredConfig.Flags = QUIC_CREDENTIAL_FLAG_NONE;
  Config.CertFile.CertificateFile = (char *)Cert;
  Config.CertFile.PrivateKeyFile = (char *)Key;
  Config.CredConfig.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
  Config.CredConfig.CertificateFile = &Config.CertFile;

  status =
      Quic_api->ConfigurationLoadCredential(configuration, &Config.CredConfig);

  if (status != QUIC_STATUS_SUCCESS) {
    printf("Failed to load Configuration.\n Make sure that in the folder cert/ "
           "there is both files server.key and server.cert");
    err = 2;
    goto Error;
  }

  // Create the listener
  HQUIC listener;
  status = ((QUIC_LISTENER_OPEN_FN)Quic_api->ListenerOpen)(
      registration, ServerListenerCallback, NULL, &listener);

  if (status != QUIC_STATUS_SUCCESS) {
    printf("Failed to open listener.\n");
    err = 3;
    goto Error;
  }

  QUIC_ADDR addr = {0};
  addr.Ipv4.sin_family = QUIC_ADDRESS_FAMILY_UNSPEC;
  addr.Ipv4.sin_port = htons(PORT);

  status = ((QUIC_LISTENER_START_FN)Quic_api->ListenerStart)(listener, &buf, 1,
                                                             &addr);

  if (status != QUIC_STATUS_SUCCESS) {
    printf("Failed to start listener.\n");
    err = 4;
    goto Error;
  }
  // Wait for a key press to exit
  printf("Server running. Press enter to exit...\n");
  getchar();
  err = 5;
  // Cleanup and exit
Error:

  if (err > 4)
    Quic_api->ListenerStop(listener);
  if (err > 3)
    Quic_api->ListenerClose(listener);
  if (err > 2)
    Quic_api->ConfigurationClose(configuration);
  if (err > 1)
    Quic_api->RegistrationClose(registration);
  if (err >= 0)
    MsQuicClose(Quic_api);

  return 0;
}
