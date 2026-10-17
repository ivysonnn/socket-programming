#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <strings.h>
#include <time.h>

int main(int argc, char **argv) { 
	int server_fd, new_socket_fd;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	socklen_t addrlen = sizeof(server_address);
	socklen_t client_addrlen;
	int port;
	char client_ip[INET_ADDRSTRLEN];
	time_t now;
	char timestamp[20];

	if (argc != 2) {
        fprintf(stderr, "Uso: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

	port = atoi(argv[1]);

	bzero(&server_address, addrlen);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); 

	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket creation failed");
			return EXIT_FAILURE;
	}

	if((bind(server_fd, (struct sockaddr*)&server_address, addrlen)) < 0) {
		perror("binding failed");
		return EXIT_FAILURE;
	}

	listen(server_fd, 5);

	now = time(NULL);
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
	printf("[%s] server listening on port %d...\n", timestamp, port);
	fflush(stdout);

	for( ; ; ) {
		client_addrlen = sizeof(client_address);
		if((new_socket_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_addrlen)) < 0) {
			perror("accepting failed");
			return EXIT_FAILURE;
		}

		if(inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip)) == NULL) {
			perror("error converting client address");
			close(new_socket_fd);
			continue;
		}

		now = time(NULL);
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
		printf("[%s] cliente connected: %s:%d (fd %d)\n",
		       timestamp, client_ip, ntohs(client_address.sin_port), new_socket_fd);
		fflush(stdout);

		shutdown(new_socket_fd, 2);
		close(new_socket_fd);
	}

	close(server_fd);

	return EXIT_SUCCESS; 
}
