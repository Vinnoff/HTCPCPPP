#include<stdio.h>
#include<sys/types.h>//socket
#include<sys/socket.h>//socket
#include<string.h>//memset
#include<stdlib.h>//sizeof
#include<netinet/in.h>//INADDR_ANY
#include <arpa/inet.h>

#define PORT 8000
#define SERVER_IP "127.0.0.1"
#define MAXSZ 1000

struct Header {
    char key[MAXSZ];
    char value[MAXSZ];
};

struct Request{
    char type[MAXSZ];
    struct Header headers[10];
    int headerSize;
};

struct Request add_header(struct Request request, struct Header header) {
    request.headers[request.headerSize] = header;
    request.headerSize ++;
    return request;
}

int main(int argc, char **argv)
{
    int sockfd;//to create socket
    
    struct sockaddr_in serverAddress;//client will connect on this
    
    int n;
    char* requestString = malloc(100);
    char msg2[MAXSZ];
    
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    
    memset(&serverAddress,0,sizeof(serverAddress));
    serverAddress.sin_addr.s_addr=inet_addr(SERVER_IP);
    serverAddress.sin_family=AF_INET;
    serverAddress.sin_port=htons(PORT);
    
    if(connect(sockfd,(struct sockaddr *)&serverAddress,sizeof(serverAddress)) == -1){
        printf("Error while connect\n",msg2);
        return 0;
    }
    
    while(1){
        strcpy(requestString, argv[1]);
        for (int i = 2; i< argc;i++) {
            strcat(requestString, i%2==0 ? "=" : "=");
            strcat(requestString, argv[i]);
        }
        
        send(sockfd, requestString, MAXSZ, 0);
        n=recv(sockfd, msg2, MAXSZ, 0);
        
        printf("Server:\n%s\n",msg2);
        break;
    }
    return 0;
}
