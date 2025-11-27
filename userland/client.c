#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <stdio.h>

int main(void) {
    i32 ret = -1;
	struct sockaddr_in daddr;
    i32 fd;

	fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (fd < 0) {
		perror("socket failed");
		_exit(1);
	}

	daddr.sin_family = AF_INET;
	daddr.sin_port = htons(9889);
	daddr.sin_addr.s_addr = inet_addr("192.168.0.115");
	
#define TRUE
#ifdef TRUE
    ret = connect(fd, (struct sockaddr *)&daddr, sizeof(daddr));
    if (ret < 0) {
        perror("connect(2) failed");
        _exit(1);
    }

    ret = write(fd, "text message", 12);
    //ret = send(fd, "text messsage", 12, 0);
    if (ret < 0) {
        perror("write(2) failed");
        _exit(1);
    }
#else
    ret = sendto(fd, "hey", 3, 0, (struct sockaddr *)&daddr, sizeof(daddr));
    if (ret < 0) {
        perror("sendto(2) failed");
        _exit(1);
    }
#endif
    printf("sent %d bytes\n", ret);

    ret = close(fd);
    if (ret < 0) {
        perror("close(2) failed");
        _exit(1);
    }

	return 0;
}
