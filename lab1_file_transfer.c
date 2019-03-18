#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define MAX_SIZE 64
#define EACH_TIME 1
#define PIECE_NUM 20
#define EACH_PERCENT 5


void error(const char *msg);
void tcp();
void tcp_server(int sock);
void tcp_client(int sock);
void udp();
void udp_server(int sock);
void udp_client(int sock);

int sockfd,portno;
char protocol[MAX_SIZE],send_or_recv[MAX_SIZE],serv_ip[MAX_SIZE],f_name[MAX_SIZE];
char buffer[EACH_TIME];
FILE *fp;
size_t f_size;
time_t timep;
struct sockaddr_in serv_addr;
struct tm *p;



int main(int argc,char *argv[])
{
    strcpy(protocol,argv[1]);
    strcpy(send_or_recv,argv[2]);
    strcpy(serv_ip,argv[3]);
    portno = atoi(argv[4]);
    if(!strcmp(send_or_recv,"send"))
        strcpy(f_name,argv[5]);
    
    if(!strcmp(send_or_recv,"send") && argc != 6)
        fprintf(stderr,"Please enter correct number of arguments\n");
    if(!strcmp(send_or_recv,"recv") && argc != 5)
        fprintf(stderr,"Please enter correct number of arguments\n");
    if(!strcmp(protocol,"tcp"))
        tcp();
    else if(!strcmp(protocol,"udp"))
        udp();
    else{
        fprintf(stderr,"Please enter protocol in the second argument\n");
        exit(1);
    }

    return 0;
}
void tcp()
{  
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
        error("error opening socket");
    
    if(!strcmp(send_or_recv,"send")) 
        tcp_server(sockfd);
    
    else if(!strcmp(send_or_recv,"recv"))
        tcp_client(sockfd);

    close(sockfd);
    return;
}
void tcp_server(int sock)
{   
    int newsock;
    int count,piece,how_many_piece,percent;
    socklen_t clilen;
    struct sockaddr_in cli_addr;

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if(bind(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("error on binding");
    listen(sock,5);

    clilen = sizeof(cli_addr);
    if((newsock = accept(sock,(struct sockaddr*) &cli_addr,&clilen)) < 0)
        error("error on accept");

    fp = fopen(f_name,"rb");
    fseek(fp,0,SEEK_END);
    f_size = ftell(fp);   //get file size
    piece = f_size / PIECE_NUM; 
    fseek(fp,0,SEEK_SET);

    
    if(send(newsock,&f_size,sizeof(f_size),0) < 0)
        error("error on sending filesize");

    if(send(newsock,&f_name,sizeof(f_name),0) < 0)
        error("error on sending filename");

    count = 0;
    how_many_piece = 0;
    percent = 0;
    while(!feof(fp)){
        count++;
        fread(buffer,EACH_TIME,EACH_TIME,fp);
        send(newsock,buffer,EACH_TIME,0);
        memset(buffer,0,sizeof(buffer));
        if(count == piece * (how_many_piece + 1)){   
            time(&timep);  
            p = localtime(&timep);
            percent += EACH_PERCENT;
        
            printf("%d%s  ",percent,"%");
            printf("%d/%d/%d  ",(1900+p->tm_year),(1+p->tm_mon),(p->tm_mday));
            printf("%d:%d:%d\n",(p->tm_hour),(p->tm_min),(p->tm_sec));
            
            how_many_piece++;
        }
    }
    close(newsock);
    return;
}
void tcp_client(int sock)
{   
    int count,piece,how_many_piece,percent;
    char output[MAX_SIZE] ="output_";
    
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    serv_addr.sin_port = htons(portno);
    if(connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("error on connecting");
    
    recv(sock,&f_size,sizeof(f_size),0);
    piece = f_size / PIECE_NUM;
    
    recv(sock,&f_name,sizeof(f_name),0);
    strcat(output,f_name);
    
    fp = fopen(output,"wb");

    count = 0;
    how_many_piece = 0;
    percent = 0; 
    while(recv(sock,buffer,EACH_TIME,0) != 0){
        count++;
        fwrite(buffer,EACH_TIME,EACH_TIME,fp);
        memset(buffer,0,sizeof(buffer));
        if(count == piece * (how_many_piece + 1)){
            time(&timep);  
            p = localtime(&timep);
            percent += EACH_PERCENT;

            printf("%d%s  ",percent,"%");
            printf("%d/%d/%d  ",(1900+p->tm_year),(1+p->tm_mon),(p->tm_mday));
            printf("%d:%d:%d\n",(p->tm_hour),(p->tm_min),(p->tm_sec));
            how_many_piece++;
        }

    }
    return;
}
void udp()
{
    if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
        error("error opening socket");
    if(!strcmp(send_or_recv,"send"))
        udp_server(sockfd);
    else if(!strcmp(send_or_recv,"recv"))
        udp_client(sockfd);

    close(sockfd);
    return;
}
void udp_server(int sock)
{
    int get_request,cli_finish;
    int count,piece,how_many_piece,percent;
    socklen_t serv_len;
    
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    serv_len = sizeof(serv_addr);
    if(bind(sock,(struct sockaddr *) &serv_addr,serv_len) < 0)
       error("error on binding");
    
    fp = fopen(f_name,"rb");
    fseek(fp,0,SEEK_END);
    f_size = ftell(fp);   //get file size
    piece = f_size / PIECE_NUM; 
    fseek(fp,0,SEEK_SET);
    
    if(sendto(sock,&f_size,sizeof(f_size),0,(struct sockaddr *)&serv_addr,serv_len) < 0)
        error("error on sending filesize");

    count = 0;
    how_many_piece = 0;
    percent = 0;
    while(!feof(fp)){
        count++;
        fread(buffer,EACH_TIME,EACH_TIME,fp);
        if(sendto(sock,buffer,EACH_TIME,0,(struct sockaddr *)&serv_addr,serv_len) < 0)
            error("error on sending file");
        memset(buffer,0,sizeof(buffer));
        
        if(count == piece * (how_many_piece + 1)){
            percent += EACH_PERCENT;
            time(&timep);  
            p = localtime(&timep);
            printf("%d%s  ",percent,"%");
            printf("%d/%d/%d  ",(1900+p->tm_year),(1+p->tm_mon),(p->tm_mday));
            printf("%d:%d:%d\n",(p->tm_hour),(p->tm_min),(p->tm_sec));
            how_many_piece++;
        }
    }
    while(1){
        if(recvfrom(sock,&cli_finish,sizeof(cli_finish),0,(struct sockaddr *)&serv_addr,&serv_len) > 0)
            return;
    }
}
void udp_client(int sock)
{
    int request,finish;
    int count,piece,how_many_piece,percent;
    char output[MAX_SIZE] = "output_";
    socklen_t serv_len;
    struct sockaddr_in my_addr;

    strcat(output,f_name);
    
    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(0);
   
    if(bind(sock,(struct sockaddr *) &my_addr,sizeof(my_addr)) < 0)
        error("error on binding");
    
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    serv_addr.sin_port = htons(portno);
    
    while(1){
        if(recvfrom(sock,&f_size,sizeof(f_size),0,(struct sockaddr *) &serv_addr,&serv_len) > 0)
            break;
    }
    
    piece = f_size / PIECE_NUM;
    FILE *fp = fopen(output,"wb");
    
    count = 0;
    how_many_piece = 0;
    percent = 0;
    while(recvfrom(sock,buffer,EACH_TIME,0,(struct sockaddr *) &serv_addr,&serv_len) != 0){
        count++;
        fwrite(buffer,EACH_TIME,EACH_TIME,fp);
        memset(buffer,0,sizeof(buffer));
        if(count == piece * (how_many_piece + 1)){
            percent += EACH_PERCENT;
            time(&timep);  
            p = localtime(&timep);
            
            printf("%d%s  ",percent,"%");
            printf("%d/%d/%d  ",(1900+p->tm_year),(1+p->tm_mon),(p->tm_mday));
            printf("%d:%d:%d\n",(p->tm_hour),(p->tm_min),(p->tm_sec));
            how_many_piece++;
        }
    }
    while(1){
        if(sendto(sock,&finish,sizeof(finish),0,(struct sockaddr *) &serv_addr,serv_len) > 0) 
            return;
        
    }
}
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

