#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <strings.h>
#include <time.h>
#include <string.h>

#define BACKLOG 5
#define TIMESTAMP_SIZE 20

static void get_timestamp(char *timestamp, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(timestamp, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

static int create_server_socket(void)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

static void configure_server_address(struct sockaddr_in *server_address, uint port)
{
    bzero(server_address, sizeof(*server_address));
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(port);
    server_address->sin_addr.s_addr = htonl(INADDR_ANY);
}

static void bind_server_socket(uint server_fd, struct sockaddr_in *server_address, socklen_t addrlen)
{
    if (bind(server_fd, (struct sockaddr *)server_address, addrlen) < 0) {
        perror("binding failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
}

static void listen_server_socket(uint server_fd)
{
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
}

static int accept_client_connection(uint server_fd,
                                   struct sockaddr_in *client_address,
                                   socklen_t *client_addrlen,
                                   char *client_ip)
{
    int new_socket_fd = accept(server_fd, (struct sockaddr *)client_address, client_addrlen);
    if (new_socket_fd < 0) {
        perror("accepting failed");
        return -1;
    }

    if (inet_ntop(AF_INET, &client_address->sin_addr, client_ip, INET_ADDRSTRLEN) == NULL) {
        perror("error converting client address");
        close(new_socket_fd);
        return -1;
    }

    return new_socket_fd;
}

static void log_server_listening(uint port)
{
    char timestamp[TIMESTAMP_SIZE];
    get_timestamp(timestamp, sizeof(timestamp));
    printf("[%s] server listening on port %d...\n", timestamp, port);
    fflush(stdout);
}

static void log_client_connected(const char *client_ip, int client_port, int client_fd)
{
    char timestamp[TIMESTAMP_SIZE];
    get_timestamp(timestamp, sizeof(timestamp));
    printf("[%s] client connected: %s:%d (fd %d)\n",
           timestamp, client_ip, client_port, client_fd);
    fflush(stdout);
}

static void handle_client(int client_fd) // messaging part for later
{
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
}

int main(int argc, char **argv)
{
    uint server_fd, new_socket_fd;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(server_address);
    socklen_t client_addrlen;
    uint port;
    char client_ip[INET_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    port = atoi(argv[1]);

    configure_server_address(&server_address, port);

    server_fd = create_server_socket();
    bind_server_socket(server_fd, &server_address, addrlen);
    listen_server_socket(server_fd);

    log_server_listening(port);

    for (;;) {
        client_addrlen = sizeof(client_address);
        new_socket_fd = accept_client_connection(server_fd, &client_address, &client_addrlen, client_ip);

        if (new_socket_fd < 0) {
            continue;
        }

        log_client_connected(client_ip, ntohs(client_address.sin_port), new_socket_fd);
        handle_client(new_socket_fd);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
