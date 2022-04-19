#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>	
#include <arpa/inet.h>	
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <sys/socket.h>	
#include <sys/mman.h>
int dump_hex(unsigned int len, unsigned char* data){
	unsigned int idx;
	printf("len:%d, data =%p",len,data);		
	for (idx=0; idx<len && idx<1000;idx++){
		printf("%s",(idx%32==0)?"":(idx%8==0)?" ":(idx%4==0)?"-":"");
		printf("%02x",data[idx]);		
	}	
}
int handle_frame(char* buffer, ssize_t count){
	//printf("count=%lu",count);
	dump_hex(count,buffer);
	return 0;
}

int main(){		
	int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));//create a af_packet socket with all 
	if (fd == -1) {
		perror("socket");
		exit(1);
	}
	char buffer[65537];
	struct sockaddr_ll src_addr;
	socklen_t src_addr_len = sizeof(src_addr);

	while (1) {
		ssize_t count = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&src_addr, &src_addr_len);
		if (count == -1) {
			perror("recvfrom");
			exit(1);
		} else if (count == sizeof(buffer)) {
			fprintf(stderr, "frame too large for buffer: truncated\n");
		} else {
			handle_frame(buffer, count);
		}
	}
	return 0;
}
