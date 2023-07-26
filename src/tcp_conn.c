#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 5678

typedef enum { FALSE = 0, TRUE = 1 } BOOLEAN;

unsigned int num_of_msgs = 0;

unsigned int size_of_msgs = 0;

void PrintUsage() {
  printf("\n"
         "TCP/IP connection runs client or server.\n"
         "\n"
         "Usage:\n"
         "\n"
         "  ./tcp_conn -client -target:{IPAddress} -num_of_msgs:{Number} "
         "-size_of_msgs:{Number}\n"
         "  ./tcp_conn -server -cert_file:<...> -key_file:<...> \n");
}

//
// Helper functions to look up a command line arguments.
//
BOOLEAN
GetFlag(int argc, char *argv[], const char *name) {
  const size_t nameLen = strlen(name);
  for (int i = 0; i < argc; i++) {
    if (strncmp(argv[i] + 1, name, nameLen) == 0 &&
        strlen(argv[i]) == nameLen + 1) {
      return TRUE;
    }
  }
  return FALSE;
}

const char *GetValue(int argc, char *argv[], const char *name) {
  const size_t nameLen = strlen(name);
  for (int i = 0; i < argc; i++) {
    if (strncmp(argv[i] + 1, name, nameLen) == 0 &&
        strlen(argv[i]) > 1 + nameLen + 1 && *(argv[i] + 1 + nameLen) == ':') {
      return argv[i] + 1 + nameLen + 1;
    }
  }
  return NULL;
}

// Create a Socket for server communication
short SocketCreate(void) {
  short hSocket;
  printf("Create the socket\n");
  hSocket = socket(AF_INET, SOCK_STREAM, 0);
  return hSocket;
}

int BindCreatedSocket(int hSocket) {
  int iRetval = -1;
  struct sockaddr_in remote = {0};
  remote.sin_family = AF_INET;
  remote.sin_addr.s_addr = htonl(INADDR_ANY);
  remote.sin_port = htons(PORT);
  iRetval = bind(hSocket, (struct sockaddr *)&remote, sizeof(remote));
  return iRetval;
}

void RunServer(int argc, char *argv[], int socket_desc) {
  int sock, clientLen, read_size;
  struct sockaddr_in server, client;
  char client_message[200] = {0};
  char message[100] = {0};
  const char *pMessage = "hello aticleworld.com";

  // Bind
  if (BindCreatedSocket(socket_desc) < 0) {
    // print the error message
    perror("bind failed.");
    goto Error;
  }
  printf("bind done\n");

  // Listen
  listen(socket_desc, 1);

  // Accept and incoming connection
  while (1) {
    printf("Waiting for incoming connections...\n");
    clientLen = sizeof(struct sockaddr_in);
    // accept connection from an incoming client
    sock = accept(socket_desc, (struct sockaddr *)&client,
                  (socklen_t *)&clientLen);
    if (sock < 0) {
      perror("accept failed");
      goto Error;
    }
    printf("Connection accepted\n");
    memset(client_message, '\0', sizeof client_message);
    memset(message, '\0', sizeof message);
    // Receive a reply from the client
    if (recv(sock, client_message, 200, 0) < 0) {
      printf("recv failed");
      break;
    }
    printf("Client reply : %s\n", client_message);
    if (strcmp(pMessage, client_message) == 0) {
      strcpy(message, "Hi there !");
    } else {
      strcpy(message, "Invalid Message !");
    }
    // Send some data
    if (send(sock, message, strlen(message), 0) < 0) {
      printf("Send failed");
      goto Error;
    }

    close(sock);
    sleep(1);
  }
Error:

  printf("Error occured!");
}
//============================Client============================
// try to connect with server
int SocketConnect(int hSocket, const char *IP_address) {
  int iRetval = -1;
  struct sockaddr_in remote = {0};
  remote.sin_addr.s_addr = inet_addr(IP_address); // Local Host
  remote.sin_family = AF_INET;
  remote.sin_port = htons(PORT);
  iRetval =
      connect(hSocket, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));
  return iRetval;
}

// Send the data to the server and set the timeout of 20 seconds
int SocketSend(int hSocket, char *Rqst, short lenRqst) {
  int shortRetval = -1;
  struct timeval tv;
  tv.tv_sec = 20; /* 20 Secs Timeout */
  tv.tv_usec = 0;
  if (setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) <
      0) {
    printf("Time Out\n");
    return -1;
  }
  shortRetval = send(hSocket, Rqst, lenRqst, 0);
  return shortRetval;
}

// receive the data from the server
int SocketReceive(int hSocket, char *Rsp, short RvcSize) {
  int shortRetval = -1;
  struct timeval tv;
  tv.tv_sec = 20; /* 20 Secs Timeout */
  tv.tv_usec = 0;
  if (setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) <
      0) {
    printf("Time Out\n");
    return -1;
  }
  shortRetval = recv(hSocket, Rsp, RvcSize, 0);
  printf("Response %s\n", Rsp);
  return shortRetval;
}

void RunClient(int argc, char *argv[], int hSocket) {
  int read_size;
  struct sockaddr_in server;
  char *Buffer = calloc(size_of_msgs, sizeof(char));
  for (int i = 0; i < size_of_msgs; i++)
    Buffer[i] = 91;

  const char *Target;
  if ((Target = GetValue(argc, argv, "target")) == NULL) {
    printf("Must specify '-target' argument!\n");
    goto Error;
  }

  // Connect to remote server
  if (SocketConnect(hSocket, Target) < 0) {
    perror("connect failed.\n");
    goto Error;
  }
  printf("Sucessfully conected with server\n");

  printf("Enter the Message: ");
  for (int i = 0; i < num_of_msgs; i++) {
    // Send data to the server
    SocketSend(hSocket, Buffer, strlen(Buffer));
  }
  // Received the data from the server
  read_size = SocketReceive(hSocket, Buffer, 200);
  printf("Server Response : %s\n\n", Buffer);
Error:
  close(hSocket);
  shutdown(hSocket, 0);
  shutdown(hSocket, 1);
  shutdown(hSocket, 2);
}

int main(int argc, char *argv[]) {
  int hSocket;
  // Create socket
  hSocket = SocketCreate();
  if (hSocket == -1) {
    printf("Could not create socket\n");
    return -1;
  }
  printf("Socket is created\n");
  if (GetFlag(argc, argv, "help") || GetFlag(argc, argv, "?")) {
    PrintUsage();
  } else if (GetFlag(argc, argv, "client")) {
    RunClient(argc, argv, hSocket);
  } else if (GetFlag(argc, argv, "server")) {
    RunServer(argc, argv, hSocket);
  } else {
    PrintUsage();
  }

  return 0;
}
