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
	printf("len:%d, data =%p\n",len,data);		
	for (idx=0; idx<len && idx<1000;idx++){
		printf("%s",(idx%32==0)?"\n":(idx%8==0)?" ":(idx%4==0)?"-":"");
		printf("%02x",data[idx]);		
	}	
}
int handle_frame(struct tpacket_hdr* thdr, struct sockaddr_ll* saddr, char* l2, char* l3){
	printf("dummy print code\n");
	printf("dummy print code 2\n");
	dump_hex(thdr->tp_len,l2);
	printf("\nget frame! l2=%p,l3=%p,thdr=%p, saddr=%p ,data:\n",l2,l3,thdr,saddr);
	return 0;
}

int main(){
		
	int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));//create a af_packet socket with all 
	if (fd == -1) {
		perror("socket");
		exit(1);
	}

	struct tpacket_req req = {0};
	int snaplen=1500;
	req.tp_frame_size = TPACKET_ALIGN(TPACKET_HDRLEN + ETH_HLEN) + TPACKET_ALIGN(snaplen);
	req.tp_block_size = sysconf(_SC_PAGESIZE);
	while (req.tp_block_size < req.tp_frame_size) {
		req.tp_block_size <<= 1;
	}
	req.tp_block_nr = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) / (2 * req.tp_block_size);
	size_t frames_per_buffer = req.tp_block_size / req.tp_frame_size;
	req.tp_frame_nr = req.tp_block_nr * frames_per_buffer;
	if (setsockopt(fd, SOL_PACKET, PACKET_RX_RING, &req, sizeof(req))==-1) {
		perror("setsockopt");
		exit(1);
	}

	size_t rx_ring_size = req.tp_block_nr * req.tp_block_size;
	char* rx_ring = mmap(0, rx_ring_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	struct pollfd fds[1] = {0};
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	size_t frame_idx = 0;
	char* frame_ptr = rx_ring;

	while (1) {
		struct tpacket_hdr* tphdr = (struct tpacket_hdr*)frame_ptr;
		while (!(tphdr->tp_status & TP_STATUS_USER)) {
			if (poll(fds, 1, -1) == -1) {
				perror("poll");
				exit(1);
			}
		}

		struct sockaddr_ll* addr = (struct sockaddr_ll*)(frame_ptr + TPACKET_HDRLEN - sizeof(struct sockaddr_ll));
		char* l2content = frame_ptr + tphdr->tp_mac;
		char* l3content = frame_ptr + tphdr->tp_net;
		handle_frame(tphdr, addr, l2content, l3content);

		frame_idx = (frame_idx + 1) % req.tp_frame_nr;
		int buffer_idx = frame_idx / frames_per_buffer;
		char* buffer_ptr = rx_ring + buffer_idx * req.tp_block_size;
		int frame_idx_diff = frame_idx % frames_per_buffer;
		frame_ptr = buffer_ptr + frame_idx_diff * req.tp_frame_size;
	}
	return 0;
}
