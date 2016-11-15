#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>
#include <errno.h>
#include <error.h>
#include <sys/epoll.h>
// #include "pdma-ioctl.h"
#include "tool.h"
// #define DEBUG
#define MATLAB 10000
#define EPOLLEVENTS 100
struct epoll_event events[EPOLLEVENTS];
struct epoll_event evx;
void add_event(int epollfd, int fd, int state){
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}
int open_port(char *port_path);
int set_port_nonblocking(int val, int fd);
int main(int argc, char const *argv[]){
	// FILE *inputdata = fopen("guest_use_pdma_example/inputData.txt", "a+")
	int i, ret, second, usecond;
	struct timeval start, stop;
	int fd, epollfd;
	int mode = O_RDWR;
	char *inputfile = "inputData.txt";
	int K_byte, code_bit, encodelen;
	struct pollfd fds[1];
	K_byte = getfilesize("inputData.txt");
	code_bit = (K_byte << 3) + 4;
	encodelen = code_bit << 2;
	unsigned char *outputBuf, *readBuf;
	outputBuf = (unsigned char *)malloc(encodelen);
	readBuf = (unsigned char *)malloc(4096);
	memset(outputBuf, 0, encodelen);
	getoutput(inputfile, K_byte, outputBuf);
	// inputBuf = (unsigned char *)malloc()
	
	for(i = 0; i < encodelen; i++){
		if(outputBuf[i] == 1)
			outputBuf[i] = 0x7f;
		else
			outputBuf[i] = 0x80;
		if((i + 1) % 8 == 0)
			outputBuf[i] = 0x01;
	}

#ifdef DEBUG
	FILE *tt = fopen("guest_debug.txt", "w");

	for(i = 0; i < encodelen; i++){
		if(i % 8 == 0)
			fprintf(tt, "\n");
		fprintf(tt, "%x ", outputBuf[i]);
	}
#endif
	
	outputBuf[encodelen - 1] = 0x81;
	printf("the encodelen is %d\n", encodelen);
	fd = open("/dev/virtio-ports/guestport", O_RDWR);
	// fd = open_port("/dev/virtio-ports/guestport");
	if(fd < 0){
		printf("open guestport failed\n");
		return -1;
	}
	// fds[0].fd = fd;
	// fds[0].events = POLLIN;
	epollfd = epoll_create(1024);
	add_event(epollfd, fd, EPOLLIN );
	printf("start send data to guestport\n");
	// if(ret < 0){
	// 	close(fd);
	// 	printf("poll /dev/virtio-ports/guestport\n");		
	// }
#ifdef MATLAB
	FILE *matlab = fopen("matlab.txt","w+");
	for(i = 0; i < MATLAB; i++){
		// printf("this is %d\n", i);
#endif
	
	gettimeofday(&start, 0);
	// ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
	// if(events[0].events & EPOLLOUT)
	ret = write(fd, outputBuf, encodelen);
	
	// if(ret < 0){
	// 	printf("write guestport failed\n");
	// 	return ret;
	// }
	
	// ret = poll(fds, 1, 0);
	
	// if(ret < 0){
	// 	printf("poll failed\n");
	// }	
	// if(fds[0].revents & POLLIN)
	// evx.events = EPOLLIN;
	// evx.data.fd = fd;
	// epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &evx);
	ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
	if(events[0].events & EPOLLIN){
		ret = read(fd, readBuf,  K_byte);
		if(ret < 0){
			fprintf(stderr, "error number %d\n", errno);
		}	
	}
	gettimeofday(&stop, 0);	

	// evx.events = EPOLLOUT;
	// evx.data.fd = fd;
	// epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &evx);
	if(stop.tv_usec < start.tv_usec){
		second = stop.tv_sec - start.tv_sec -1;
		usecond = 1000000 + stop.tv_usec - start.tv_usec;
	}else {
		second = stop.tv_sec - start.tv_sec;
		usecond = stop.tv_usec - start.tv_usec;
	}

	// 
	if(ret != 720){
		printf("read is error. Read length is %d\n", ret);
		// return -1;
	}
#ifdef MATLAB	
	if(ret <= 0){
		printf("ret: %d\n", ret);
	}
	if(usecond > 500){
		printf("usecond: %d, i: %d\n", usecond, i);
	}
	
	// printf("%s\n", readBuf);
	usleep(100);
	fprintf(matlab, "%d \n", usecond); 	
}
#endif

// #ifdef MATLAB
// 	// fclose(matlab);
// #endif

#ifdef DEBUG
	printf("readBuf is \n");
	for(i = 0; i < K_byte; i++){
		if(i % 8 == 0)
			printf("\n");
		printf("%d ", readBuf[i]);
	}
	printf("Final time is %ds %dus\n", second, usecond);
#endif
	close(fd);
	free(outputBuf);
	free(readBuf);
	return 0;
}
int open_port(char *port_path){
	int fd, ret;
	fd = open(port_path, O_RDWR);
	if(fd == -1){
		printf("open port failed\n");
		return errno;
	}
	ret = fcntl(fd, F_SETOWN, getpid());
	if(ret < 0){
		perror("F_SETOWN");
	}
	ret = fcntl(fd, F_GETFL);
	ret = fcntl(fd, F_SETFL, ret | O_ASYNC );
	if(ret < 0){
		perror("F_SETFL");
	}
	return fd;
}
int set_port_nonblocking(int val, int fd){
	int ret, flags;

	flags = fcntl(fd, F_GETFL);
	if(flags == -1){
		return errno;
	}
	if(val){
		flags |= O_NONBLOCK;
	}else flags &= ~O_NONBLOCK;

	ret = fcntl(fd, F_SETFL, flags);
	if(ret == -1)
		return errno;
	return 0;


}
