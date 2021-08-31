#include "ingenic_oss_output.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define UNIX_DOMAIN "/var/run/audio_socket"
static int mSocketFd = -1;

void open_ingenic_audio_oss_output(char * mode,int mode_size){
    int fd;
    struct sockaddr_un ser_addr;  
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sun_family = AF_UNIX;
    strcpy(ser_addr.sun_path,UNIX_DOMAIN); 
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);//clivia 180429
    if(fd < 0){
	printf("socket error: %s\n",strerror(errno));	
        return;
    }
    int ret = connect(fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if(ret < 0){
	printf("connect socket error: %s\n",strerror(errno));
	return;
    }
    ret = send(fd, mode, mode_size, 0);
    if(ret <= 0)
	    printf("send buffer data error: %s\n",strerror(errno));	
    mSocketFd = fd;
}

void close_ingenic_audio_oss_output(){
    if(mSocketFd > 0){
	    close(mSocketFd);
	    mSocketFd = -1;
    }
}
int ingenic_audio_oss_write(const char* data,int size){
    if(mSocketFd < 0){
//	printf("ERROR:1111111111111 socket is closed");
	return -1;
    }

    int ret = send(mSocketFd, data, size, 0);
    if(ret < size){
	printf("ERROR:send failed ret=%d size=%d",ret,size);
    }

    return ret;   
}

