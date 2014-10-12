#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char **argv){
    char domain[512];
    //Getting the input from user
    /***************************************************************************/
    printf("Please insert the mail server domain :\n");
    printf("smtp://");
    scanf("%s",domain);
    /***************************************************************************/
    
    //Create socket
    /***************************************************************************/
    int sockfd4, sockfd6, sockfd;
    if((sockfd4 = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Error creating IPv4 socket\n");
        return 1;
    }
    if((sockfd6 = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Error creating IPv6 socket\n");
        return 1;
    }
    /***************************************************************************/
    
    
    //Connecting to the sever
    /***************************************************************************/
    struct addrinfo *servaddr, *iter, hint;
    struct sockaddr_in *h1;
    struct sockaddr_in6 *h2;
    struct sockaddr *h;
    
    int status;
    
    hint.ai_family = AF_UNSPEC;     //Can accept either IPv4 or IPv6
    hint.ai_socktype = SOCK_STREAM;  //Using TCP 
    
    //Resolve the domain
    if((status = getaddrinfo(domain, "smtp", &hint, &servaddr)) != 0){
        //Print the error
        fprintf(stderr,"getaddrinfo error : %s\n", gai_strerror(status));
        return 1;
    }
    
    //Trying to connect to the server
    int tries = 0;
    char *address;
    //Loop through all the result
    for(iter = servaddr; iter != NULL; iter->ai_next){
        ++tries;
        h = iter->ai_addr;
        //Check the IP version
        if(iter->ai_family == AF_INET){
            sockfd = sockfd4;
            h1 = (struct sockaddr_in *)h;
            inet_ntop(AF_INET, &(h1)->sin_addr, address, sizeof(h1->sin_addr));            
        }
        else if(iter->ai_family == AF_INET6){
            sockfd = sockfd6;
            inet_ntop(AF_INET6, &(h2)->sin6_addr, address, sizeof(h2->sin6_addr));
        }
        
        fprintf(stdout, "Connecting to %s\n", address);
        
        //Connect to the server
        if(connect(sockfd, h, sizeof(*h)) == 0){
            fprintf(stdout, "Successfully connected to the server after %d try(es)\n", tries);
            break;
        }
    }
    //Deallocate the unused memory
    freeaddrinfo(servaddr);
    /***************************************************************************/
    
    //Interacting with the server
    /***************************************************************************/
    //Send and receive buffer
    char recvBuffer[1024], sendBuffer[4096];
    memset(sendBuffer, 0, sizeof(sendBuffer));
    memset(recvBuffer, 0, sizeof(recvBuffer));
    //Message size
    int msgSize;
    
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer));
    fprintf(stdout, "%s", recvBuffer);
    
    //Send our hello to the server
    sprintf(sendBuffer, "ehlo %s\n", domain);
    write(sockfd, sendBuffer, strlen(sendBuffer));
    
    //Print the response
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer)); 
    fprintf(stdout, "%s", recvBuffer);

    memset(sendBuffer, 0, sizeof(sendBuffer));
    memset(recvBuffer, 0, sizeof(recvBuffer));
    //Exit from the server
    sprintf(sendBuffer, "quit\n");
    write(sockfd, sendBuffer, strlen(sendBuffer));
    
     //Print the response
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer));
    fprintf(stdout, "%s", recvBuffer);
    
    /***************************************************************************/
    
    
    close(sockfd4);
    close(sockfd6);
    return 0;
}