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
  unsigned char *name;// domain name in octet format, no padding is used to fill, zero length octet to terminate
  unsigned short qtype;
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

int  make_dns_query_domain(char *domain)
{
    char out[256];
    char *p = domain;
    char *pout = &out[0];
    while (*p)
    {
        int size = 0;
        char *pdomain = p;
        while (*pdomain && *pdomain != '.')
        {
            size++;
            pdomain++;
        }
        pout[0] = size;
        pout ++;
        strncpy(pout, p, size);
        pout += size;
        p = pdomain;
        if ('.' == *p)
        {
            p++;
        }
    }
    *(pout) = 0;
    memcpy(domain, &out[0], pout - &out[0] + 1);
    return pout - &out[0] + 1;
}


char *label_to_str (struct label label) {
  size_t length = 0;
  length = snprintf(NULL, length, "%hu%s", label.length, label.domain_name);

  // add one to length to null terminate the string
  char *label_str = calloc(1, length + 1);
  if (!label_str) {
    fprintf(stderr, "error: memory allocation failed");
    return NULL;
  }

  size_t write_length = snprintf(label_str, length + 1, "%hu%s", label.length, label.domain_name);
  if(write_length > length) {
    fprintf(stderr, "error: snprintf truncated result.");
    return NULL;
  }
  return label_str;
};


struct header build_dns_header() {
  return (struct header) {
    .id = (unsigned short) htons(1),
    .rd = 1,
    .qdcount = htons(1),
  };
};


struct question build_dns_question(char *qname) {
  return (struct question) {
    .name = (unsigned char *) qname,
    .qtype = htons(1), // A record == 1
    .qclass = htons(1), // Internet == 1
  };
};


DNS_MESSAGE build_dns_message(char *qname) {
  return (DNS_MESSAGE) {
    .header = build_dns_header(),
    .question = build_dns_question(qname),
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

  char NAME_LENGTH = strlen(argv[1]);
  char name_to_query[NAME_LENGTH];
  strncpy(name_to_query, argv[1], NAME_LENGTH);
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
    // printf("%s", label_to_str(labels[i]));
    if (i == 0) {
      strcpy(qname, label_to_str(labels[i]));
    } else {
      strcat(qname, label_to_str(labels[i]));
    }
  }
  printf("%s", qname);

  // UDP DNS lookups have a max size of 512 bytes
  struct header dns_header = build_dns_header();
  struct question dns_question = build_dns_question(qname);
  unsigned char buffer[512];
  memcpy(buffer, &dns_header, sizeof (struct header));
  int length = strlen((char *) dns_question.name);
  memcpy(&buffer[sizeof (struct header)], dns_question.name, strlen((char *) dns_question.name));
  memcpy(&buffer[sizeof (struct header) + length], &dns_question.qtype, sizeof dns_question.qtype);
  memcpy(&buffer[sizeof (struct header) + length + sizeof dns_question.qtype], &dns_question.qclass, sizeof dns_question.qclass);
  int bytes_sent = sendto(sock, (char *)buffer, strlen((char *)buffer), 0, (struct sockaddr*)&destination, sizeof destination);
  if (bytes_sent < 0) {
    printf("\nError sending packet: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  int bytes_recv = recvfrom(sock, (char*) buffer, 512, 0, (struct sockaddr*)&destination, (socklen_t *) sizeof destination);
  if(bytes_recv < 0) {
    printf("\nError receiving packet: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  struct header *dns_h = (struct header *) buffer;
  // reader = &buffer[sizeof (DNS_MESSAGE.header) + (strlen((char *) qname)) + sizeof (DNS_MESSAGE.query)];
  // printf("\nThe response contains : ");
  // printf("\n %d Questions.",ntohs(dns_header->q_count));
  // printf("\n %d Answers.",ntohs(dns_header->ans_count));
  // printf("\n %d Authoritative Servers.",ntohs(dns_header->auth_count));
  // printf("\n %d Additional records.\n\n",ntohs(dns_header->add_count));

  close(sock); /* close the socket */
  return 0;
}
