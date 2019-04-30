#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#define SIZE 1024 
struct sockaddr_in groupSock; 
struct in_addr localInterface;
void error(char *err);

int main(int argc,char *argv[]){
    char *fileName = argv[1]; //get file name
    char buffer[SIZE];
    size_t f_size;
    
    /* Create a datagram socket on which to send. */
    int sockfd = socket(AF_INET,SOCK_DGRAM,0); 
    if(sockfd < 0) error("error opening socket");
    memset(&groupSock,0,sizeof(groupSock));
    
    /* Initialize the group sockaddr structure with a  group address of 226.1.1.1 and port 8010. */
    groupSock.sin_family = AF_INET;
    groupSock.sin_addr.s_addr = inet_addr("226.1.1.1"); 
    groupSock.sin_port = htons(8010);
    
    /* Disable loopback */
    int loopBack = 0;
    if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_LOOP,(const char *)&loopBack,sizeof(loopBack)) < 0) 
        error("setting IP_MULTICAST_LOOP error");

    /* Set local interface for outbound multicast datagrams. */
    localInterface.s_addr = inet_addr("127.0.0.1");
    if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_IF,&localInterface,sizeof(localInterface)) < 0)
        error("setting local interface error");
    
    FILE *fp = fopen(fileName,"rb");
    
    /* get file size */
    fseek(fp,0,SEEK_END);
    f_size = ftell(fp);
    fseek(fp,0,SEEK_SET);

    int time = f_size / SIZE; 
    int left = f_size - time * SIZE;

    /*send file size to client */
    sendto(sockfd,&f_size,sizeof(f_size),0,(struct sockaddr *)&groupSock,sizeof(groupSock));
    
    /* Send file to the multicast group specified by the groupSock sockaddr structure. */
    for(int i = 0;i < time;++i){
        fread(buffer,SIZE,1,fp);
        sendto(sockfd,buffer,SIZE,0,(struct sockaddr *)&groupSock,sizeof(groupSock));
        usleep(100);
    }
    fread(buffer,left,1,fp);
    sendto(sockfd,buffer,left,0,(struct sockaddr *)&groupSock,sizeof(groupSock));


    
    close(sockfd);

    return 0;
}
/* error handler */
void error(char *err){
    perror(err);
    exit(1);
}




