#include<stdio.h>
#include<string.h>    
#include<stdlib.h>   
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h>    
#include<pthread.h>
#define SIZE 1024
#define MAX_SIZE 32
struct sockaddr_in serv_addr,cli_addr;
socklen_t cli_len;
int portno;
char buffer[SIZE],fileName[MAX_SIZE],serv_ip[MAX_SIZE];

/* the thread function */
void *conn_handler(void *cli_sock);

int main(int argc,char *argv[]){
    int sockfd,new_sockfd;
    
    //Get host address and port number
    strcpy(serv_ip,argv[1]);
    portno = atoi(argv[2]);

    //Get file name
    strcpy(fileName,argv[3]);

    //Create socket
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0){
        perror("error opening socket");
        exit(1);
    }
    
    /* allow to use the same port after the socket close. */
    int yes = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof(yes));

    //Prepare the sockaddr_in structure
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    //Bind
    if(bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
        perror("error on binding");
        exit(1);
    }

    //Listen
    listen(sockfd,3);
    puts("Waiting for incoming connections...");
    cli_len = sizeof(cli_addr);
    pthread_t thread_id;
    
    //Accept and incoming connection
    while((new_sockfd = accept(sockfd,(struct sockaddr *)&cli_addr,&cli_len))){
        puts("Connection accepted");
        if(pthread_create(&thread_id,NULL,conn_handler,(void *)&new_sockfd) < 0){
            perror("could not create thread");
            exit(1);
        }
        puts("Handler assigned");
    }
    
    return 0;
}

/*
 * This will handle connection for each client
 * */
void *conn_handler(void *cli_sock){
    int sockfd = *(int *)cli_sock;
    FILE *fp;
    size_t f_size;

    fp = fopen(fileName,"rb");
    /* get file size. */
    
    fseek(fp,0,SEEK_END);
    f_size = ftell(fp);
    fseek(fp,0,SEEK_SET);
    int time = f_size / SIZE; 
    int left = f_size - time * SIZE;
    
    /* send file size to client. */
    send(sockfd,&f_size,sizeof(f_size),0);
    
    /* send file to client */
    for(int i = 0;i < time;++i){
        fread(buffer,SIZE,1,fp);
        send(sockfd,buffer,SIZE,0);
    }
    fread(buffer,left,1,fp);
    send(sockfd,buffer,left,0);
    
    close(sockfd);
}
