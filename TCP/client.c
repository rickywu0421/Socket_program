#include<stdio.h>
#include<string.h>    
#include<stdlib.h>   
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h>    
#define SIZE 1024
#define MAX_SIZE 32
struct sockaddr_in serv_addr;
size_t f_size;
int portno;
char buffer[SIZE],serv_ip[MAX_SIZE];


int main(int argc,char *argv[]){
    //Get host address and port number
    strcpy(serv_ip,argv[1]);
    portno = atoi(argv[2]);
    
    //Create socket
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    
    //Prepare the sockaddr_in structure
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    serv_addr.sin_port = htons(portno);

    //Connect to server
    if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
        perror("error connecting server");
        exit(1);
    }

    //Receive file size from server
    recv(sockfd,&f_size,sizeof(f_size),0);
    int time = f_size / SIZE; 
    int left = f_size - time * SIZE;

    //Open file io
    FILE *fp;
    fp = fopen("output","wb");

    //Receive file from serversize_t f_size;
    for(int i = 0;i < time;++i){
        recv(sockfd,buffer,SIZE,0);
        fwrite(buffer,SIZE,1,fp);
    }
    recv(sockfd,buffer,left,0);
    fwrite(buffer,left,1,fp);
    
    close(sockfd);
    return 0;
}
