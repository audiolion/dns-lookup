#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
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
  unsigned char *name;
  unsigned short qtype; // domain name in octet format, no padding is used to fill, zero length octet to terminate
  unsigned short qclass; // all valid resource record types, with additional general codes
};

typedef struct {
  struct header header;
  struct question question;
} DNS_MESSAGE;


struct label {
  unsigned short length;
  char *domain_name;
};

void split_to_labels(struct label *labels, char *domain_name) {
  int i = 0;
  char *token = strtok(domain_name, ".");
  char *tokens[10];
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
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: getname <domain_name> <dns_server_ip>");
    exit(EXIT_FAILURE);
  }
  char *name_to_query = argv[1];
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

  /* The address is IPv4 */
  destination.sin_family = AF_INET;

   /* IPv4 adresses is a uint32_t, convert a string representation of the octets to the appropriate value */
  destination.sin_addr.s_addr = inet_addr(dns_ip_address);

  /* sockets are unsigned shorts, htons(x) ensures x is in network byte order, set the port to 53 per rfc 1035 */
  destination.sin_port = htons(53);

  const char DELIMITER = '.';
  short token_count = 0;
  for (int i = 0; i < strlen(name_to_query); i++) {
    if (name_to_query[i] == DELIMITER) {
      token_count++;
    }
  }

  DNS_MESSAGE *dns_message = build_dns_message();
  // bytes_sent = sendto(sock, , strlen(), 0,(struct sockaddr*)&destination, sizeof destination);
  // if (bytes_sent < 0) {
  //   printf("Error sending packet: %s\n", strerror(errno));
  //   exit(EXIT_FAILURE);
  // }

  close(sock); /* close the socket */
  return 0;
}

DNS_MESSAGE *build_dns_message(char *name_to_query) {
  /* Set up random generator for dns_header id */
  time_t t;
  srand((unsigned) time(&t));

  unsigned char header_buffer[12];
  struct dns_header *header = NULL;
  header = (struct dns_header *)&header_buffer;
  header->id = (unsigned short) rand();
  header->qr = 0;
  header->opcode = 0;
  header->aa = 0;
  header->tc = 0;
  header->rd = 1;
  header->ra = 0;
  header->z = 0;
  header->rcode = 0;
  header->qdcount = htons(1);
  header->ancount = 0;
  header->nscount = 0;
  header->arcount = 0;

  struct label *labels = malloc(token_count * sizeof(*labels));
  split_to_labels(labels, name_to_query);
  char buf[sizeof(labels) / sizeof(struct label)];
  for(int i = 0; i < token_count; i++) {
    printf("%hu%s\t", labels[i].length, labels[i].domain_name);

  }

  unsigned char question_buffer[255];
  struct dns_question *question = NULL;
  question = (struct question *)&question_buffer;
  question->qname =
};


