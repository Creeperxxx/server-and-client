#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#define PORT 8080
#define BACKLOG 10
#define MAX_EVENTS 10
#define SERVER_IP "127.0.0.1"
#define CLIENT_BUFFER_SIZE 1024
const char* receive_buffer = "received";