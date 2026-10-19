#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>

#define BUFF_SIZE 1024

static uint create_client_socket(void)
{
    uint socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    return socket_fd; 
}

static void configure_server_address(struct sockaddr_in *server_address, const char *ip, uint port)
{
    bzero(server_address, sizeof(*server_address));
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_address->sin_addr) < 1) {
        perror("error converting ip address");
        exit(EXIT_FAILURE);
    }
}

static void connect_to_server(int socket_fd, struct sockaddr_in *server_address, socklen_t addrlen)
{
    if (connect(socket_fd, (struct sockaddr *)server_address, addrlen) < 0) {
        perror("error connecting to server");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    uint socket_fd;
    struct sockaddr_in server_address;
    socklen_t addrlen = sizeof(server_address);
    const char *ip = NULL;
    uint port;
	char buffer[BUFF_SIZE] = {0};

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <server-ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    ip = argv[1];
    port = atoi(argv[2]);

    socket_fd = create_client_socket();
    configure_server_address(&server_address, ip, port);
    connect_to_server(socket_fd, &server_address, addrlen);

	for( ; ; ) {
		printf("You: ");
		fgets(buffer, BUFF_SIZE, stdin);

		send(socket_fd, buffer, strlen(buffer), 0);

		if(strcmp(buffer, "/exit\n") == 0) {
			printf("EXITED");
			break;
		}
	}

    close(socket_fd);
    return EXIT_SUCCESS;
}
