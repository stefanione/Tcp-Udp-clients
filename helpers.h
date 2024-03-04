#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define BUFLEN 1551
#define CONTENT_LEN 1500
#define MAX_CLIENTS 10

// Structura pentru clientii TCP.
struct tcp_client { 
  char* id;
  int socket;
  int is_connected;
  int nr_subscriptions;
  struct subscribe* subscriptions;
};

// Structura pentru abonament.
struct subscribe {
  char is_subscribed;
  int sf_arg;
  char* topic;
};

// structura unui mesaj UDP primit de client
struct udp_message {
  int port_nr;
  char ip[20];
  char topic[50];
  char type[20];
  char content[1501];
};

// Functie care gaseste clientul care se conecteaza dupa id.
int search_client_by_id(char* arg_id, int clients_nr, struct tcp_client* clients) { 
  for (int i = 0; i < clients_nr; i++) {
    if (strcmp(clients[i].id, arg_id) == 0) {
      return i;
    }
  }
   return -1;
}

// Functie care gaseste clientul care se conecteaza dupa socket.
int search_client_by_socket(int socket, int clients_nr, 
                            struct tcp_client* clients) { 
  for (int i = 0; i < clients_nr; i = i + 1) {
    if (clients[i].socket == socket) {
      return i;
    }
  }

  return -1;
}

// Functie care gaseste abonamentul dupa topic.
int search_subscription_by_topic(struct tcp_client client, char* topic) {
  for (int i = 0; i < client.nr_subscriptions; i = i + 1) {
    if (strncmp(client.subscriptions[i].topic, topic, strlen(topic)) == 0) {
      return i;
    }
  }

  return -1;
}

// Functie care creeaza un abonament nou.
void add_subscription(char* topic, int sf_arg, struct subscribe* subs) {
  // Retin datele in structura.
  subs->sf_arg = sf_arg;
  subs->is_subscribed = 's';
  subs->topic = malloc(51 * sizeof(char));
  memcpy(subs->topic, topic, strlen(topic) + 1);
}

// Functie care verifica corectitudinea comenii primite de client.
int check_command(char* command) {
  char* argument = strtok(command, " ");

  if (strncmp(argument, "subscribe", strlen("subscribe")) == 0) {
    // Clientul a primit o comanda de abonare la un topic.
    char* topic = strtok(NULL, " "); // Extrag topicul.
    char* sf = strtok(NULL, " ");    // Extrag sf-ul.
    int sf_arg = atoi(sf);

    // Verific daca lungimea topicului este buna.
    if (strlen(topic) > 50) {
      printf("Client error: wrong topic length!\n");
      return 0;
    }

    // Verific daca am primit un argument sf valid.
    if (sf_arg != 0 && sf_arg != 1) {
      printf("Client error: wrong sf number!\n");
      return 0;
    }
  } else if (strncmp(argument, "unsubscribe", strlen("unsubscribe")) == 0) {
    // Clientul a primit o comanda de dezabonare de la un topic.
    char* topic = strtok(NULL, " "); // Extrag topicul.

    // Verific daca lungimea topicului este buna.
    if (strlen(topic) > 50) {
      printf("Client error: wrong topic length!\n");
      return 0;
    }
  } else {
    printf("Client error: unknown command!\n");
    return 0;
  }

  return 1;
}

// Functie care afiseaza un mesaj de abonare/dezabonare.
void display_subscription(char* bufy) {
  if (strncmp(bufy, "subscribe", strlen("subscribe")) == 0) {
    printf("Subscribed to topic.\n");
  } else {
    printf("Unsubscribed from topic.\n");
  }
}

// Functie care creeaza un mesaj UDP nou sub forma unei structuri.
struct udp_message* create_message(char* ip, int port, 
                      char* topic, char* type, char* content) {
  struct udp_message* msg = malloc(1 * sizeof(struct udp_message));

  msg->port_nr = port;
  memcpy(msg->ip, ip, strlen(ip) + 1);
  memcpy(msg->topic, topic, strlen(topic) + 1);

  if (strcmp(type, "0") == 0) {
    memcpy(msg->type, "INT", strlen("INT") + 1);
  } else if (strcmp(type, "1") == 0) {
    memcpy(msg->type, "SHORT_REAL", strlen("SHORT_REAL") + 1);
  } else if (strcmp(type, "2") == 0) {
    memcpy(msg->type, "FLOAT", strlen("FLOAT") + 1);
  } else if (strcmp(type, "3") == 0) {
    memcpy(msg->type, "STRING", strlen("STRING") + 1);
  }

  memcpy(msg->content, content, strlen(content) + 1);

  return msg;
}

#endif