#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct label {
  unsigned short length;
  char *domain_name;
};

char *label_to_str (struct label label) {
  size_t length = 0;
  length = snprintf(NULL, length, "%hu%s", label.length, label.domain_name);

  // add one to length to null terminate the string
  char *label_str = calloc(1, length + 1);
  if (!label_str) {
    fprintf(stderr, "error: memory allocation failed");
    return;
  }

  size_t write_length = snprintf(label_str, length + 1, "%hu%s", label.length, label.domain_name);
  if(write_length > length) {
    fprintf(stderr, "error: snprintf truncated result.");
    return NULL;
  }
  return label_str;
}

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
}

int main(void) {
  char name_to_query[] = "foo.bar.google.com";
  const char DELIMITER = '.';
  short token_count = 0;
  for (int i = 0; i < strlen(name_to_query); i++) {
    if (name_to_query[i] == DELIMITER) {
      token_count++;

    }
  }
  struct label *labels = malloc((token_count + 1) * sizeof(*labels));

  split_to_labels(labels, name_to_query, token_count);

  for (int i = 0; i < 4; i++) {
    printf("%s", label_to_str(labels[i]));
  }

  return 0;
}
