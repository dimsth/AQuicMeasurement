#include "msquic.h"
#include <arpa/inet.h>
#include <msquic_posix.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

#define PORT 5678
#define IDLE_TIMEOUT 1000
//
// The (optional) registration configuration for the app. This sets a name for
// the app (used for persistent storage and for debugging). It also configures
// the execution profile, using the default "low latency" profile.
//
const QUIC_REGISTRATION_CONFIG RegConfig = {"quic_datag",
                                            QUIC_EXECUTION_PROFILE_LOW_LATENCY};

//
// The protocol name used in the Application Layer Protocol Negotiation (ALPN).
//
const QUIC_BUFFER Alpn = {sizeof("sample") - 1, (uint8_t *)"sample"};

//
// The length of buffer sent over the streams in the protocol.
//

//
// The QUIC API/function table returned from MsQuicOpen2. It contains all the
// functions called by the app to interact with MsQuic.
//
const QUIC_API_TABLE *QuicApi;

//
// The QUIC handle to the registration object. This is the top level API object
// that represents the execution context for all work done by MsQuic on behalf
// of the app.
//
HQUIC Registration;

//
// The QUIC handle to the configuration object. This object abstracts the
// connection configuration. This includes TLS configuration and any other
// QUIC layer settings.
//
HQUIC Configuration;

unsigned int num_of_msgs = 0;

unsigned int size_of_msgs = 0;

void PrintUsage() {
  printf("\n"
         "quic connection using datagram runs client or server.\n"
         "\n"
         "Usage:\n"
         "\n"
         "  ./q_datag -client -unsecure -target:{IPAddress} "
         "-num_of_msgs:{Number} -size_of_msgs:{Number}\n"
         "  ./q_datag -server -cert_file:<...> -key_file:<...> \n");
}

//
// Helper functions to look up a command line arguments.
//
BOOLEAN
GetFlag(_In_ int argc, _In_reads_(argc) _Null_terminated_ char *argv[],
        _In_z_ const char *name) {
  const size_t nameLen = strlen(name);
  for (int i = 0; i < argc; i++) {
    if (_strnicmp(argv[i] + 1, name, nameLen) == 0 &&
        strlen(argv[i]) == nameLen + 1) {
      return TRUE;
    }
  }
  return FALSE;
}

_Ret_maybenull_ _Null_terminated_ const char *
GetValue(_In_ int argc, _In_reads_(argc) _Null_terminated_ char *argv[],
         _In_z_ const char *name) {
  const size_t nameLen = strlen(name);
  for (int i = 0; i < argc; i++) {
    if (_strnicmp(argv[i] + 1, name, nameLen) == 0 &&
        strlen(argv[i]) > 1 + nameLen + 1 && *(argv[i] + 1 + nameLen) == ':') {
      return argv[i] + 1 + nameLen + 1;
    }
  }
  return NULL;
}

//
// Helper function to convert a hex character to its decimal value.
//
uint8_t DecodeHexChar(_In_ char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return 10 + c - 'A';
  if (c >= 'a' && c <= 'f')
    return 10 + c - 'a';
  return 0;
}

//
// Helper function to convert a string of hex characters to a byte buffer.
//
uint32_t DecodeHexBuffer(_In_z_ const char *HexBuffer,
                         _In_ uint32_t OutBufferLen,
                         _Out_writes_to_(OutBufferLen, return)
                             uint8_t *OutBuffer) {
  uint32_t HexBufferLen = (uint32_t)strlen(HexBuffer) / 2;
  if (HexBufferLen > OutBufferLen) {
    return 0;
  }

  for (uint32_t i = 0; i < HexBufferLen; i++) {
    OutBuffer[i] = (DecodeHexChar(HexBuffer[i * 2]) << 4) |
                   DecodeHexChar(HexBuffer[i * 2 + 1]);
  }

  return HexBufferLen;
}

//
// Allocates and sends some data over a QUIC stream.
//
void ServerSend(_In_ HQUIC Stream) {
  //
  // Allocates and builds the buffer to send over the stream.
  //
  void *SendBufferRaw = malloc(sizeof(QUIC_BUFFER) + size_of_msgs);
  if (SendBufferRaw == NULL) {
    printf("SendBuffer allocation failed!\n");
    QuicApi->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
    return;
  }
  QUIC_BUFFER *SendBuffer = (QUIC_BUFFER *)SendBufferRaw;
  SendBuffer->Buffer = (uint8_t *)SendBufferRaw + sizeof(QUIC_BUFFER);
  SendBuffer->Length = size_of_msgs;

  printf("[strm][%p] Sending data...\n", Stream);

  //
  // Sends the buffer over the stream. Note the FIN flag is passed along with
  // the buffer. This indicates this is the last buffer on the stream and the
  // the stream is shut down (in the send direction) immediately after.
  //
  QUIC_STATUS Status;
  if (QUIC_FAILED(Status = QuicApi->StreamSend(
                      Stream, SendBuffer, 1, QUIC_SEND_FLAG_FIN, SendBuffer))) {
    printf("StreamSend failed, 0x%x!\n", Status);
    free(SendBufferRaw);
    QuicApi->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
  }
}

//
// The server's callback for stream events from MsQuic.
//
_IRQL_requires_max_(DISPATCH_LEVEL)
    _Function_class_(QUIC_STREAM_CALLBACK) QUIC_STATUS QUIC_API
    ServerStreamCallback(_In_ HQUIC Stream, _In_opt_ void *Context,
                         _Inout_ QUIC_STREAM_EVENT *Event) {
  UNREFERENCED_PARAMETER(Context);
  switch (Event->Type) {
  case QUIC_STREAM_EVENT_SEND_COMPLETE:
    //
    // A previous StreamSend call has completed, and the context is being
    // returned back to the app.
    //
    free(Event->SEND_COMPLETE.ClientContext);
    printf("[strm][%p] Data sent\n", Stream);
    break;
  case QUIC_STREAM_EVENT_RECEIVE:
    //
    // Data was received from the peer on the stream.
    //
    printf("[strm][%p] Data received\n", Stream);
    break;
  case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    //
    // The peer gracefully shut down its send direction of the stream.
    //
    printf("[strm][%p] Peer shut down\n", Stream);
    ServerSend(Stream);
    break;
  case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
    //
    // The peer aborted its send direction of the stream.
    //
    printf("[strm][%p] Peer aborted\n", Stream);
    QuicApi->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
    break;
  case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
    //
    // Both directions of the stream have been shut down and MsQuic is done
    // with the stream. It can now be safely cleaned up.
    //
    printf("[strm][%p] All done\n", Stream);
    QuicApi->StreamClose(Stream);
    break;
  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
}

//
// The server's callback for connection events from MsQuic.
//
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
    QuicApi->ConnectionSendResumptionTicket(
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
    QuicApi->ConnectionClose(Connection);
    break;
  case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
    //
    // The peer has started/created a new stream. The app MUST set the
    // callback handler before returning.
    //
    printf("[strm][%p] Peer started\n", Event->PEER_STREAM_STARTED.Stream);
    QuicApi->SetCallbackHandler(Event->PEER_STREAM_STARTED.Stream,
                                (void *)ServerStreamCallback, NULL);
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
    QuicApi->SetCallbackHandler(Event->NEW_CONNECTION.Connection,
                                (void *)ServerConnectionCallback, NULL);
    Status = QuicApi->ConnectionSetConfiguration(
        Event->NEW_CONNECTION.Connection, Configuration);
    break;
  default:
    break;
  }
  return Status;
}

typedef struct QUIC_CREDENTIAL_CONFIG_HELPER {
  QUIC_CREDENTIAL_CONFIG CredConfig;
  union {
    QUIC_CERTIFICATE_HASH CertHash;
    QUIC_CERTIFICATE_HASH_STORE CertHashStore;
    QUIC_CERTIFICATE_FILE CertFile;
    QUIC_CERTIFICATE_FILE_PROTECTED CertFileProtected;
  };
} QUIC_CREDENTIAL_CONFIG_HELPER;

//
// Helper function to load a server configuration. Uses the command line
// arguments to load the credential part of the configuration.
//
BOOLEAN
ServerLoadConfiguration(_In_ int argc,
                        _In_reads_(argc) _Null_terminated_ char *argv[]) {
  QUIC_SETTINGS Settings = {0};
  //
  // Configures the server's idle timeout.
  //
  Settings.IdleTimeoutMs = IDLE_TIMEOUT;
  Settings.IsSet.IdleTimeoutMs = TRUE;
  //
  // Configures the server's resumption level to allow for resumption and
  // 0-RTT.
  //
  Settings.ServerResumptionLevel = QUIC_SERVER_RESUME_AND_ZERORTT;
  Settings.IsSet.ServerResumptionLevel = TRUE;
  //
  // Configures the server's settings to allow for the peer to open a single
  // bidirectional stream. By default connections are not configured to allow
  // any streams from the peer.
  //
  Settings.PeerBidiStreamCount = 1;
  Settings.IsSet.PeerBidiStreamCount = TRUE;

  QUIC_CREDENTIAL_CONFIG_HELPER Config;
  memset(&Config, 0, sizeof(Config));
  Config.CredConfig.Flags = QUIC_CREDENTIAL_FLAG_NONE;

  const char *Cert;
  const char *KeyFile;

  if ((Cert = GetValue(argc, argv, "cert_file")) != NULL &&
      (KeyFile = GetValue(argc, argv, "key_file")) != NULL) {
    //
    // Loads the server's certificate from the file.
    //
    Config.CertFile.CertificateFile = (char *)Cert;
    Config.CertFile.PrivateKeyFile = (char *)KeyFile;
    Config.CredConfig.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
    Config.CredConfig.CertificateFile = &Config.CertFile;

  } else {
    printf("Must specify ['cert_file' and 'key_file']\n");
    return FALSE;
  }

  //
  // Allocate/initialize the configuration object, with the configured ALPN
  // and settings.
  //
  QUIC_STATUS Status = QUIC_STATUS_SUCCESS;
  if (QUIC_FAILED(Status = QuicApi->ConfigurationOpen(
                      Registration, &Alpn, 1, &Settings, sizeof(Settings), NULL,
                      &Configuration))) {
    printf("ConfigurationOpen failed, 0x%x!\n", Status);
    return FALSE;
  }

  //
  // Loads the TLS credential part of the configuration.
  //
  if (QUIC_FAILED(Status = QuicApi->ConfigurationLoadCredential(
                      Configuration, &Config.CredConfig))) {
    printf("ConfigurationLoadCredential failed, 0x%x!\n", Status);
    return FALSE;
  }

  return TRUE;
}

//
// Runs the server side of the protocol.
//
void RunServer(_In_ int argc, _In_reads_(argc) _Null_terminated_ char *argv[]) {
  QUIC_STATUS Status;
  HQUIC Listener = NULL;

  //
  // Configures the address used for the listener to listen on all IP
  // addresses and the given UDP port.
  //
  QUIC_ADDR Address = {0};
  Address.Ipv4.sin_family = QUIC_ADDRESS_FAMILY_UNSPEC;
  Address.Ipv4.sin_port = htons(PORT);

  //
  // Load the server configuration based on the command line.
  //
  if (!ServerLoadConfiguration(argc, argv)) {
    return;
  }

  //
  // Create/allocate a new listener object.
  //
  if (QUIC_FAILED(Status = QuicApi->ListenerOpen(
                      Registration, ServerListenerCallback, NULL, &Listener))) {
    printf("ListenerOpen failed, 0x%x!\n", Status);
    goto Error;
  }

  //
  // Starts listening for incoming connections.
  //
  if (QUIC_FAILED(Status =
                      QuicApi->ListenerStart(Listener, &Alpn, 1, &Address))) {
    printf("ListenerStart failed, 0x%x!\n", Status);
    goto Error;
  }

  //
  // Continue listening for connections until the Enter key is pressed.
  //
  printf("Press Enter to exit.\n\n");
  getchar();

Error:

  if (Listener != NULL) {
    QuicApi->ListenerClose(Listener);
  }
}
//==================================================Client==================================================
//
// The clients's callback for stream events from MsQuic.
//
_IRQL_requires_max_(DISPATCH_LEVEL)
    _Function_class_(QUIC_STREAM_CALLBACK) QUIC_STATUS QUIC_API
    ClientStreamCallback(_In_ HQUIC Stream, _In_opt_ void *Context,
                         _Inout_ QUIC_STREAM_EVENT *Event) {
  UNREFERENCED_PARAMETER(Context);
  switch (Event->Type) {
  case QUIC_STREAM_EVENT_SEND_COMPLETE:
    //
    // A previous StreamSend call has completed, and the context is being
    // returned back to the app.
    //
    free(Event->SEND_COMPLETE.ClientContext);
    printf("[strm][%p] Data sent\n", Stream);
    break;
  case QUIC_STREAM_EVENT_RECEIVE:
    //
    // Data was received from the peer on the stream.
    //
    printf("[strm][%p] Data received\n", Stream);
    break;
  case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
    //
    // The peer gracefully shut down its send direction of the stream.
    //
    printf("[strm][%p] Peer aborted\n", Stream);
    break;
  case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    //
    // The peer aborted its send direction of the stream.
    //
    printf("[strm][%p] Peer shut down\n", Stream);
    break;
  case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
    //
    // Both directions of the stream have been shut down and MsQuic is done
    // with the stream. It can now be safely cleaned up.
    //
    printf("[strm][%p] All done\n", Stream);
    if (!Event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
      QuicApi->StreamClose(Stream);
    }
    break;
  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
}

void ClientSend(_In_ HQUIC Connection, _In_ int msgs_num) {
  QUIC_STATUS Status;
  HQUIC Stream = NULL;
  uint8_t *SendBufferRaw;
  QUIC_BUFFER *SendBuffer;

  //
  // Create/allocate a new bidirectional stream. The stream is just allocated
  // and no QUIC stream identifier is assigned until it's started.
  //
  if (QUIC_FAILED(
          Status = QuicApi->StreamOpen(Connection, QUIC_STREAM_OPEN_FLAG_NONE,
                                       ClientStreamCallback, NULL, &Stream))) {
    printf("StreamOpen failed, 0x%x!\n", Status);
    goto Error;
  }

  printf("[strm][%p] Starting...\n", Stream);

  //
  // Starts the bidirectional stream. By default, the peer is not notified of
  // the stream being started until data is sent on the stream.
  //
  if (QUIC_FAILED(
          Status = QuicApi->StreamStart(Stream, QUIC_STREAM_START_FLAG_NONE))) {
    printf("StreamStart failed, 0x%x!\n", Status);
    QuicApi->StreamClose(Stream);
    goto Error;
  }

  //
  // Allocates and builds the buffer to send over the stream.
  //
  SendBufferRaw = (uint8_t *)malloc(sizeof(QUIC_BUFFER) + size_of_msgs);
  if (SendBufferRaw == NULL) {
    printf("SendBuffer allocation failed!\n");
    Status = QUIC_STATUS_OUT_OF_MEMORY;
    goto Error;
  }
  SendBuffer = (QUIC_BUFFER *)SendBufferRaw;
  SendBuffer->Buffer = SendBufferRaw + sizeof(QUIC_BUFFER);
  SendBuffer->Length = size_of_msgs;

  printf("[strm][%p] Sending data...\n", Stream);

  //
  // Sends the buffer over the stream. Note the FIN flag is passed along with
  // the buffer. This indicates this is the last buffer on the stream and the
  // the stream is shut down (in the send direction) immediately after.
  //
  if (msgs_num == 0)
    Status = QuicApi->StreamSend(Stream, SendBuffer, 1,
                                 QUIC_SEND_FLAG_ALLOW_0_RTT, SendBuffer);
  else if (msgs_num == num_of_msgs - 1)
    Status = QuicApi->StreamSend(Stream, SendBuffer, 1, QUIC_SEND_FLAG_FIN,
                                 SendBuffer);
  else
    Status = QuicApi->StreamSend(Stream, SendBuffer, 1, QUIC_SEND_FLAG_NONE,
                                 SendBuffer);

  if (QUIC_FAILED(Status)) {
    printf("StreamSend failed, 0x%x!\n", Status);
    free(SendBufferRaw);
    goto Error;
  }

Error:

  if (QUIC_FAILED(Status)) {
    QuicApi->ConnectionShutdown(Connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE,
                                0);
  }
}

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
    for (int i = 0; i < num_of_msgs; i++)
      ClientSend(Connection, i);
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
      QuicApi->ConnectionClose(Connection);
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

//
// Helper function to load a client configuration.
//
BOOLEAN
ClientLoadConfiguration(BOOLEAN Unsecure) {
  QUIC_SETTINGS Settings = {0};
  //
  // Configures the client's idle timeout.
  //
  Settings.IdleTimeoutMs = IDLE_TIMEOUT;
  Settings.IsSet.IdleTimeoutMs = TRUE;

  //
  // Configures a default client configuration, optionally disabling
  // server certificate validation.
  //
  QUIC_CREDENTIAL_CONFIG CredConfig;
  memset(&CredConfig, 0, sizeof(CredConfig));
  CredConfig.Type = QUIC_CREDENTIAL_TYPE_NONE;
  CredConfig.Flags = QUIC_CREDENTIAL_FLAG_CLIENT;
  if (Unsecure) {
    CredConfig.Flags |= QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;
  }

  //
  // Allocate/initialize the configuration object, with the configured ALPN
  // and settings.
  //
  QUIC_STATUS Status = QUIC_STATUS_SUCCESS;
  if (QUIC_FAILED(Status = QuicApi->ConfigurationOpen(
                      Registration, &Alpn, 1, &Settings, sizeof(Settings), NULL,
                      &Configuration))) {
    printf("ConfigurationOpen failed, 0x%x!\n", Status);
    return FALSE;
  }

  //
  // Loads the TLS credential part of the configuration. This is required even
  // on client side, to indicate if a certificate is required or not.
  //
  if (QUIC_FAILED(Status = QuicApi->ConfigurationLoadCredential(Configuration,
                                                                &CredConfig))) {
    printf("ConfigurationLoadCredential failed, 0x%x!\n", Status);
    return FALSE;
  }

  return TRUE;
}

//
// Runs the client side of the protocol.
//
void RunClient(_In_ int argc, _In_reads_(argc) _Null_terminated_ char *argv[]) {
  //
  // Load the client configuration based on the "unsecure" command line option.
  //
  if (!ClientLoadConfiguration(GetFlag(argc, argv, "unsecure"))) {
    return;
  }

  QUIC_STATUS Status;
  const char *ResumptionTicketString = NULL;
  HQUIC Connection = NULL;

  const char *nom;
  if ((nom = GetValue(argc, argv, "num_of_msgs")) == NULL) {
    printf("Must specify '-num_of_msgs' argument!\n");
    Status = QUIC_STATUS_INVALID_PARAMETER;
    goto Error;
  }
  num_of_msgs = atoi(nom);

  const char *som;
  if ((som = GetValue(argc, argv, "size_of_msgs")) == NULL) {
    printf("Must specify '-size_of_msgs' argument!\n");
    Status = QUIC_STATUS_INVALID_PARAMETER;
    goto Error;
  }
  size_of_msgs = atoi(som);

  //
  // Allocate a new connection object.
  //
  if (QUIC_FAILED(Status = QuicApi->ConnectionOpen(Registration,
                                                   ClientConnectionCallback,
                                                   NULL, &Connection))) {
    printf("ConnectionOpen failed, 0x%x!\n", Status);
    goto Error;
  }

  //
  // Get the target / server name or IP from the command line.
  //
  const char *Target;
  if ((Target = GetValue(argc, argv, "target")) == NULL) {
    printf("Must specify '-target' argument!\n");
    Status = QUIC_STATUS_INVALID_PARAMETER;
    goto Error;
  }
  printf("[conn][%p] Connecting...\n", Connection);

  //
  // Start the connection to the server.
  //
  if (QUIC_FAILED(Status = QuicApi->ConnectionStart(Connection, Configuration,
                                                    QUIC_ADDRESS_FAMILY_UNSPEC,
                                                    Target, PORT))) {
    printf("ConnectionStart failed, 0x%x!\n", Status);
    goto Error;
  }

Error:

  if (QUIC_FAILED(Status) && Connection != NULL) {
    QuicApi->ConnectionClose(Connection);
  }
}

int QUIC_MAIN_EXPORT main(_In_ int argc,
                          _In_reads_(argc) _Null_terminated_ char *argv[]) {
  QUIC_STATUS Status = QUIC_STATUS_SUCCESS;

  //
  // Open a handle to the library and get the API function table.
  //
  if (QUIC_FAILED(Status = MsQuicOpen2(&QuicApi))) {
    printf("MsQuicOpen2 failed, 0x%x!\n", Status);
    goto Error;
  }

  //
  // Create a registration for the app's connections.
  //
  if (QUIC_FAILED(Status =
                      QuicApi->RegistrationOpen(&RegConfig, &Registration))) {
    printf("RegistrationOpen failed, 0x%x!\n", Status);
    goto Error;
  }

  if (GetFlag(argc, argv, "help") || GetFlag(argc, argv, "?")) {
    PrintUsage();
  } else if (GetFlag(argc, argv, "client")) {
    RunClient(argc, argv);
  } else if (GetFlag(argc, argv, "server")) {
    RunServer(argc, argv);
  } else {
    PrintUsage();
  }

Error:

  if (QuicApi != NULL) {
    if (Configuration != NULL) {
      QuicApi->ConfigurationClose(Configuration);
    }
    if (Registration != NULL) {
      //
      // This will block until all outstanding child objects have been
      // closed.
      //
      QuicApi->RegistrationClose(Registration);
    }
    MsQuicClose(QuicApi);
  }

  return (int)Status;
}
