#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_REQUEST_SIZE 8192

short int read_line(char *buffer, int client) {
  int pos = 0;
  short n;
  int len = 0;
  char c;

  while (pos < MAX_REQUEST_SIZE - 1) {
    n = read(client, &c, 1);
    len += n;

    if (n <= 0)
      break;
    buffer[pos++] = c;

    if (pos >= 2 && buffer[pos - 2] == '\r' && buffer[pos - 1] == '\n') {
      break;
    }
  }

  return len;
}

void *connectionHandler(void *arg) {
  int client = *((int *)arg);
  char method[8];
  char path[1024];
  char version[12];
  char buffer[MAX_REQUEST_SIZE] = {0};

  short int buffLen = read_line(buffer, client);
  printf("%s\n", buffer);
  buffer[buffLen] = '\0';

  if (sscanf(buffer, "%7s %1023s %11s", method, path, version) == 3) {
    fprintf(stderr, "Method : %s\nPath : %s\nVersion :%s", method, path,
            version);
    if (strcmp(method, "GET") != 0) {
      //...
    } else {
      printf("Unsupported request");
    }
  } else {
    printf("400 Bad Request");
  }
}

int main() {
  int server_fd, new_socket;
  ssize_t valread;
  struct sockaddr_in address;
  int opt = 1;
  socklen_t addrlen = sizeof(address);
  char buffer[1024] = {0};
  char *hello = "Hello from server";

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 10) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  while (1) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int *client = malloc(sizeof(int));

    if ((*client = accept(server_fd, (struct sockaddr *)&clientAddr,
                          &clientAddrLen)) < 0) {
      perror("Accept Failed");
      continue;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, connectionHandler, (void *)client);
    pthread_detach(thread);
  }
}
