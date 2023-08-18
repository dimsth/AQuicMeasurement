#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define KILO_NUM 1024
#define MEGA_NUM (KILO_NUM * KILO_NUM)
#define MAX_BUFFER_SIZE (5 * MEGA_NUM)

typedef enum { FALSE = 0, TRUE = 1 } BOOLEAN;

int Port = 0;

char *final_msg = "Closing Socket!";

unsigned int num_of_msgs = 0;

long long int size_of_msgs = 0;

float percent;

void PrintUsage() {
  printf("\n"
         "TCP/IP connection runs client or server.\n"
         "\n"
         "Usage:\n"
         "\n"
         "  ./tcp_conn -client -target:{IPAddress} -port:{Port Number} "
         "-num_of_msgs:{Number} -size_of_msgs:{Number}\n"
         "  ./tcp_conn -server -port:{Port Number} \n");
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
  remote.sin_port = htons(Port);
  iRetval = bind(hSocket, (struct sockaddr *)&remote, sizeof(remote));
  return iRetval;
}

void RunServer(int argc, char *argv[], int socket_desc) {
  int sock;
  struct sockaddr_in client;
  char Buffer[MAX_BUFFER_SIZE];
  char message[100];

  // Get Port num from the argv
  const char *s_port;
  if ((s_port = GetValue(argc, argv, "port")) == NULL) {
    printf("Must specify '-port' argument!\n");
    goto Error;
  }
  Port = atoi(s_port);

  // Bind
  if (BindCreatedSocket(socket_desc) < 0) {
    // print the error message
    perror("bind failed.");
    goto Error;
  }
  printf("bind done\n");
  // Listen
  listen(socket_desc, 1);

  printf("Waiting for incoming connections...\n");
  int clientLen = sizeof(struct sockaddr_in);
  // accept connection from an incoming client
  sock =
      accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&clientLen);
  if (sock < 0) {
    perror("accept failed");
    goto Error;
  }
  printf("Connection accepted\n");

  int fm_size = strlen(final_msg) + 1;
  int ret;
  // Accept and incoming connection
  while (1) {
    memset(Buffer, 0, MAX_BUFFER_SIZE);

    //  Receive a reply from the client
    ret = recv(sock, Buffer, MAX_BUFFER_SIZE, 0);
    if (ret < 0) {
      printf("Recv failed! \n");
      goto Error;
    } else if (ret == 0) {
      printf("Client closed connection! \n");
      goto Close_Sock;
    } else {
      num_of_msgs++;
      size_of_msgs += ret;
    }

    if (memcmp(Buffer + ret - fm_size, final_msg, fm_size) == 0) {
      printf("Final message came!\n");
      break;
    }
  }

  sprintf(message,
          "Received total number of msgs: %d \n Received total size of msgs: "
          "%lld \n",
          num_of_msgs, size_of_msgs);
  printf("%s", message);
  // Send some data
  if (send(sock, message, strlen(message), 0) < 0) {
    printf("Send failed");
    goto Error;
  }

Error:
  if (ret < 0)
    printf("Error occured!");
Close_Sock:
  close(sock);
}
//============================Client============================
// try to connect with server
int SocketConnect(int hSocket, const char *IP_address) {
  int iRetval = -1;
  struct sockaddr_in remote = {0};
  remote.sin_addr.s_addr = inet_addr(IP_address); // Local Host
  remote.sin_family = AF_INET;
  remote.sin_port = htons(Port);
  iRetval =
      connect(hSocket, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));
  return iRetval;
}

// receive the data from the server
int SocketReceive(int hSocket, char *Rsp, short RvcSize) {
  int shortRetval = -1;
  shortRetval = recv(hSocket, Rsp, RvcSize, 0);
  printf("Server Response: %s\n", Rsp);
  return shortRetval;
}

void RunClient(int argc, char *argv[], int hSocket) {

  // Get number of msgs to send from argv
  const char *nom;
  if ((nom = GetValue(argc, argv, "num_of_msgs")) == NULL) {
    printf("Must specify '-num_of_msgs' argument!\n");
    goto Error;
  }
  num_of_msgs = atoi(nom);
  percent = (float)num_of_msgs / 20;

  // Get size of msgs to send from argv
  const char *som;
  if ((som = GetValue(argc, argv, "size_of_msgs")) == NULL) {
    printf("Must specify '-size_of_msgs' argument!\n");
    goto Error;
  }
  size_of_msgs = atoi(som);

  // Allocate the msgs that in the correct size
  char *Buffer = calloc(size_of_msgs, sizeof(char));
  memset(Buffer, 90, size_of_msgs - 1);
  Buffer[size_of_msgs - 1] = '\0';

  // Get IP from argv
  const char *Target;
  if ((Target = GetValue(argc, argv, "target")) == NULL) {
    printf("Must specify '-target' argument!\n");
    goto Error;
  }

  // Get Port num from the argv
  const char *s_port;
  if ((s_port = GetValue(argc, argv, "port")) == NULL) {
    printf("Must specify '-port' argument!\n");
    goto Error;
  }
  Port = atoi(s_port);

  // Connect to remote server
  if (SocketConnect(hSocket, Target) < 0) {
    perror("connect failed.\n");
    goto Error;
  }
  printf("Sucessfully conected with server\n");

  printf("Start sending messages!\n");
  for (unsigned int i = 0; i < num_of_msgs; i++) {
    // Send data to the server
    if ((unsigned int)percent == i) {
      printf("[%d done] Sending data %d...\n",
             (int)((percent * 100) / num_of_msgs), i);
      percent += (float)num_of_msgs / 20;
    }
    if (send(hSocket, Buffer, size_of_msgs, 0) < 0) {
      printf("Send failed\n");
      goto Error;
    }
  }

  printf("[100 done] Sending final message!\n");
  if (send(hSocket, final_msg, strlen(final_msg) + 1, 0) < 0) {
    printf("Send final message failed\n");
    goto Error;
  }

  // Receive the data from the server
  char response[200];
  memset(response, '\0', sizeof(response));
  if (SocketReceive(hSocket, response, sizeof(response) - 1) < 0) {
    printf("Receive failed\n");
    goto Error;
  }

Error:
  close(hSocket);
}

int main(int argc, char *argv[]) {
  // Create socket
  int hSocket = SocketCreate();
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
