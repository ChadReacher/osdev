#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 

int main(void) {
	i32 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket failed");
		_exit(1);
	}
	struct sockaddr_in addr, client;
	socklen_t len = sizeof(client);
	char buffer[1024] = {0};

	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		perror("bind failed");
		_exit(1);
	}

	while (1) {
		i32 read = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client, &len);
		if (read > 0) {
			buffer[read] = 0;
			if (strcmp(buffer, "ping") == 0) {
				sendto(sockfd, "pong", 4, 0, (struct sockaddr *)&client, len);
			} else {
				sendto(sockfd, "echo", 4, 0, (struct sockaddr *)&client, len);
			}
		}
	}

	return 0;
}
