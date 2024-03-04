#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <sys/select.h>

#include "helpers.h"


int main(int argc, char* argv[])
{   
    // Declarari si initializari.
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int enable = 1;
    int listenfd_tcp = -1, listenfd_udp = -1;
    int res, sock_tcp = -1, idx;
    struct sockaddr_in serv_tcp_addr, serv_udp_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    if (argc != 2) {
        printf("\n Usage: %s <port>\n", argv[0]);
        return -1;
    }

    // Server-ul va receptiona un stream de mesaje de la clientii TCP.
    listenfd_tcp = socket(AF_INET, SOCK_STREAM, 0); 

    // Server-ul va receptiona un stream de mesaje de la clientii UDP.
    listenfd_udp = socket(PF_INET, SOCK_DGRAM, 0); 

    int port_nr = atoi(argv[1]);

    // Completez datele pentru conectarea cu clientii TCP.
    serv_tcp_addr.sin_family = AF_INET;            // Setez familia de adrese.
    serv_tcp_addr.sin_port = htons(port_nr); // Setez portul ales.
    serv_tcp_addr.sin_addr.s_addr = INADDR_ANY;    // Server-ul primeste request de la orice adresa.

    res = bind(listenfd_tcp, (struct sockaddr*) &serv_tcp_addr, sizeof(serv_tcp_addr)); 
     if (res < 0) {
       fprintf(stderr, "Server: eroare la functia bind pentru listenfd_tcp!\n"); 
       close(listenfd_tcp);
       exit(-1);
    }

    res = listen(listenfd_tcp, 1000); 
    DIE(res < 0, "Server: eroare la functia listen!\n");

    DIE(setsockopt(listenfd_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0, "Setsockopt(SO_REUSEADDR) failed (server) !!!\n");

    // Completez datele pentru conectarea cu clientii UDP.
    serv_udp_addr.sin_family = AF_INET;            // Setez familia de adrese.
    serv_udp_addr.sin_port = htons(port_nr); // Setez portul ales.
    serv_udp_addr.sin_addr.s_addr = INADDR_ANY;    // Server-ul primeste request de la orice adresa.

    res = bind(listenfd_udp, (struct sockaddr*) &serv_udp_addr, sizeof(serv_udp_addr)); 
     if (res < 0) {
       fprintf(stderr, "Server: eroare la functia bind pentru listenfd_udp!\n"); 
       close(listenfd_udp);
       exit(-1);
    }

    // Creez multimile de file descriptori.
    fd_set input_fd;
    fd_set copy_fd;
    FD_ZERO(&input_fd);
    FD_SET(STDIN_FILENO, &input_fd);
    FD_SET(listenfd_tcp, &input_fd);
    FD_SET(listenfd_udp, &input_fd);

    int max_fd = listenfd_tcp;
    if (max_fd < listenfd_udp) {
      max_fd = listenfd_udp;
    }

    struct sockaddr_in cli;
    char buf[BUFLEN];

    int clients_idx = 0;

    // Aloc spatiu de memorie pentru vectorul de clienti TCP.
    struct tcp_client* tcp_clients = malloc(MAX_CLIENTS * sizeof(struct tcp_client)); 
    DIE(tcp_clients == NULL, "Server error when allocating memory to clients vector\n");

    while (1) {
      copy_fd = input_fd;

      // Retin file descriptorii pe care au loc evenimente.
      res = select(max_fd + 1, &copy_fd, NULL, NULL, NULL); 
      DIE(res < 0, "Error in select(server) !!! \n");

      for (int i = 0 ; i <= max_fd; i++) {
        if (FD_ISSET(i, &copy_fd)) {
          if (i == STDIN_FILENO) {
            // Daca primesc o comanda de la tastatura
            fgets(buf, 99, stdin);

            if(strncmp(buf, "exit", 4) == 0) {
              break;
            }
          } else if (i == listenfd_udp) {
            // Se primesc mesaje de la clinetii UDP.
          } else if (i == listenfd_tcp) {
            // Daca are loc un eveniment pe socket-ul ce primeste cereri de conexiune de la
            // clientii TCP, dau accept.
            sock_tcp = accept(listenfd_tcp, (struct sockaddr *)&cli, &socket_len);

            DIE(sock_tcp < 0, "Error accept(server) !!! \n");

            DIE(setsockopt(sock_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0, "Setsockopt(SO_REUSEADDR) failed (server) !!!\n");
            
            // Server-ul primeste de la client mesaj cu ID-ul acestuia.
            res = recv(sock_tcp, buf, BUFLEN, 0);
            if(res > 0) {
                // Cautam clientul de la care serverul primeste mesaj, dupa id.
                idx = search_client_by_id(buf, clients_idx, tcp_clients); 
                if (idx != -1) {
                  // Daca este in lista de clienti un client cu acelasi ID
                  if (tcp_clients[idx].is_connected == 0) {
                    // Daca clientul a fost conectat anterior, ii actualizez status-ul si socket-ul.
                    // Adaug in multimea de file descriptori socket-ul nou creat.
                    FD_SET(sock_tcp, &input_fd);

                    // Reactualizez file descriptorul maxim.
                    if (sock_tcp > max_fd) {
                      max_fd = sock_tcp; 
                    }

                    // Setez status-ul clientului ca fiind activ.
                    tcp_clients[idx].is_connected = 1; 
                    // Ii stabilim si socketul
                    tcp_clients[idx].socket = sock_tcp; 

                    // Afisez mesajul de conectare al unui nou client.
                    printf("New client %s connected from %s:%d.\n", buf, inet_ntoa(cli.sin_addr), cli.sin_port); 
                  } else {
                    printf("Client %s already connected.\n", buf);
                    // Inchid socket-ul.
                    close(sock_tcp); 
                  }
                } else {
                  // In cazul in care nu a mai fost un client conectat cu acelasi ID anterior,
                  // creez un client nou si ii completez datele. Adaug acest client in lista de 
                  // clieti.
                    FD_SET(sock_tcp, &input_fd);
                    if (sock_tcp > max_fd) {
                      max_fd = sock_tcp; 
                    }

                    tcp_clients[clients_idx].is_connected = 1;
                    tcp_clients[clients_idx].socket = sock_tcp;
                    tcp_clients[clients_idx].nr_subscriptions = 0;
                    tcp_clients[clients_idx].subscriptions = malloc(20 * sizeof(struct subscribe));
                    tcp_clients[clients_idx].id = malloc(10 * sizeof(char));
                    memcpy(tcp_clients[clients_idx].id, buf, strlen(buf) + 1);

                    // Actualizez numarul de clienti.
                    clients_idx = clients_idx + 1;

                    // Afisez mesajul corespunzator conectarii clientului.
                    printf("New client %s connected from %s:%d.\n", buf, inet_ntoa(cli.sin_addr), cli.sin_port); 
                }
            } else if (res < 0) {
              // Daca server-ul nu a primit nimic de la client, afisez un mesaj de eroare.
              DIE(res < 0, "Hasn't been received (server) \n"); 
            }
          } else {
            // Primesc un mesaj de la client.
            res = recv(i, buf, BUFLEN, 0);
            if(res > 0) {
                idx = search_client_by_socket(i, clients_idx, tcp_clients);
                // In cazul in care clinetul notifica server-ul ca s-a deconectat
                if (strstr(buf, "disconnected") != NULL) {
                  if (idx != -1) {
                    // Daca clientul se deconecteaza de la server, ii setez status-ul la 0.
                    tcp_clients[idx].is_connected = 0; 
                    // Afisez mesajul de deconectare al clientului.
                    printf("Client %s disconnected.\n", tcp_clients[idx].id); 
                    // Sterg socket-ul acestuia din lista de file descriptori.
                    FD_CLR(i, &input_fd);
                    // Inchid file descriptorul.
                    close(i); 
                  }
                } else {
                  // Primesc mesaj de abonare/dezabonare de la client.
                  int sub_idx;
                  char* argument = strtok(buf, " ");
                  char* topic = strtok(NULL, " "); // Extrag topicul.
                  // Caut topicul in lista de abonari a clientului
                  sub_idx = search_subscription_by_topic(tcp_clients[idx], topic);

                  if (strcmp(argument, "subscribe") == 0) {
                    char* sf = strtok(NULL, " "); // Extrag sf-ul.
                    int sf_arg = atoi(sf);

                    // Daca clientul nu a mai fost abonat la topicul curent
                    if (sub_idx == -1) {
                      // Ii creez un nou abonament si il adaug in lista de abonamente.
                      sub_idx = tcp_clients[idx].nr_subscriptions;
                      add_subscription(topic, sf_arg, &tcp_clients[idx].subscriptions[sub_idx]);
                      tcp_clients[idx].nr_subscriptions++;
                    } else {
                      // Daca abonamentul exista deja, actualizez sf-ul.
                      tcp_clients[idx].subscriptions[sub_idx].sf_arg = sf_arg;
                    }
                  } else {
                    tcp_clients[idx].subscriptions[sub_idx].is_subscribed = 'u';
                  }
                }
            } else if (res < 0) {
              // Afisez un mesaj corespunzator in caz de eroare.
              DIE(res < 0, "Hasn't been sent (server) !!! \n");
            }
          }
        }
      }

      if (strncmp (buf, "exit", 4) == 0) {
        break; 
      }
    }

    // Eliberez spatiul de memorie alocat.
    for (int i = 0; i < clients_idx; i++) {
      for (int j = 0; j < tcp_clients[i].nr_subscriptions; j++) {
        free(tcp_clients[i].subscriptions[j].topic);
      }

      free(tcp_clients[i].subscriptions);

      free(tcp_clients[i].id);
    }

    free(tcp_clients);

    // Inchidem socketii de comunicare.
    close(listenfd_tcp);

    return 0;
}