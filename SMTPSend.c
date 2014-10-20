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

#include <time.h>
#include <fcntl.h>

int serv_connect(const char* server, const char* protocol);
int handshake(int sockfd);
int setAttachment(int sockfd, const char* path);
void writeMetadata(int sockfd, const char* sender, const char* receiver);

int main(int argc, char **argv){
    char *domain = (char*)malloc(sizeof(char) * 512);
    //Getting the input from user
    /***************************************************************************/
    printf("Please insert the mail server domain :\n");
    printf("smtp://");
    scanf("%s", domain);
    /***************************************************************************/
    
    //Connecting to the specified server
    /***************************************************************************/
    int sockfd;
    if((sockfd = serv_connect(domain, "smtp")) == -1) return 1;
    /***************************************************************************/
    
    //Handshaking the server
    /***************************************************************************/
    int retval;
    if((retval = handshake(sockfd)) != 0) return 1;
    /***************************************************************************/
    
    //Specify the Sender and Receiver
    /***************************************************************************/
    int result;
    int msgSize;
    char recvBuffer[1024], sendBuffer[2048];
    memset(sendBuffer, '\0', sizeof(sendBuffer));
    memset(recvBuffer, '\0', sizeof(recvBuffer));
    
    char *mail = (char*)malloc(sizeof(char) * 50);
    //Specify the sender
    do{
        //Get the sender
        fprintf(stdout, "Specify the sender : ");
        fscanf(stdin, "%s", mail);
        sprintf(sendBuffer,"MAIL FROM: %s\n",mail);
        
        write(sockfd, sendBuffer, strlen(sendBuffer));
        
        //Print the response
        msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
        recvBuffer[msgSize] = '\0';
//        fprintf(stdout, "%s", recvBuffer);
        sscanf(recvBuffer, "%d", &result);
    }while(result != 250);
    
    char *sender = (char*)malloc(sizeof(char) * 50);
    strcpy(sender, mail);
    
    //Specify the receiver
    do{
        //Get the receiver
        fprintf(stdout, "Specify the recipient : ");
        fscanf(stdin, "%s", mail);
        sprintf(sendBuffer,"RCPT TO: %s\n",mail);
        
        write(sockfd, sendBuffer, strlen(sendBuffer));
        
        //Print the response
        msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
        recvBuffer[msgSize] = '\0';
//        fprintf(stdout, "%s", recvBuffer);
        sscanf(recvBuffer, "%d", &result);
        if(result != 250) fprintf(stderr, "There is no such address in this server\n");
    }while(result != 250);
        
    char *receiver = (char*)malloc(sizeof(char) * 50);
    strcpy(receiver, mail);
    
    free(mail);
    /****************************************************************************/
    
    //Write the metadata    
    writeMetadata(sockfd, sender, receiver);     
    
    //Write the subject
    getc(stdin);
    fprintf(stdout, "Subject : ");
    fgets(sendBuffer, sizeof(sendBuffer), stdin);
    
    write(sockfd, "Subject: ", strlen("Subject: "));
    write(sockfd, sendBuffer, strlen(sendBuffer));
    
    free(sender);
    free(receiver);
    
    //Put an attachment
    
    
    //Write the message
    /***************************************************************************/
    fprintf(stdout, "Write your message below\n");
    do{
        fgets(sendBuffer, sizeof(sendBuffer), stdin);
        write(sockfd, sendBuffer, strlen(sendBuffer));
        result = strcmp(sendBuffer, ".\n");
    } while (result != 0);

    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
    recvBuffer[msgSize] = '\0';
    fprintf(stdout, "%s", recvBuffer);
    
    /***************************************************************************/    
    
    //Exit from the server
    write(sockfd, "QUIT\n", strlen("QUIT\n"));
    
     //Print the response
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
    recvBuffer[msgSize] = '\0';
    fprintf(stdout, "%s", recvBuffer);
    
    /***************************************************************************/
    return 0;
}

int serv_connect(const char* server, const char* protocol){
    //Create socket
    /***************************************************************************/
    int sockfd4, sockfd6, sockfd;
    
    if((sockfd4 = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Error creating IPv4 socket\n");
        return -1;
    }
    if((sockfd6 = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Error creating IPv6 socket\n");
        return -1;
    }
    /***************************************************************************/
    
    
    //Connecting to the sever
    /***************************************************************************/
    struct addrinfo *servaddr, *iter, hint;
    struct sockaddr_in *h1;
    struct sockaddr_in6 *h2;
    struct sockaddr *h;
    
    int status;
    memset(&hint, '\0', sizeof(struct addrinfo));   //Clean the struct from its remnant
    hint.ai_family = AF_UNSPEC;                     //Can accept either IPv4 or IPv6
    hint.ai_socktype = SOCK_STREAM;                 //Using TCP 
    
    //Resolve the domain
    if((status = getaddrinfo(server, protocol, &hint, &servaddr)) != 0){
        //Print the error
        fprintf(stderr,"getaddrinfo error : %s\n", gai_strerror(status));
        return 1;
    }
    
    //Trying to connect to the server
    int tries = 0;
    char *address = (char*)malloc(sizeof(char) * 30);
    //Loop through all the result
    for(iter = servaddr; iter != NULL; iter = iter->ai_next){
        ++tries;
        h = iter->ai_addr;
        //Check the IP version
        if(iter->ai_family == AF_INET){
            sockfd = sockfd4;
            h1 = (struct sockaddr_in*)h;
            inet_ntop(AF_INET, &(h1->sin_addr), address, INET_ADDRSTRLEN);
        }
        else if(iter->ai_family == AF_INET6){
            sockfd = sockfd6;
            h2 = (struct sockaddr_in6*)h;
            inet_ntop(AF_INET6, &(h2->sin6_addr), address, INET6_ADDRSTRLEN);
        }
        
        fprintf(stdout, "Connecting to %s\n", address);
        
        //Connect to the server
        if(connect(sockfd, h, sizeof(*h)) == 0){
            fprintf(stdout, "Successfully connected to the server after %d try(es)\n", tries);
            break;
        }
    }
    //Deallocate the unused memory
    if(h->sa_family == AF_INET){
        close(sockfd6);
    }
    else if(h->sa_family == AF_INET6){
        close(sockfd4);
    }
    freeaddrinfo(servaddr);
    free(address);
    /***************************************************************************/
    return sockfd;
}
int handshake(int sockfd){
    //Send and receive buffer
    char recvBuffer[1024], sendBuffer[256], domain[50];
    memset(sendBuffer, '\0', sizeof(sendBuffer));
    memset(recvBuffer, '\0', sizeof(recvBuffer));
    //Message size
    int msgSize;
    
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
    recvBuffer[msgSize] = '\0';
//    fprintf(stdout, "%s", recvBuffer);
    
    //Get the domain name
    /*****************************************************/
    int result;
    char *temp, *token;
    do{
        token = strtok_r(recvBuffer, "\r\n", &temp);
        result = strcmp(temp, "");
    }while (result != 0);        

    sscanf(token, "%d %s", &result, domain);
    fprintf(stdout, "%s\n", domain);
    /*****************************************************/
    
    //Send our hello to the server
    sprintf(sendBuffer, "EHLO %s\n", domain);
    write(sockfd, sendBuffer, strlen(sendBuffer));
    
    //Get the response
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
    recvBuffer[msgSize] = '\0';
//    fprintf(stdout, "%s", recvBuffer);
    
    return 0;
}
int setAttachment(int sockfd, const char* path);
void writeMetadata(int sockfd, const char* sender, const char* receiver){
    int msgSize;
    char buff[512];
    memset(buff, '\0', sizeof(buff));
    
    write(sockfd, "DATA\n", strlen("DATA\n"));
    
    msgSize = read(sockfd, buff, sizeof(buff) - 1);
    buff[msgSize] = '\0';
    fprintf(stdout, "%s", buff);
    
    //Get current time
    char *dates = (char*)malloc(sizeof(char) * 512);
    time_t t = time(NULL);
    struct tm *times = localtime(&t);
    strftime(dates, 512, "%a, %d %B %Y %T %z", times);
    fprintf(stdout, "%s\n", dates);
    
    //Write the date
    sprintf(buff, "Date: %s\n", dates);
    write(sockfd, buff, strlen(buff));
    
    //Write the sender
    sprintf(buff, "From: %s\n", sender);
    write(sockfd, buff, strlen(buff));
    
    //Write the recipient
    sprintf(buff, "To: %s\n", receiver);
    write(sockfd, buff, strlen(buff));
    
    free(dates);
}