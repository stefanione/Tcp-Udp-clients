#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/tcp.h>
#include <sys/select.h>

#include "helpers.h"

int main(int argc, char* argv[]) {
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  //DECLARARI SI INITIALIZARI
  int sockfd = -1, sock_recv = -1; 
  int enable = 1;
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);
  memset(&serv_addr, 0, socket_len);
  int res, ok = 0;

  if (argc != 4) {
    printf("\n Usage: %s <client_ID> <ip> <port>\n", argv[0]);
    return 1;
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  serv_addr.sin_family = AF_INET; 
  serv_addr.sin_port = htons(atoi(argv[3])); 
  inet_aton(argv[2], &serv_addr.sin_addr); 

  // Dezactivez algoritmul Neagle.
  int f = 1;
  int value = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
              (char*) &f, sizeof(int));
  DIE(value < 0, "Client error when deactivating Neagle algorithm!\n"); 

  fd_set client_fd;
  fd_set copy_client_fd;
  FD_ZERO(&client_fd);
  FD_SET(STDIN_FILENO, &client_fd);
  FD_SET(sockfd, &client_fd);

  // Reazlizez conexiunea cu server-ul.
  int c = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)); 
  DIE(c == -1, "Client error when trying to connect to server!\n");

  // Ii trimit server-ului un mesaj cu ID-ul propriu.
  int sent = send(sockfd, argv[1], strlen(argv[1]) + 1, 0); 

  char bufy[BUFLEN];
    
  while (1) {
    copy_client_fd = client_fd;

    // Retin file descriptorii pentru care au avut loc evenimente.
    res = select(sockfd + 1, &copy_client_fd, NULL, NULL, NULL); 
    DIE(res < 0, "Client error when selecting file descriptors!\n"); 

    for (int i = 0;i <= sockfd; i++) {
      if (FD_ISSET(i, &copy_client_fd)) {
        // Daca am primit o comanda de la tastatura.
        if (i == STDIN_FILENO) {
          fgets(bufy, BUFLEN, stdin);

          if (strncmp(bufy, "exit", 4) == 0) {
            char disconnect_message[BUFLEN];
            char *ID = argv[1];
            memcpy(disconnect_message, "Client ", 9);
            char *new = strcat(disconnect_message, ID);
            char *second = strcat(new, " disconnected.");

            // Instiintez server-ul ca clientul s-a deconectat.
            int send2 = send(sockfd, second, strlen(second) + 1, 0); 
            DIE(send2 == -1, "Client error when sending disconnect message to server!\n"); 
            
            // Ies din bucla for.
            break; 
          } else {
            char copy[BUFLEN];
            memcpy(copy, bufy, strlen(bufy));

            // Verific daca comanda introdusa este corecta
            res = check_command(copy);
            if (res == 1) {
              res = send(sockfd, bufy, strlen(bufy) + 1, 0);
  				    DIE(res < 0, "Client error: could not send subscription message to server!\n");
              display_subscription(bufy);
            }
          }
        } else if (i == sockfd) {
          // Daca a avut loc un eveniment pe socket-ul de comunicare cu server-ul, atunci
          // inseamna ca primesc un mesaj de la server.
          memset(bufy, 0, (sizeof(struct udp_message)));
          res = recv(sockfd, bufy, (sizeof(struct udp_message)), 0);
          DIE(res < 0, "Client error when receiving message from server!\n");

          if (res > 0) {
            // Se primesc mesaje de la server.
          } else if (res == 0) {
            ok = 1; 
            break;
          }
        }
      }
    }

    // In cazul in care comanda exit este tastata in cadrul clientului sau 
    // serverul isi incheie activitatea se va da break.
    if ((strncmp(bufy, "exit", 4) == 0) || (ok == 1)) { 
      break;
    }
  }

  // Inchid socket-ul de comunicare cu server-ul.
  close(sockfd); 

  return 0;
}
