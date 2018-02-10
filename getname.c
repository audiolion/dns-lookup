#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>


struct resource_record {
  unsigned char *name;
  unsigned short type;
  unsigned short klass;
  signed int ttl;
  unsigned short rdlength;
  unsigned char *rdata;
};


struct header {
  unsigned short id;
  unsigned char qr:1; // 0 == query, 1 == response
  unsigned char opcode:4; // 0 == standard query, 1 == inverse query, 2 == server status request
  unsigned char aa:1; // authoritative answer, whether responding server is an authority for the domain
  unsigned char tc:1; // truncate, 1 indicates message was truncated
  unsigned char rd:1; // recursion desired, set in query, copied to response, indicates ns should pursue query recursively
  unsigned char ra:1; // recursion available, set or cleared in response indicating if ns supports recursion queries
  unsigned char z:3; // reservered bits for future use, must be 0 in all queries and responses
  unsigned char rcode:4; // response code, 0 == no error, 1 == format error, 2 == server fail, 3 == name error, 4 == not implemented, 5 == refused
  unsigned short qdcount; // count of entries in question section
  unsigned short ancount; // count of resource records in answer section
  unsigned short nscount; // count of name server resource records in the authority records section
  unsigned short arcount; // count of resource records in the additional records section
};


struct question {
  unsigned short qtype;
  unsigned short qclass; // all valid resource record types, with additional general codes
};


typedef struct {
  struct header header;
  struct question question;
} DNS_MESSAGE;


struct label {
  char length;
  char *domain_name;
};


void split_to_labels(struct label *labels, char *domain_name, short token_count) {
  int i = 0;
  char *token = strtok(domain_name, ".");
  char *tokens[token_count];
  while(token != NULL) {
    tokens[i++] = token;
    token = strtok(NULL, ".");
  }
   // iterate through array of tokens and assign lengths
  int j = 0;
  for (j = 0; j < i; j++) {
    char *token = tokens[j];
    labels[j].length = strlen(token);
    labels[j].domain_name = token;
  }
};

char *label_to_str (struct label label) {
  char *label_str = malloc(65);
  label_str[0] = label.length;
  strncpy(&label_str[1], label.domain_name, label.length);
  label_str[label.length + 1] = '\0';
  return label_str;
};


struct header build_dns_header() {
  return (struct header) {
    .id = (unsigned short) htons(rand()),
    .qr = 0,
    .rd = 0,
    .qdcount = htons(1),
  };
};


struct question build_dns_question() {
  return (struct question) {
    .qtype = htons(1), // A record == 1
    .qclass = htons(1), // Internet == 1
  };
};


DNS_MESSAGE build_dns_message() {
  return (DNS_MESSAGE) {
    .header = build_dns_header(),
    .question = build_dns_question(),
  };
};

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: getname <domain_name> <dns_server_ip>");
    exit(EXIT_FAILURE);
  }
  /* Set up random generator for dns header id generation */
  time_t t;
  srand(time(&t));

  char *dns_ip_address = argv[2];
  int sock;
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == -1) {
      /* if socket failed to initialize, exit */
      printf("Error Creating Socket");
      exit(EXIT_FAILURE);
    }

  struct sockaddr_in destination;
  memset(&destination, 0, sizeof destination);

  destination.sin_family = AF_INET;
  destination.sin_addr.s_addr = inet_addr(dns_ip_address);
  destination.sin_port = htons(53);

  int query_name_length = strlen(argv[1]);
  char name_to_query[query_name_length];
  strncpy(name_to_query, argv[1], query_name_length);
  const char DELIMITER = '.';
  short token_count = 1;
  for (int i = 0; i < strlen(name_to_query); i++) {
    if (name_to_query[i] == DELIMITER) {
      token_count++;
    }
  }
  struct label *labels = malloc(token_count * sizeof(*labels));
  split_to_labels(labels, name_to_query, token_count);

  // max size of qname allowed is 255
  char qname[255];
  for(int i = 0; i < token_count; i++) {
    if (i == 0) {
      strcpy(qname, label_to_str(labels[i]));
    } else {
      strcat(qname, label_to_str(labels[i]));
    }
  }

  // UDP DNS lookups have a max size of 512 bytes
  struct header dns_header = build_dns_header();
  struct question dns_question = build_dns_question();
  char buffer[512];
  char *buffer_ptr = buffer;
  memcpy(buffer_ptr, &dns_header, sizeof (struct header));
  buffer_ptr += sizeof (struct header);
  memcpy(buffer_ptr, qname, strlen((char *) qname) + 1);
  buffer_ptr += strlen((char *) qname) + 1;
  memcpy(buffer_ptr, &dns_question, sizeof dns_question);
  buffer_ptr += sizeof dns_question;
  int bytes_sent = sendto(sock, buffer, buffer_ptr - buffer, 0, (struct sockaddr*)&destination, sizeof destination);
  if (bytes_sent < 0) {
    printf("\nError sending packet: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  socklen_t socket_length = sizeof destination;
  if(recvfrom(sock, buffer, buffer_ptr - buffer, 0, (struct sockaddr*)&destination, &socket_length) < 0) {
    printf("\nError receiving packet: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  close(sock); /* close the socket */
  return 0;
}
