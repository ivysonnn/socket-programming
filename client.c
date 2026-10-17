#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <strings.h>

int main(int argc, char **argv) { 
	uint socket_fd;
	struct sockaddr_in server_address;
	uint addrlen = sizeof(server_address);
	uint port;
	const char* ip = NULL;

	if (argc != 3) {
        fprintf(stderr, "Uso: %s <server-ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
	
	
	ip = argv[1];
	port = atoi(argv[2]);

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&server_address, addrlen);
	if(inet_pton(AF_INET, ip, &server_address.sin_addr) < 1) {
		perror("error converting ip address");
		return EXIT_FAILURE;
	}
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);

	if(connect(socket_fd, (struct sockaddr *)&server_address, addrlen) < 0) {
		perror("error connecting to server");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS; 
}
