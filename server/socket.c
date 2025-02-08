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

#define PORT "9000"
#define FILE_PATH "/var/tmp/aesdsocketdata"

int server_fd; //global to be able to use in signal handler

void handle_signal(int sig) {
    syslog(LOG_INFO, "Caught signal, exiting");

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
    ssize_t msg_bytes;
    char buff[1024] = {'\0'};
    socklen_t client_addr_size;
    FILE *fp;
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

        if ((fp = fopen(FILE_PATH, "a")) == NULL)
        {
            perror("fopen");
            close(accept_fd);
            continue;   
        }

        while ((msg_bytes = recv(accept_fd, buff, 1023, 0)) > 0)
        {
            buff[msg_bytes] = '\0';

            fputs(buff, fp);
            fflush(fp);

            if (strchr(buff, '\n'))
                break;
        }

        fclose(fp);

        if ((fp = fopen(FILE_PATH, "r")) == NULL)
        {
            perror("fopen");
            close(accept_fd);
            continue;   
        }

        while ((msg_bytes = fread(buff, 1, sizeof(buff), fp)) > 0)
        {
            send(accept_fd, buff, msg_bytes, 0);
        }

        fclose(fp);

        syslog(LOG_INFO, "closed connection from %s", client_ip);
        printf("Closed connection from %s\n", client_ip);

        close(accept_fd);

    }

    close(server_fd);
    closelog(); 

    return 0;
}
