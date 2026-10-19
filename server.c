#include <netinet/in.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <strings.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#define TIMESTAMP_SIZE 20
#define MAX_CLIENTS 8
#define BUFFER_SIZE 1024

uint clients_sock[MAX_CLIENTS] = {0};
pthread_mutex_t client_mut = PTHREAD_MUTEX_INITIALIZER;


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
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
}

static int accept_client_connection(uint server_fd, struct sockaddr_in *client_address, socklen_t *client_addrlen, char *client_ip)
{
    int new_socket_fd = accept(server_fd, (struct sockaddr *)client_address, client_addrlen);
    if (new_socket_fd < 0) {
        perror("accepting failed");
        return -1;
    }

	pthread_mutex_lock(&client_mut);
	for(uint i = 0; i < MAX_CLIENTS; i++) {
		if(clients_sock[i] == 0) {
			clients_sock[i] = new_socket_fd;
			break;
		}
	}
	pthread_mutex_unlock(&client_mut);

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
    printf("[%s] client connected: %s:%d (fd %d)\n", timestamp, client_ip, client_port, client_fd);
    fflush(stdout);
}

static void broadcast(char* msg, uint sender_sock)
{
	pthread_mutex_lock(&client_mut);
	for(uint i = 0; i < MAX_CLIENTS; i++) {
		if(clients_sock[i] !=0 && clients_sock[i] != sender_sock) {
			send(clients_sock[i], msg, strlen(msg), 0);
		}
	}
	pthread_mutex_unlock(&client_mut);
}

static void* handle_client(void* client_fd) // this weird cause needs to be a function pointer to pass to pthread_create
{
	uint sock = *(uint*)client_fd;
	char buffer[BUFFER_SIZE] = {0};
	uint valread;
	char timestamp[TIMESTAMP_SIZE];

	while((valread = read(sock, buffer, BUFFER_SIZE)) > 0) {
		buffer[valread] = '\0';
		char buff[sizeof(buffer)];
		strcpy(buff, buffer);
		buff[strcspn(buff, "\n")] = '\0';
		
		
		if (strcmp(buffer, "exit\n") == 0) {
			get_timestamp(timestamp, sizeof(timestamp));
            printf("[%s] client disconnected: fd %d\n", timestamp, sock);
            break;
        }

		get_timestamp(timestamp, sizeof(timestamp));
		printf("[%s] received '%s' from fd %d\n", timestamp, buff, sock);
		broadcast(buffer, sock);
	}

	pthread_mutex_lock(&client_mut);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients_sock[i] == sock) {
            clients_sock[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&client_mut);

	close(sock);
	free(client_fd);

	return NULL;
}

int main(int argc, char **argv)
{
    int server_fd, new_socket_fd;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(server_address);
    socklen_t client_addrlen;
	pthread_t thread_id;
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
    
		uint *new_sock = malloc(sizeof(int));
		*new_sock = new_socket_fd;

		if(pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
			perror("error creating thread");
			return EXIT_FAILURE;
		}

		pthread_detach(thread_id);
	}
    close(server_fd);
    return EXIT_SUCCESS;
}
