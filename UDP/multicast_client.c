#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#define SIZE 1024
#define MAX_SIZE 32

struct sockaddr_in localSock;
socklen_t localLen;
struct ip_mreq group;
struct timeval timeout;
int portno;
char serv_ip[MAX_SIZE];

void error(char *err);

int main(int argc,char *argv[]){
    //Get host address and port number
    strcpy(serv_ip,argv[1]);
    portno = atoi(argv[2]);
    
    char buffer[SIZE];
    size_t f_size;
    double loseRate;
    
    /* Create a datagram socket on which to receive. */  
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0) error("error opening socket");

    /* Enable SO_REUSEADDR to allow multiple instances of this */
	/* application to receive copies of the multicast datagrams. */
    int reuse = 1;
    if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char *)&reuse,sizeof(reuse)) < 0)
        error("error setting SO_REUSEADDR");

    /* Bind to the port number 8010 with the IP address */
	/* specified as INADDR_ANY. */
    memset(&localSock,0,sizeof(localSock));
    localSock.sin_family = AF_INET;
    localSock.sin_addr.s_addr = INADDR_ANY;
    localSock.sin_port = htons(portno);
    if(bind(sockfd,(struct sockaddr *)&localSock,sizeof(localSock)) < 0)
        error("error binding localSock");

    /* Join the multicast group 226.1.1.1 on the local 127.0.0.1 interface */
    group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
    group.imr_interface.s_addr = inet_addr(serv_ip);
    if(setsockopt(sockfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&group,sizeof(group)) < 0)
        error("adding multicast group error");
    localLen = sizeof(localSock);

    /* get file size from server */
    recvfrom(sockfd,&f_size,sizeof(f_size),0,(struct sockaddr *)&localSock,&localLen);
    
    /* Set timeout for recvfrom */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if(setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0)
        error("setting receive timeout error");
    
    
    int time = f_size / SIZE; 
    int left = f_size - time * SIZE;
   
    int count = 0;

    /* Read from the socket. */
    if(f_size >= SIZE){
        for(int i = 0;i < time;++i){
            recvfrom(sockfd,buffer,SIZE,0,(struct sockaddr *)&localSock,&localLen);
            ++count;
        }
        if(count == time){
            recvfrom(sockfd,buffer,left,0,(struct sockaddr *)&localSock,&localLen);
            ++count;    
        }
    }
    else{
        recvfrom(sockfd,buffer,left,0,(struct sockaddr *)&localSock,&localLen);
        ++count;
    }

    /* Calculate the packet lose rate */
    loseRate = (double)(time+1 - count)/(time+1);
    printf("Expected packets : %d\n",time+1);
    printf("Receive : %d\n",count);
    printf("Packet lose rate = %f\n",loseRate);

    close(sockfd);

    return 0;
}
/* error handler */
void error(char *err){
    perror(err);
    exit(1);
}

