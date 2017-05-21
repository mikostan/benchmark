#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <pthread.h>


// Usage: ./main host port request_count thread_count

void error(const char *msg) { perror(msg); exit(0); }

int check_if_header_ended(char *buffer) {
  const char *search_string1 = "\r\n\r\n";
  const char *search_string2 = "\n\r\n\r";

  char *find_buffer;
  int ofset = -1;

  find_buffer = strstr(buffer, search_string1);
  if (find_buffer != NULL) {
    printf("Header ended\n");
  }
  else {
    find_buffer = strstr(buffer, search_string2);
    if (find_buffer != NULL) {
      printf("Header ended\n");
    }
  }
  return ofset;
}

int find_content_length(char *buffer) {
  const char *search_string1 = "Content-Length:";
  const char *search_string2 = "content-length:";

  char *find_buffer;
  char *find_buffer2;
  int ofset = -1;
  int ofset2 = -1;
  char content_length[10];

  find_buffer = strstr(buffer, search_string1);
  if (find_buffer != NULL) {
    ofset = find_buffer - buffer;
    ofset += strlen(search_string2);

    find_buffer2 = strstr(find_buffer, "\r\n");

    ofset2 = find_buffer2 - find_buffer - 16;

    memcpy(content_length, find_buffer + 15, ofset2 + 1);
  }
  else {
    find_buffer = strstr(buffer, search_string2);
    if (find_buffer != NULL) {
      ofset = find_buffer - buffer;
      ofset += strlen(search_string2);

      find_buffer2 = strstr(find_buffer, "\r\n");

      ofset2 = find_buffer2 - find_buffer - 16;

      memcpy(content_length, find_buffer + 15, ofset2 + 1);
    }
  }

  printf("Content length: %d\n", atoi(content_length));

  return atoi(content_length);
}



int make_request(char *argv[]) {
  int portno = atoi(argv[2]);
  char *host = argv[1];
  char *user_agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X x.y; rv:42.0) Gecko/20100101 Firefox/42.0";


  char *message_format = "GET / HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
  char message[65535];

  struct hostent *server;
  struct sockaddr_in serv_addr;
  int sockfd, bytes, sent, received, total;
  char response_header[512];

  sprintf(message,message_format,host,user_agent);
  // printf("Request:\n%s\n",message);

  // printf("Host: %s, port: %d\n\n", host, portno);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error("ERROR opening socket");

  server = gethostbyname(host);
  if (server == NULL) error("ERROR, no such host");

  memset(&serv_addr,0,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

  if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    error("ERROR connecting");

  total = strlen(message);
  sent = 0;
  do {
    bytes = write(sockfd,message+sent,total-sent);
    if (bytes < 0)
      error("ERROR writing message to socket");
    if (bytes == 0)
      break;
    sent+=bytes;
  } while (sent < total);


  struct timeval t1, t2;
  double elapsedTime;

  gettimeofday(&t1, NULL);




  char *response_body = NULL;
  int content_length = -1;




  memset(response_header,0,sizeof(response_header));
  total = sizeof(response_header)-1;
  received = 0;
  do {
    bytes = read(sockfd, response_header, 511);
    // printf("%s\n", response_header);

    check_if_header_ended(response_header);
    content_length = find_content_length(response_header);


    // printf("Content length: %d\n", content_length);
    if (content_length > 0) {
      response_body = new char[content_length];
    }

    if (bytes < 0)
      error("ERROR reading response from socket");
    if (bytes < 511 || check_if_header_ended(response_header))
      break;
    received+=bytes;

  } while (1);

  // printf("Reading response body\n");

  do {
    bytes = read(sockfd, response_body, content_length);
    // printf("Received %d bytes\n", bytes);

    if (bytes <= 0)
      error("ERROR reading response from socket");
    if (bytes < content_length)
      break;
    received+=bytes;

  } while (1);

  // printf("End read\n");

  gettimeofday(&t2, NULL);

  // compute and print the elapsed time in millisec
  elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000000.0;      // sec to ms
  elapsedTime += (t2.tv_usec - t1.tv_usec);   // us to ms

  // printf("Elapsed time: %.0f us\n", elapsedTime);

  close(sockfd);

  delete[] response_body;

  return elapsedTime;
}


int request_count, thread_count, requests_per_thread;
int **response_times;

char **arguments;

void *make_requests(void *arg) {
  int *thread_index = (int *)arg;
  printf("Thread %d\n", *thread_index);

  for (int i = 0; i < requests_per_thread; i++) {
    response_times[*thread_index][i] = make_request(arguments);
  }

  return NULL;
}


int main(int argc, char *argv[]) {
  request_count = atoi(argv[3]);
  thread_count = atoi(argv[4]);

  arguments = argv;

  requests_per_thread = request_count / thread_count;

  response_times = new int*[thread_count];

  for (int i = 0; i < thread_count; i++) {
    response_times[i] = new int[requests_per_thread];
  }

  pthread_t *thread_ids = new pthread_t[thread_count];
  int *thread_indices = new int[thread_count];


  for (int i = 0; i < thread_count; i++) {
    thread_indices[i] = i;
    if (pthread_create(thread_ids + i, NULL, make_requests, thread_indices + i)) {
      printf("error while creating thread\n");
      return 1;
    }
  }

  sleep(1);


  printf("Request count: %d\n", request_count);
  printf("Thread count: %d\n", thread_count);


  for (int i = 0; i < thread_count; i++) {
    for (int j = 0; j < requests_per_thread; j++) {
      printf("Thread = %d, Time = %d us\n", i, response_times[i][j]);
    }
  }


  for (int i = 0; i < thread_count; i++) {
    delete[] response_times[i];
  }
  delete[] response_times;
  delete[] thread_indices;


  return 0;
}
