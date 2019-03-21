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
#define PIECE_NUM 20

void error(const char *msg);
void tcp();
void tcp_server(int sock);
void tcp_client(int sock);
void udp();
void udp_server(int sock);
void udp_client(int sock);


int sockfd,portno;
char protocol[MAX_SIZE],send_or_recv[MAX_SIZE],serv_ip[MAX_SIZE],f_name[MAX_SIZE];

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
    int piece,count,left;
    FILE *fp;
    size_t f_size;
    time_t timep;
    socklen_t clilen;
    struct sockaddr_in serv_addr,cli_addr;
    struct tm *p;

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
    
    char buffer[piece];

    count = 0;
    left = f_size - 20 * piece;
    if(f_size < PIECE_NUM){
        fread(buffer,f_size,1,fp);
        send(newsock,buffer,f_size,0);
    }
    else{
        while(1){
            if(++count == 21)
                break;
            fread(buffer,piece,1,fp);
            if(send(newsock,buffer,piece,0) < 0)
                error("error on sending file");
            memset(buffer,0,sizeof(buffer));
                
            time(&timep);  
            p = localtime(&timep);
            printf("%d%s  ",count * 5,"%");
            printf("%d/%d/%d  ",(1900+p->tm_year),(1+p->tm_mon),(p->tm_mday));
            printf("%d:%d:%d\n",(p->tm_hour),(p->tm_min),(p->tm_sec));
        }
        fread(buffer,left,1,fp);
        send(newsock,buffer,left,0);
    }    
    close(newsock);
    printf("done!\n");

    return;
}
void tcp_client(int sock)
{   
    int piece,count,left;
    FILE *fp;
    size_t f_size;
    struct sockaddr_in serv_addr;

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    serv_addr.sin_port = htons(portno);
    if(connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("error on connecting");
    if(recv(sock,&f_size,sizeof(f_size),0) < 0)
        error("error on receving file size");
    if(recv(sock,&f_name,sizeof(f_name),0) < 0)
        error("error on receving file name");
    
    char output[MAX_SIZE] = "output_";
    strcat(output,f_name);
    fp = fopen(output,"wb");
    piece = f_size / PIECE_NUM;
    left = f_size - 20 * piece;
    char buffer[piece];
    count = 0;
    if(f_size < PIECE_NUM){
        recv(sock,buffer,f_size,0);
        fwrite(buffer,f_size,1,fp);
    }
    else{
        while(1){
            if(++count == 21)
                break;
            recv(sock,buffer,piece,0);
            fwrite(buffer,piece,1,fp);
            memset(buffer,0,sizeof(buffer));
        }
        recv(sock,buffer,left,0);
        fwrite(buffer,left,1,fp);
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
    int piece,count,left;
    FILE *fp;
    size_t f_size;
    time_t timep;
    socklen_t cli_len;
    struct sockaddr_in serv_addr,cli_addr;
    struct tm *p;

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if(bind(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("error on binding");
    fp = fopen(f_name,"rb");
    fseek(fp,0,SEEK_END);
    f_size = ftell(fp);   //get file size
    piece = f_size / PIECE_NUM; 
    fseek(fp,0,SEEK_SET);
    
    cli_len = sizeof(cli_addr);
    
    while(1){
        if(recvfrom(sock,&get_request,sizeof(get_request),0,(struct sockaddr *)&cli_addr,&cli_len) > 0)
            break;
    }
    
    if(sendto(sock,&f_name,sizeof(f_name),0,(struct sockaddr *)&cli_addr,cli_len) < 0)
        error("error on sending file name");
    if(sendto(sock,&f_size,sizeof(f_size),0,(struct sockaddr *)&cli_addr,cli_len) < 0)
        error("error on sending file size");
    
    char buffer[piece];

    count = 0;
    left = f_size - 20* piece;
    if(f_size < PIECE_NUM){
        fread(buffer,f_size,1,fp);
        sendto(sock,buffer,f_size,0,(struct sockaddr *)&cli_addr,cli_len);
    }
    else{
        while(1){
            if(++count == 21)
                break;
            fread(buffer,piece,1,fp);
            if(sendto(sock,buffer,piece,0,(struct sockaddr *)&cli_addr,cli_len) < 0)
                error("error on sending file");
            memset(buffer,0,sizeof(buffer));
                
            time(&timep);  
            p = localtime(&timep);
            printf("%d%s  ",count * 5,"%");
            printf("%d/%d/%d  ",(1900+p->tm_year),(1+p->tm_mon),(p->tm_mday));
            printf("%d:%d:%d\n",(p->tm_hour),(p->tm_min),(p->tm_sec));
            usleep(100);
    
        }
        usleep(100);
        fread(buffer,left,1,fp);
        sendto(sock,buffer,left,0,(struct sockaddr *)&cli_addr,cli_len);
    }   
    recvfrom(sock,&cli_finish,sizeof(cli_finish),0,(struct sockaddr *)&cli_addr,&cli_len);

    printf("done!\n");
    
    return;
}
void udp_client(int sock)
{
    int request,finish;
    int piece,count,left;
    FILE *fp;
    size_t f_size;
    socklen_t serv_len;
    struct sockaddr_in serv_addr,my_addr;

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

    serv_len = sizeof(serv_addr);
    if(sendto(sock,&request,sizeof(request),0,(struct sockaddr *) &serv_addr,serv_len) < 0)
        error("error on sending request");
    
    if(recvfrom(sock,&f_name,sizeof(f_name),0,(struct sockaddr *) &serv_addr,&serv_len) < 0)
        error("error on receiving file name");
    if(recvfrom(sock,&f_size,sizeof(f_size),0,(struct sockaddr *) &serv_addr,&serv_len) < 0)
        error("error on receiving file size");
    
    char output[MAX_SIZE] = "output_"; 
    strcat(output,f_name);
    fp = fopen(output,"wb");
    piece = f_size / PIECE_NUM;
    left = f_size - 20 * piece;
    char buffer[piece];
    count = 0;
    if(f_size < PIECE_NUM){
        recvfrom(sock,buffer,f_size,0,(struct sockaddr *) &serv_addr,&serv_len);
        fwrite(buffer,f_size,1,fp);
    }
    else{
        while(1){
            if(++count == 21)
                break;
            recvfrom(sock,buffer,piece,0,(struct sockaddr *) &serv_addr,&serv_len);
            fwrite(buffer,piece,1,fp);
            memset(buffer,0,sizeof(buffer));
        }
        recvfrom(sock,buffer,left,0,(struct sockaddr *) &serv_addr,&serv_len);
        fwrite(buffer,left,1,fp);
    }
    if(sendto(sock,&finish,sizeof(finish),0,(struct sockaddr *) &serv_addr,serv_len) < 0)
        error("error on sending finish message");
        
    return;
}
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

