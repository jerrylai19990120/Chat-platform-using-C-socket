#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "socket.h"
#include "chat_helpers.h"

int write_buf_to_client(struct client_sock *c, char *buf, int len) {
    // To be completed.
    int msg_len = strlen(buf);
    buf[msg_len] = '\r';
    buf[msg_len+1] = '\n';
    return write_to_socket(c->sock_fd, buf, len+2);
}

int remove_client(struct client_sock **curr, struct client_sock **clients) {
  
  if((*clients)==NULL || (*curr)==NULL){
    return 1;
  }
  
  if((*curr)->sock_fd == (*clients)->sock_fd){
    memset((*clients)->buf, '0', BUF_SIZE);
    (*clients)->inbuf = 0;
    (*clients)->sock_fd = -1;
    (*clients)->username = NULL;  
    clients = &((*clients)->next);
    return 0;
  }
  struct client_sock * current = (*clients)->next;
  while( current != NULL){
    if((*curr)->sock_fd == current->sock_fd){
      memset((*curr)->buf, '0', BUF_SIZE);
      (*curr)->inbuf = 0;
      (*curr)->sock_fd = -1;
      (*curr)->username = NULL;
      curr = &(current->next);
      return 0;
    }
   
    current = current->next;
    
  }
  
  
  
  return 1; // Couldn't find the client in the list, or empty list
}

int read_from_client(struct client_sock *curr) {
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
}

int set_username(struct client_sock *curr) {
    // To be completed. Hint: Use get_message().
  int client_status;
  int val;
  do{
    client_status = read_from_client(curr);
    if(client_status<0){
      fprintf(stderr, "Error setting username.\n");
      break;
    }else if(client_status == 1){
      fprintf(stderr, "Socket is already closed.\n");
      break;
    }else if(client_status == 0){
      val = get_message(&(curr->username), curr->buf, &(curr->inbuf));
      if(val == 1){
        return val;
      }else{
        break;
      }
    }
  } while (client_status != 1);
  
  return 0;
}
