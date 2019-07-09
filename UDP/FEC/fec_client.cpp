#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <map>
#include <iostream>
#include "fecpp.h"
#define K 20
#define N 32
#define MAX_SIZE 32
struct sockaddr_in localSock;
socklen_t localLen;
struct ip_mreq group;
struct timeval timeout;
int number = 0;
int portno;
char serv_ip[MAX_SIZE];

void outputData(size_t, size_t, const uint8_t[], size_t);
void error(char *err);

int main(int argc,char *argv[]){
    //Get host address and port number
    strcpy(serv_ip,argv[1]);
    portno = atoi(argv[2]);
    
    size_t f_size;
    std::map<size_t,const uint8_t *> shares;
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

    
    /* Initiate K,N */
    fecpp::fec_code fec(K,N);
    
    int shareSize = f_size / K;
    int left = f_size - shareSize * K;
    
    uint8_t buffer[shareSize];
    uint8_t *allData = (uint8_t *)malloc(shareSize * N);

    /* Set timeout for recvfrom */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if(setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0)
        error("setting receive timeout error");
    
    size_t index;
    int count = 0;
    
    /* Receive N shares */
    for(int i = 0;i < N;++i){
        if(recvfrom(sockfd,&index,sizeof(size_t),0,(struct sockaddr *)&localSock,&localLen) < 0){
            printf("no else packets\n");
            break;
        }
        
        if(recvfrom(sockfd,buffer,shareSize,0,(struct sockaddr *)&localSock,&localLen) < 0){
            printf("no else packets\n");
            break;
        }

        for(int j = 0;j < shareSize;++j){
            *(allData + i*shareSize + j) = buffer[j];
        }
        
        /* Put data and its index into map */
        shares.insert(std::make_pair(index,allData + i*shareSize));
        ++count;
    }
    
    /* The left part inform K = 1,N = 3 */
    fecpp::fec_code fecLeft(1,3);
    std::map<size_t,const uint8_t *> leftShares;
    
    uint8_t leftBuf[left];
    uint8_t allLeftData[left * 3];
    
    
    /* Receive left share */
    if(count == N){    
        for(int i = 0;i < 3;++i){
            recvfrom(sockfd,&index,sizeof(size_t),0,(struct sockaddr *)&localSock,&localLen);
            recvfrom(sockfd,leftBuf,left,0,(struct sockaddr *)&localSock,&localLen);
        
            for(int j = 0;j < shareSize;++j){
                *(allLeftData + i*left + j) = leftBuf[j];
            }

            /* Put data and its index into map */
            leftShares.insert(std::make_pair(index,allLeftData + i*left));
            ++count;
        }
    }
    
    /* Decode packets only if receive all packet from server */
    if(count == N + 3){
        //Decode data and call the callback function "outputData" K times
        fec.decode(shares,shareSize,outputData);
        
        //Decode left part data 
        fecLeft.decode(leftShares,left,outputData);
    }
    else{
        printf("receive %d packets,less than %d\n",count,N + 3);
        exit(1);printf("Expected packets : %d\n",time+1);
    printf("Receive : %d\n",count);
    }

    /* Calculate the packet lose rate */
    loseRate = static_cast<double>((K+1)-number)/(K+1);
    printf("Expected packets : %d\n",N+3);
    printf("Receive : %d\n",count);
    printf("Decode to %d packets\n",number);
    printf("Packet lose rate = %f\n",loseRate);

    close(sockfd);
    return 0;
}

/* Function latter to be callback */
/* The function will be called K + 1 times */
void outputData(size_t index, size_t k, const uint8_t buf[], size_t length){
    ++number;
}

/* error handler */
void error(char *err){
    perror(err);
    exit(1);
}
