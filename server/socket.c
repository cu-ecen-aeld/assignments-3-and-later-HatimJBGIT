#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/queue.h>
#include <stdbool.h>
#include <time.h>

#define PORT "9000"
#define FILE_PATH "/var/tmp/aesdsocketdata"

time_t t;
struct tm *tmp;
char time_str[200];
char time_foramt_RFC2822[] = "timestamp:%a, %d %b %Y %T %z\n";
pthread_t thread_id_time;
volatile sig_atomic_t exit_flag = 0;

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct thread_dataT
{
    int accept_fd;
    char client_ip[INET_ADDRSTRLEN];
    bool thread_complete;
    pthread_t thread_id;

    SLIST_ENTRY(thread_dataT) next; //singly linked list
}thread_dataT;

SLIST_HEAD(thread_list, thread_dataT) head;

void *thread_func(void *arg)
{
    thread_dataT *l_thread_info = (thread_dataT *)arg;
    FILE *fp;
    ssize_t msg_bytes;
    char buff[1024] = {'\0'};

    while ((msg_bytes = recv(l_thread_info->accept_fd, buff, 1023, 0)) > 0)
    {
        buff[msg_bytes] = '\0';

        pthread_mutex_lock(&file_mutex); // mutext to lock shared resource

        if ((fp = fopen(FILE_PATH, "a")) != NULL)
        {
            fputs(buff, fp);
            fflush(fp);
            fclose(fp);
        }

        pthread_mutex_unlock(&file_mutex);

        if (strchr(buff, '\n'))
            break;
    }

    pthread_mutex_lock(&file_mutex);

    if ((fp = fopen(FILE_PATH, "r")) != NULL)
    {
        while ((msg_bytes = fread(buff, 1, sizeof(buff), fp)) > 0)
        {
            send(l_thread_info->accept_fd, buff, msg_bytes, 0);
        }
        fclose(fp);
    }

    pthread_mutex_unlock(&file_mutex);

    syslog(LOG_INFO, "closed connection from %s", l_thread_info->client_ip);
    printf("Closed connection from %s\n", l_thread_info->client_ip);

    close(l_thread_info->accept_fd);

    l_thread_info->thread_complete = true;
    return NULL;
}

void *time_thread(void *arg)
{
    // appending time stamp in file every 10 seconds.
    FILE *fp;

    while (!exit_flag)
    {
        t = time(NULL);
        tmp = localtime(&t);
        if (tmp == NULL)
        {
            perror("localtime");
            exit(EXIT_FAILURE);
        }

        if (strftime(time_str, sizeof(time_str), time_foramt_RFC2822, tmp) == 0)
        {
            fprintf(stderr, "strftime returned 0");
            exit(EXIT_FAILURE);
        }

        //printf("%s", time_str);

        pthread_mutex_lock(&file_mutex);

        if ((fp = fopen(FILE_PATH, "a")) != NULL)
        {
            fputs(time_str, fp);
            fclose(fp);
        }

        pthread_mutex_unlock(&file_mutex);

        sleep(10); // sleep 10 seconds to update timestamp in file.
    }
    return NULL;
}

int server_fd; //global to be able to use in signal handler

void handle_signal(int sig) {
    syslog(LOG_INFO, "Caught signal, exiting");

    exit_flag = 1;
    //pthread_join(thread_id_time, NULL);

    // Close server socket
    if (server_fd != -1) {
        close(server_fd);
    }

    // Delete the file
    remove(FILE_PATH);

    // Close syslog and exit
    closelog();
    exit(0);
}

void setup_signal_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char *argv[])
{
    int accept_fd;
    int option = 1;
    char *node;
    struct addrinfo hints, *res;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;
    int daemon_mode = 0;

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }
    memset(&hints, 0, sizeof(hints));

    setup_signal_handler();

    openlog("aesdsocket", LOG_PID, LOG_USER);

    if ((server_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
        return (-1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
    {
        perror("Socket options");
        close(server_fd);
        return (-1);
    }

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    node = NULL;

    if (getaddrinfo(node, PORT, &hints, &res) == -1)
    {
        perror("getaddrinfo");
        return (-1);
    }

    if (bind(server_fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("bind");
        close(server_fd);
        freeaddrinfo(res);
        return (-1);
    }

    freeaddrinfo(res);

    if (daemon_mode) {
        daemon(0, 0);
    }

    if (listen(server_fd, 1) == -1)
    {
        perror("listen");
        close(server_fd);
        return (-1);
    }

    printf("server listening on port 9000\n");
    if (pthread_create(&thread_id_time, NULL, &time_thread, NULL) != 0)
    {
        perror("timer thread");
        exit(EXIT_FAILURE);
    }

    SLIST_INIT(&head); //initializing slist head pointer

    while (1)
    {
        client_addr_size = sizeof(client_addr);
        if ((accept_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_size)) == -1)
        {
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        struct sockaddr_in *client_sock = (struct sockaddr_in *)&client_addr;
        inet_ntop(AF_INET, &client_sock->sin_addr, client_ip, sizeof(client_ip));

        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        printf("Accepted connection from %s\n", client_ip);

        thread_dataT *l_thread_info = malloc(sizeof(thread_dataT));
        if(!l_thread_info)
        {
            perror("malloc");
            continue;
        }
        l_thread_info->accept_fd = accept_fd;
        strcpy(l_thread_info->client_ip, client_ip);
        l_thread_info->thread_complete = false;

        if (pthread_create(&l_thread_info->thread_id, NULL, &thread_func, l_thread_info) != 0)
        {
            perror("pthread_create");
            close(accept_fd);
            free(l_thread_info);
            continue;
        }

        SLIST_INSERT_HEAD(&head, l_thread_info, next);

        // Cleanup completed threads
        thread_dataT *iter, *temp;
        iter = SLIST_FIRST(&head);
        while (iter != NULL)
        {
            temp = SLIST_NEXT(iter, next);

            if (iter->thread_complete)
            {
                pthread_join(iter->thread_id, NULL);
                SLIST_REMOVE(&head, iter, thread_dataT, next);
                free(iter);
            }

            iter = temp;
        }
    }

    close(server_fd);
    closelog(); 

    return 0;
}
