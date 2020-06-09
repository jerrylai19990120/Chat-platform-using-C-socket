#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"

struct server_sock {
    int sock_fd;
    char buf[BUF_SIZE];
    int inbuf;
};

int main(int argc, char**argv) {
  
    struct server_sock s;
    s.inbuf = 0;
    int exit_status = 0;
    
    // Create the socket FD.
    s.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s.sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    if(argv[1]){
      if (inet_pton(AF_INET, argv[1], &server.sin_addr) < 1) {
        perror("client: inet_pton");
        close(s.sock_fd);
        exit(1);
      }
    }else{
      if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1) {
        perror("client: inet_pton");
        close(s.sock_fd);
        exit(1);
      }
    }
    

    // Connect to the server.
    if (connect(s.sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(s.sock_fd);
        exit(1);
    }

    char *buf = NULL; // Buffer to read name from stdin
    int name_valid = 0;
    while(!name_valid) {
        printf("Please enter a username: ");
        fflush(stdout);
        size_t buf_len = 0;
        size_t name_len = getline(&buf, &buf_len, stdin);
        if (name_len < 0) {
            perror("getline");
            fprintf(stderr, "Error reading username.\n");
            free(buf);
            exit(1);
        }
        
        if (name_len - 1 > MAX_NAME) { // name_len includes '\n'
            printf("Username can be at most %d characters.\n", MAX_NAME);
        }
        else {
            // Replace LF+NULL with CR+LF
            buf[name_len-1] = '\r';
            buf[name_len] = '\n';
            if (write_to_socket(s.sock_fd, buf, name_len+1)) {
                fprintf(stderr, "Error sending username.\n");
                free(buf);
                exit(1);
            }
            name_valid = 1;
            free(buf);
        }
    }
    
    /*
     * See here for why getline() is used above instead of fgets():
     * https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152445
     */
    
    /*
     * Step 1: Prepare to read from stdin as well as the socket,
     * by setting up a file descriptor set and allocating a buffer
     * to read into. It is suggested that you use buf for saving data
     * read from stdin, and s.buf for saving data read from the socket.
     * Why? Because note that the maximum size of a user-sent message
     * is MAX_USR_MSG + 2, whereas the maximum size of a server-sent
     * message is MAX_NAME + 1 + MAX_USER_MSG + 2. Refer to the macros
     * defined in socket.h.
     */
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(s.sock_fd, &read_fds);
    buf = malloc(sizeof(char)*BUF_SIZE);
    int numfd;
    
    if(STDIN_FILENO > s.sock_fd){
      numfd = STDIN_FILENO + 1;
    }else{
      numfd = s.sock_fd + 1;
    }
    /*
     * Step 2: Using select, monitor the socket for incoming mesages
     * from the server and stdin for data typed in by the user.
     */
    int status = 1;
    while(status) {
         
         select(numfd, &read_fds, NULL, NULL, NULL);

        
        /*
         * Step 3: Read user-entered message from the standard input
         * stream. We should read at most MAX_USR_MSG bytes at a time.
         * If the user types in a message longer than MAX_USR_MSG,
         * we should leave the rest of the message in the standard
         * input stream so that we can read it later when we loop
         * back around.
         * 
         * In other words, an oversized messages will be split up
         * into smaller messages. For example, assuming that
         * MAX_USR_MSG is 10 bytes, a message of 22 bytes would be
         * split up into 3 messages of length 10, 10, and 2,
         * respectively.
         * 
         * It will probably be easier to do this using a combination of
         * fgets() and ungetc(). You probably don't want to use
         * getline() as was used for reading the user name, because
         * that would read all the bytes off of the standard input
         * stream, even if it exceeds MAX_USR_MSG.
         */
        if(FD_ISSET(STDIN_FILENO, &read_fds)){
          
          if(fgets(buf, MAX_USER_MSG, stdin)<0){
            perror("fgets");
          }
          
          int msg_len = strlen(buf);
          buf[msg_len-1] = '\r';
          buf[msg_len] = '\n';
         
          if(msg_len-2 > MAX_USER_MSG){
            write_to_socket(s.sock_fd, buf, MAX_USER_MSG+2);
          }else{
            write_to_socket(s.sock_fd, buf, msg_len+1);
          }
           
          
        }
       
        /*
         * Step 4: Read server-sent messages from the socket.
         * The read_from_socket() and get_message() helper functions
         * will be useful here. This will look similar to the
         * server-side code.
         */
        if(FD_ISSET(s.sock_fd, &read_fds)){
        
          int server_status;
          do{
            
            server_status = read_from_socket(s.sock_fd, s.buf, &(s.inbuf));
            
            if(server_status<0){
              fprintf(stderr, "Error reading from server.\n");
              status = 0;
              break;
            }else if(server_status==1){
              fprintf(stderr, "Server is closed.\n");
              status = 0;
              break;
            }
            char *msg;
            while(server_status==0 && !get_message(&msg, s.buf, &(s.inbuf))){
              int i = 0;
              int index = -1;
              while(i<strlen(msg)){
                if(msg[i]==' '){
                  index = i;
                  break;
                }
                i++;
              }
              
              msg[index] = '|';
              char* name = strtok(msg, "|");
              char* body = strtok(NULL, "|");
              printf("%s: %s\n", name, body);
              free(msg);
            }
            break;
          } while (server_status != 1);
          
         
        }
       
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(s.sock_fd, &read_fds);
        
        if(STDIN_FILENO > s.sock_fd){
          numfd = STDIN_FILENO + 1;
        }else{
          numfd = s.sock_fd + 1;
        }
        
    }
    free(buf);
    close(s.sock_fd);
    exit(exit_status);
}
