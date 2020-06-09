#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>     /* inet_ntoa */
#include <netdb.h>         /* gethostname */
#include <netinet/in.h>    /* struct sockaddr_in */

#include "socket.h"

void setup_server_socket(struct listen_sock *s) {
    if(!(s->addr = malloc(sizeof(struct sockaddr_in)))) {
        perror("malloc");
        exit(1);
    }
    // Allow sockets across machines.
    s->addr->sin_family = AF_INET;
    // The port the process will listen on.
    s->addr->sin_port = htons(SERVER_PORT);
    // Clear this field; sin_zero is used for padding for the struct.
    memset(&(s->addr->sin_zero), 0, 8);
    // Listen on all network interfaces.
    s->addr->sin_addr.s_addr = INADDR_ANY;

    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock_fd < 0) {
        perror("server socket");
        exit(1);
    }

    // Make sure we can reuse the port immediately after the
    // server terminates. Avoids the "address in use" error
    int on = 1;
    int status = setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &on, sizeof(on));
    if (status < 0) {
        perror("setsockopt");
        exit(1);
    }

    // Bind the selected port to the socket.
    if (bind(s->sock_fd, (struct sockaddr *)s->addr, sizeof(*(s->addr))) < 0) {
        perror("server: bind");
        close(s->sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(s->sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(s->sock_fd);
        exit(1);
    }
}

/* Insert Tutorial 10 helper functions here. */

int find_network_newline(const char *buf, int inbuf) {
  int index = -1;
  for(int i=0;i<inbuf;i++){
    if(buf[i]=='\n'){
      if(buf[i-1]=='\r'){
        index = 1 + i;
        return index;
      }
    }
  }
  
  return -1; 
  
}

int read_from_socket(int sock_fd, char *buf, int *inbuf) {
  int bytes = read(sock_fd, &buf[*inbuf], sizeof(char));
  
  *inbuf += bytes;
  if(bytes==0){
    return 1;
  }
  
  if(bytes<0||strlen(buf)>BUF_SIZE){
    return -1;
  }
  
  
  int index = -1;
  for(int i=0;i<strlen(buf);i++){
    if(buf[i]=='\n'){
      if(buf[i-1]=='\r'){
        index = 1 + i;
        break;
      }
    }
  }
  
  
  if(index != -1){
    return 0;
  }else{
    return 2;
  }
    
}

int get_message(char **dst, char *src, int *inbuf) {
  int num_clf = find_network_newline(src, *inbuf);
  if(num_clf==-1){
    return 1;
  }
  
  *dst = malloc(sizeof(char)*(num_clf-1));
  
  *inbuf -= num_clf;
  snprintf(*dst, (num_clf-1)*sizeof(char),"%s", src);
  
  int size = strlen(src) - num_clf;
  int j=num_clf;
  for(int i=0;i<size;i++){
    memset(src+i, src[j], sizeof(char));
    j++;
  }
  for(int i=size;i<strlen(src);i++){
    memset(src+i, '\0', sizeof(char));
  }
  
  
  return 0;  
}

/* Helper function to be completed for Tutorial 11. */

int write_to_socket(int sock_fd, char *buf, int len) {
  
  int bytes = write(sock_fd, buf, sizeof(char)*len);

  if(bytes<0){
    return 1;
  }
  
  if(bytes==0){
    return 2;
  }
  
  return 0;
  
}
