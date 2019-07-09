#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdint.h>
#include <iostream>
#include <netdb.h>
#include "fecpp.h"
#define K 20
#define N 32
#define MAX_SIZE 32
struct sockaddr_in groupSock; 
struct in_addr localInterface;
int portno;
char serv_ip[MAX_SIZE];

void error(char *err);
void sendData(size_t sock, size_t count, const uint8_t buffer[], size_t size);

/* Create a datagram socket on which to send. */
int sockfd = socket(AF_INET,SOCK_DGRAM,0); 

int main(int argc,char *argv[]){
    //Get host address and port number
    strcpy(serv_ip,argv[1]);
    portno = atoi(argv[2]);
    
    if(sockfd < 0) error("error opening socket");


    char *fileName = argv[3]; 
    size_t f_size;
    
     memset(&groupSock,0,sizeof(groupSock));
    /* Initialize the group sockaddr structure with a  group address of 226.1.1.1 and port 8010. */
    groupSock.sin_family = AF_INET;
    groupSock.sin_addr.s_addr = inet_addr("226.1.1.1"); 
    groupSock.sin_port = htons(portno);
    
    /* Disable loopback */
    int loopBack = 0;
    if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_LOOP,(const char *)&loopBack,sizeof(loopBack)) < 0) 
        error("setting IP_MULTICAST_LOOP error");

    /* Set local interface for outbound multicast datagrams. */
    localInterface.s_addr = inet_addr(serv_ip);
    if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_IF,&localInterface,sizeof(localInterface)) < 0)
        error("setting local interface error");
    
    FILE *fp = fopen(fileName,"rb");
    
    /* Get file size */
    fseek(fp,0,SEEK_END);
    f_size = ftell(fp);
    fseek(fp,0,SEEK_SET);

    sendto(sockfd,&f_size,sizeof(f_size),0,(struct sockaddr *)&groupSock,sizeof(groupSock));

    uint8_t buffer[f_size];

    /* Initiate K,N */
    fecpp::fec_code fec(K,N);
    
    int shareSize = f_size / K;
    int left = f_size - shareSize * K;
     
    fread(buffer,f_size,1,fp);
    
    /* Encode data and call the callback function "sendData" N times*/
    fec.encode(buffer,shareSize * K,sendData);
    
    /* The left part inform K = 1,N = 3 */
    fecpp::fec_code fecLeft(1,3);

    /* Encode left part data */
    fecLeft.encode(buffer + shareSize * K,left,sendData);

    close(sockfd);
    return 0;
}

/* Function latter to be callback */
/* The function will be called N + 3 times to send data and its index */
void sendData(size_t index, size_t n, const uint8_t buf[], size_t length){
    sendto(sockfd,&index,sizeof(size_t),0,(struct sockaddr *)&groupSock,sizeof(groupSock));
    sendto(sockfd,buf,length,0,(struct sockaddr *)&groupSock,sizeof(groupSock));
    usleep(100);
}

/* error handler */
void error(char *err){
    perror(err);
    exit(1);
}



    
