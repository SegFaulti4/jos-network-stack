#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

int main(void) {
	int sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct hostent *me = gethostbyname("192.168.123.2");//gethostbyname("192.168.123.2");
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr = *(struct in_addr *)me->h_addr_list[0],
		.sin_port = htons(8888)
	};

	getchar();
	while (1) {
	//for (int i = 0; i < 70; i++) {
		char buf[] = "Hello\nI am glad to send a very long UDP package\n";
		sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, sizeof(addr));
		printf("send\n");
		getchar();
	}
	return 0;
}
