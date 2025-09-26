#include <fcntl.h>
#include <magic.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define MAX_REQUEST_SIZE 8192

const char *getMime2(const char *fileName) {
  const char *ext = strrchr(fileName, '.');
  if (!ext)
    return "application/octet-stream";

  if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
    return "text/html";
  if (strcmp(ext, ".css") == 0)
    return "text/css";
  if (strcmp(ext, ".js") == 0)
    return "application/javascript";
  if (strcmp(ext, ".png") == 0)
    return "image/png";
  if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    return "image/jpeg";
  if (strcmp(ext, ".gif") == 0)
    return "image/gif";
  if (strcmp(ext, ".svg") == 0)
    return "image/svg+xml";
  if (strcmp(ext, ".txt") == 0)
    return "text/plain";
  if (strcmp(ext, ".ico") == 0)
    return "image/x-icon";

  // fallback for unknown extensions
  return "application/octet-stream";
}

char *getMime(char *filePath) {
  const char *mime;
  magic_t magic;

  magic = magic_open(MAGIC_MIME_TYPE);
  magic_load(magic, NULL);
  magic_compile(magic, NULL);
  mime = magic_file(magic, filePath);

  magic_close(magic);

  return mime;
}

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

void createGETResponse(char *fileName, int client) {
  char *filePath = malloc(strlen(fileName) + strlen("file/") + 1);
  sprintf(filePath, "file%s", fileName);

  int fileFd = open(filePath, O_RDONLY);
  if (fileFd == -1) {
    char negativeResponse[512];
    snprintf(negativeResponse, sizeof(negativeResponse),
             "HTTP/1.1 404 Not Found\r\n"
             "Content-Type: text/plain\r\n"
             "\r\n"
             "404 Not Found");
    send(client, negativeResponse, strlen(negativeResponse), 0);
    free(filePath);
    return;
  }

  const char *mime = getMime2(filePath);
  FILE *f = fopen(filePath, "rb");
  fseek(f, 0, SEEK_END);
  long fileSize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char header[512];
  snprintf(header, 512,
           "HTTP/1.1 200 OK \r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %ld\r\n"
           "\r\n",
           mime, fileSize);

  printf("\n%s\n", fileName);
  fprintf(stderr, header);
  write(client, header, strlen(header));

  char buf[2048];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    write(client, buf, n);
  }
}

void *connectionHandler(void *arg) {
  int client = *((int *)arg);
  char method[8];
  char path[1024];
  char version[12];
  char buffer[MAX_REQUEST_SIZE] = {0};

  while (1) {
    short int buffLen = read_line(buffer, client);
    printf("%s\n", buffer);
    buffer[buffLen] = '\0';

    if (sscanf(buffer, "%7s %1023s %11s", method, path, version) == 3) {
      fprintf(stderr, "Method : %s\nPath : %s\nVersion :%s", method, path,
              version);

      if (strcmp(method, "GET") == 0) {
        createGETResponse(path, client);
      } else {
        printf("Unsupported request");
      }
    } else {
      printf("400 Bad Request");
    }
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
