
// Usage: ./main host port request_count thread_count

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <pthread.h>
#include <algorithm>
#include <cstring>
#include <stdio.h>
#include <string.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int request_count, thread_count, requests_per_thread;
int *response_times;
char **arguments;

void error(const char *msg) { perror(msg); exit(0); }
//--------------------------------------------------------------------------------
int check_if_header_ended(char *buffer) {
  const char *search_string1 = "\r\n\r\n";
  const char *search_string2 = "\n\r\n\r";

  char *find_buffer;
  int ofset = -1;

  find_buffer = strstr(buffer, search_string1);
  if (find_buffer != NULL) {
    // printf("Header ended\n");
    return 1;
  }
  else {
    find_buffer = strstr(buffer, search_string2);
    if (find_buffer != NULL) {
      // printf("Header ended\n");
      return 1;
    }
  }
  return 0;
}
//--------------------------------------------------------------------------------

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

  // printf("Content length: %d\n", atoi(content_length));

  return atoi(content_length);
}

//--------------------------------------------------------------------------------

int make_request(char *argv[]) {
  char *host = argv[1];
  int portno = atoi(argv[2]);
  // int portno = 80;
  // char *host = "api.somesite.com";
  char *user_agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X x.y; rv:42.0) Gecko/20100101 Firefox/42.0";
  char *message_format = "PUT /apikey=%s&command=%s HTTP/1.0\r\n\r\n";
  char message[65535];

  struct hostent *server;
  struct sockaddr_in serv_addr;
  int sockfd, bytes, sent, received, total;

  sprintf(message,message_format,host,user_agent);

  //utworzenie gniazda TCP
  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error("ERROR opening socket");

  server = gethostbyname(host);
  if (server == NULL) error("ERROR, no such host");

  memset(&serv_addr,0,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

  if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    error("ERROR connecting");

  //wysłanie żądania
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

  int content_length = -1;
  int oneBajt = 1;
  char response_header[oneBajt];
  char whole_header[1600];

  //odbiór nagłówka
  memset(response_header, 0, sizeof(response_header));
  memset(whole_header, 0, sizeof(whole_header));
  // total = sizeof(whole_header)-1;
  received = 0;
  int licznik=0;
  do {
    bytes = read(sockfd, response_header, oneBajt);
    memcpy(whole_header + licznik*oneBajt, response_header, oneBajt); licznik++;
    // printf("bytes = %d   |   koniec=%d\n", bytes, check_if_header_ended(whole_header));
  } while (!check_if_header_ended(whole_header) && bytes==oneBajt);


    content_length = find_content_length(whole_header);
    char response_body[content_length];

  received = 0;
  do {
    bytes = read(sockfd, response_body+received, content_length-received);
    if (bytes < 0)
      error("ERROR reading response from socket!");

    if (bytes == content_length)
      break;
    received+=bytes;

  } while (received < content_length);

  gettimeofday(&t2, NULL);

  // compute and print the elapsed time in millisec
  elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000000.0;      // sec to ms
  elapsedTime += (t2.tv_usec - t1.tv_usec);   // us to ms

  // printf("Elapsed time: %.0f us\n", elapsedTime);

  close(sockfd);

  // delete[] response_body;

  return elapsedTime;
}
//--------------------------------------------------------------------------------


void *make_requests(void *arg) {
  int thread_start_index = *((int *)arg) * requests_per_thread;
  int thread_end_index = thread_start_index + requests_per_thread;

  for (int i = thread_start_index; i < thread_end_index; i++) {
    printf("Teraz dziala watek: %d, zadanie: %d\n", *((int *)arg), i-thread_start_index);
    response_times[i] = make_request(arguments);
  }
  return NULL;
}

//--------------------------------------------------------------------------------

void calculateResult() {
  float avg_time = 0;
  for (int i = 0; i < thread_count * requests_per_thread; i++) {
    avg_time += response_times[i];
  }

  std::sort(response_times, response_times + thread_count * requests_per_thread);
  avg_time = avg_time / (requests_per_thread * thread_count);

  float median_time = response_times[thread_count * requests_per_thread / 2];
  float min_time = response_times[0];
  float max_time = response_times[thread_count * requests_per_thread - 1];

  printf("Min time:\t");
  printf(ANSI_COLOR_GREEN "%.0f us\n" ANSI_COLOR_RESET, min_time);
  printf("Average time:\t");
  printf(ANSI_COLOR_YELLOW "%.0f us\n" ANSI_COLOR_RESET, avg_time);
  printf("Max time:\t");
  printf(ANSI_COLOR_RED "%.0f us\n" ANSI_COLOR_RESET, max_time);
  printf("Median time:\t");
  printf(ANSI_COLOR_MAGENTA "%.0f us\n" ANSI_COLOR_RESET, median_time);

}

//--------------------------------------------------------------------------------


int main(int argc, char *argv[]) {
  request_count = atoi(argv[3]);
  thread_count = atoi(argv[4]);
  arguments = argv;

  requests_per_thread = request_count / thread_count;
  response_times = new int[thread_count * requests_per_thread];
  printf("Request count:\t");
  printf(ANSI_COLOR_CYAN "%d\n" ANSI_COLOR_RESET, request_count);
  printf("Thread count:\t");
  printf(ANSI_COLOR_CYAN "%d\n" ANSI_COLOR_RESET, thread_count);
  pthread_t *thread_ids = new pthread_t[thread_count];
  int *thread_indices = new int[thread_count];

  struct timeval t1, t2;
  double elapsedTime;

  gettimeofday(&t1, NULL);

  for (int i = 0; i < thread_count; i++) {
    thread_indices[i] = i;
    if (pthread_create(thread_ids + i, NULL, make_requests, thread_indices + i)) {
      printf("Error while creating thread\n");
      return 1;
    }
  }

  for (int i = 0; i < thread_count; i++) {
    if (pthread_join(thread_ids[i], NULL)) {
      printf("Error while joining thread\n");
      return 1;
    }
  }

  gettimeofday(&t2, NULL);

  elapsedTime = (t2.tv_sec - t1.tv_sec);                  // sec to ms
  elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;   // us to ms

  printf("Elapsed time:\t%.2f s\n", elapsedTime);

  calculateResult();

  delete[] response_times;
  delete[] thread_indices;


  return 0;
}
