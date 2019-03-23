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

#define MAX_SIZE 64 //some array size
#define PIECE_NUM 20 //divide the file to be sent into 20 part

void error(const char *msg);
void tcp();
void tcp_server(int sock);
void tcp_client(int sock);
void udp();
void udp_server(int sock);
void udp_client(int sock);

//initialize
int sockfd,portno;
char protocol[MAX_SIZE],send_or_recv[MAX_SIZE],serv_ip[MAX_SIZE],f_name[MAX_SIZE];

int main(int argc,char *argv[])
{   
    //get arguments from shell
    strcpy(protocol,argv[1]); //get protocol:tcp or udp
    strcpy(send_or_recv,argv[2]); //get send or recv,determine whether be a server or client
    strcpy(serv_ip,argv[3]); //get server ip
    portno = atoi(argv[4]); //get port number
    if(!strcmp(send_or_recv,"send")) 
        strcpy(f_name,argv[5]); //get file name
    
    if(!strcmp(send_or_recv,"send") && argc != 6)
        fprintf(stderr,"Please enter correct number of arguments\n");
    if(!strcmp(send_or_recv,"recv") && argc != 5)
        fprintf(stderr,"Please enter correct number of arguments\n");
    if(!strcmp(protocol,"tcp")) //jump to tcp()
        tcp();
    else if(!strcmp(protocol,"udp")) //jump to udp()
        udp();
    else{
        fprintf(stderr,"Please enter protocol in the second argument\n");
        exit(1);
    }
    return 0;
}
void tcp()
{  
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) //open socket
        error("error opening socket");
    
    if(!strcmp(send_or_recv,"send")) //jump to tcp_server()
        tcp_server(sockfd);
    
    else if(!strcmp(send_or_recv,"recv")) //jump to tcp_client()
        tcp_client(sockfd);

    close(sockfd); //close socket
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
    //set server info
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if(bind(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) //bind socket to port
        error("error on binding");
    listen(sock,5); //listen

    clilen = sizeof(cli_addr);
    if((newsock = accept(sock,(struct sockaddr*) &cli_addr,&clilen)) < 0) //accept 
        error("error on accept");

    fp = fopen(f_name,"rb"); //open file io
    //get file size
    fseek(fp,0,SEEK_END); 
    f_size = ftell(fp);   
    fseek(fp,0,SEEK_SET);
    
    piece = f_size / PIECE_NUM; //divide file size into 20 parts,each part weigh piece(about)
    
    if(send(newsock,&f_size,sizeof(f_size),0) < 0) //send file size 
        error("error on sending filesize");

    if(send(newsock,&f_name,sizeof(f_name),0) < 0) //send file name
        error("error on sending filename");
    
    char buffer[piece]; //buffer to store 

    count = 0; //counter
    left = f_size - 20 * piece; //the part out of 20 pieces
    if(f_size < PIECE_NUM){ //if file size < 20,directly send left to client(piece = 0)
        fread(buffer,f_size,1,fp);
        send(newsock,buffer,f_size,0);
    }
    else{
        while(1){ //send 20 piece to client
            if(++count == 21) //break the loop in 21th time
                break;
            fread(buffer,piece,1,fp); //read piece using file io
            if(send(newsock,buffer,piece,0) < 0) //send piece to client
                error("error on sending file");
            memset(buffer,0,sizeof(buffer)); //clear buffer
                
            time(&timep); //set time stamp
            p = localtime(&timep); //get current time
            printf("%d%s  ",count * 5,"%"); //print log each 5%
            printf("%d/%d/%d  ",(1900+p->tm_year),(1+p->tm_mon),(p->tm_mday)); //print current date
            printf("%d:%d:%d\n",(p->tm_hour),(p->tm_min),(p->tm_sec)); //print current time
        }
        fread(buffer,left,1,fp); //read left using file io
        send(newsock,buffer,left,0); //send left to client
    }    
    close(newsock); //close new socket
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
    //set server info
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    serv_addr.sin_port = htons(portno);
    if(connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) //connect to server
        error("error on connecting");
    if(recv(sock,&f_size,sizeof(f_size),0) < 0) //receive file size
        error("error on receving file size");
    if(recv(sock,&f_name,sizeof(f_name),0) < 0) //receive file name
        error("error on receving file name");
    
    char output[MAX_SIZE] = "output_"; //add ouput_ in front of file name
    strcat(output,f_name);
    
    fp = fopen(output,"wb"); //open file io
    piece = f_size / PIECE_NUM; //divide file size into 20 parts,each part weigh piece(about)
    left = f_size - 20 * piece; //the part out of 20 pieces
    char buffer[piece]; //buffer to store
    count = 0; //counter
    if(f_size < PIECE_NUM){ //if file size < 20,directly recv left from server(piece = 0) 
        recv(sock,buffer,f_size,0);
        fwrite(buffer,f_size,1,fp);
    }
    else{
        while(1){ //recv 20 piece from server
            if(++count == 21) //break the loop in 21th time
                break;
            recv(sock,buffer,piece,0);
            fwrite(buffer,piece,1,fp);
            memset(buffer,0,sizeof(buffer));
        }
        recv(sock,buffer,left,0); //receive left part from server
        fwrite(buffer,left,1,fp); //write left part into output file
    }
    return;
}
void udp()
{
    if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0) //open socket
        error("error opening socket");
    if(!strcmp(send_or_recv,"send")) //jump to udp_server()
        udp_server(sockfd);
    else if(!strcmp(send_or_recv,"recv")) //jump to udp_client()
        udp_client(sockfd);

    close(sockfd); //close socket
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
    //set server info
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if(bind(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) //bind socket to port
        error("error on binding");
    
    fp = fopen(f_name,"rb"); //open file io
    //get file size
    fseek(fp,0,SEEK_END);
    f_size = ftell(fp);   
    piece = f_size / PIECE_NUM; //divide file size into 20 parts,each part weigh piece(about)
    fseek(fp,0,SEEK_SET);
    
    cli_len = sizeof(cli_addr);
    
    while(1){ //wait for client(block)
        if(recvfrom(sock,&get_request,sizeof(get_request),0,(struct sockaddr *)&cli_addr,&cli_len) > 0)
            break;
    }
    
    if(sendto(sock,&f_name,sizeof(f_name),0,(struct sockaddr *)&cli_addr,cli_len) < 0) //send file name
        error("error on sending file name");
    if(sendto(sock,&f_size,sizeof(f_size),0,(struct sockaddr *)&cli_addr,cli_len) < 0) //send file size
        error("error on sending file size");
    
    char buffer[piece]; //buffer to store

    count = 0; //counter
    left = f_size - 20* piece; //the part out of 20 pieces
    if(f_size < PIECE_NUM){ //if file size < 20,directly send left to client(piece = 0)
        fread(buffer,f_size,1,fp);
        sendto(sock,buffer,f_size,0,(struct sockaddr *)&cli_addr,cli_len);
    }
    else{
        while(1){ //send 20 piece to client
            if(++count == 21) //break the loop in 21th time
                break;
            fread(buffer,piece,1,fp); //read piece using file io
            if(sendto(sock,buffer,piece,0,(struct sockaddr *)&cli_addr,cli_len) < 0) //send piece to client
                error("error on sending file");
            memset(buffer,0,sizeof(buffer)); //clear buffer
                
            time(&timep); //set time stamp
            p = localtime(&timep); //get current time
            printf("%d%s  ",count * 5,"%"); //print log each 5%
            printf("%d/%d/%d  ",(1900+p->tm_year),(1+p->tm_mon),(p->tm_mday)); //print current date
            printf("%d:%d:%d\n",(p->tm_hour),(p->tm_min),(p->tm_sec)); //print current time
            usleep(100); //sleep 0.1 ms
        }
        usleep(100); //sleep 0.1 ms
        fread(buffer,left,1,fp); //read left using file io
        sendto(sock,buffer,left,0,(struct sockaddr *)&cli_addr,cli_len); //send left to client
    }
    //wait for finish message from client
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
    //set self info
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(0);

    if(bind(sock,(struct sockaddr *) &my_addr,sizeof(my_addr)) < 0) //bind socket to self port
        error("error on binding");

    memset(&serv_addr,0,sizeof(serv_addr));
    //set server info
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    serv_addr.sin_port = htons(portno);

    serv_len = sizeof(serv_addr);
    if(sendto(sock,&request,sizeof(request),0,(struct sockaddr *) &serv_addr,serv_len) < 0) //tell server to send file
        error("error on sending request");
    
    if(recvfrom(sock,&f_name,sizeof(f_name),0,(struct sockaddr *) &serv_addr,&serv_len) < 0) //receive file name
        error("error on receiving file name");
    if(recvfrom(sock,&f_size,sizeof(f_size),0,(struct sockaddr *) &serv_addr,&serv_len) < 0) //receive file size
        error("error on receiving file size");
    
    char output[MAX_SIZE] = "output_"; //add ouput_ in front of file name
    strcat(output,f_name);
    fp = fopen(output,"wb"); //open file io
    piece = f_size / PIECE_NUM; //divide file size into 20 part,each part weigh piece(about)
    left = f_size - 20 * piece; //the part out of 20 pieces
    char buffer[piece]; //buffer to store
    count = 0; //counter
    if(f_size < PIECE_NUM){ //if file size < 20,directly recv left part from server(piece = 0)
        recvfrom(sock,buffer,f_size,0,(struct sockaddr *) &serv_addr,&serv_len);
        fwrite(buffer,f_size,1,fp);
    }
    else{
        while(1){ //recv 20 piece from server
            if(++count == 21) //break the loop in 21th time
                break;
            recvfrom(sock,buffer,piece,0,(struct sockaddr *) &serv_addr,&serv_len);
            fwrite(buffer,piece,1,fp);
            memset(buffer,0,sizeof(buffer));
        }
        recvfrom(sock,buffer,left,0,(struct sockaddr *) &serv_addr,&serv_len); //receive left part from server
        fwrite(buffer,left,1,fp); //write left part into output file
    }
    if(sendto(sock,&finish,sizeof(finish),0,(struct sockaddr *) &serv_addr,serv_len) < 0) //tell server finished
        error("error on sending finish message");
        
    return;
}
void error(const char *msg) //error handling
{
    perror(msg);
    exit(1);
}

