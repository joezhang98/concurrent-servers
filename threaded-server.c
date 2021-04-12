// A threaded socket server - accepting multiple clients concurrently, by creating
// a new thread for each connecting client.

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

typedef struct { int sockfd; } thread_config_t;

typedef enum { WAIT_FOR_MSG, IN_MSG } ProcessingState;

void serve_connection(int sockfd) {
  // Clients attempting to connect and send data will succeed even before the
  // connection is accept()-ed by the server. Therefore, to better simulate
  // blocking of other clients while one is being served, do this "ack" from the
  // server which the client expects to see before proceeding.
  if (send(sockfd, "*", 1, 0) < 1) {
    perror_die("send");
  }

  ProcessingState state = WAIT_FOR_MSG;

  while (1) {
    uint8_t buf[1024];
    int len = recv(sockfd, buf, sizeof buf, 0);
    if (len < 0) {
      perror_die("recv");
    } else if (len == 0) {
      break;
    }

    for (int i = 0; i < len; ++i) {
      switch (state) {
        case WAIT_FOR_MSG:
          if (buf[i] == '^') {
            state = IN_MSG;
          }
          break;
        case IN_MSG:
          if (buf[i] == '$') {
            state = WAIT_FOR_MSG;
          } else {
            buf[i] += 1;
            if (send(sockfd, &buf[i], 1, 0) < 1) {
              perror("send failed");
              close(sockfd);
              return;
            }
          }
          break;
      }
    }
  } 

  close(sockfd);
} 

void* serve_thread(void* arg) {
  thread_config_t* config = (thread_config_t*)arg;
  int sockfd = config->sockfd;
  free(config);

  // This case will work for Linux, but in general casting pthread_id to an
  // integral type isn't portable.
  unsigned long id = (unsigned long)pthread_self();
  printf("Thread %lu created to handle connection with socket %d\n", id,
         sockfd);
  serve_connection(sockfd);
  printf("Thread %lu done\n", id);
  return 0;
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);

  int portnum = 9090;
  if (argc >= 2) {
    portnum = atoi(argv[1]);
  }
  printf("Serving on port %d\n", portnum);

  int sockfd = listen_inet_socket(portnum);

  while (1) {
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);

    int newsockfd =
        accept(sockfd, (struct sockaddr*)&peer_addr, &peer_addr_len);

    if (newsockfd < 0) {
      perror_die("ERROR on accept");
    }

    report_peer_connected(&peer_addr, peer_addr_len);
    pthread_t the_thread;

    thread_config_t* config = (thread_config_t*)malloc(sizeof(*config));
    if (!config) {
      die("OOM");
    }
    config->sockfd = newsockfd;
    pthread_create(&the_thread, NULL, serve_thread, config);

    // Detach the thread - when it's done, its resources will be cleaned up.
    // Since the main thread lives forever, it will outlive the serving threads.
    pthread_detach(the_thread);
  }

  return 0;
}
