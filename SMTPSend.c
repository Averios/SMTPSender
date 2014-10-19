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

int main(int argc, char **argv){
    char *domain = (char*)malloc(sizeof(char) * 512);
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
    char *address = (char*)malloc(sizeof(char) * 30);
    //Loop through all the result
    for(iter = servaddr; iter != NULL; iter->ai_next){
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
    freeaddrinfo(servaddr);
    free(address);
    /***************************************************************************/
    
    //Interacting with the server
    /***************************************************************************/
    //Send and receive buffer
    char recvBuffer[1024], sendBuffer[4096];
    memset(sendBuffer, 0, sizeof(sendBuffer));
    memset(recvBuffer, 0, sizeof(recvBuffer));
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

    free(domain);
    //Sending e-mail
    /***************************************************************************/
    char *mail = (char*)malloc(sizeof(char) * 50);
    //Specify the sender
    do{
        //Get the sender
        fprintf(stdout, "Specify the sender : ");
        scanf("%s", mail);
        sprintf(sendBuffer,"MAIL FROM: %s\n",mail);
        
        write(sockfd, sendBuffer, strlen(sendBuffer));
        
        //Print the response
        msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
        recvBuffer[msgSize] = '\0';
//        fprintf(stdout, "%s", recvBuffer);
//        result = strcmp(recvBuffer, "250 2.1.0 Ok\r\n");
        sscanf(recvBuffer, "%d", &result);
    }while(result != 250);
    
    char *sender = (char*)malloc(sizeof(char) * 50);
    strcpy(sender, mail);
    
    //Specify the receiver
    do{
        //Get the receiver
        fprintf(stdout, "Specify the recipient : ");
        scanf("%s", mail);
        sprintf(sendBuffer,"RCPT TO: %s\n",mail);
        
        write(sockfd, sendBuffer, strlen(sendBuffer));
        
        //Print the response
        msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
        recvBuffer[msgSize] = '\0';
//        fprintf(stdout, "%s", recvBuffer);
        sscanf(recvBuffer, "%d", &result);
        if(result != 250) fprintf(stderr, "There is no such address in this server\n");
    }while(result != 250);
        
    char *reciever = (char*)malloc(sizeof(char) * 50);
    strcpy(reciever, mail);
    
    free(mail);
    
    //Write data
    write(sockfd, "DATA\n", strlen("DATA\n"));
    
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
    recvBuffer[msgSize] = '\0';
//    fprintf(stdout, "%s", recvBuffer);
    
    //Get current time
    char *dates = (char*)malloc(sizeof(char) * 512);
    time_t t = time(NULL);
    struct tm *times = localtime(&t);
    strftime(dates, sizeof(dates), "%a, %d %B %Y %T %z", times);
    fprintf(stdout, "%s\n", dates);
    
    //Write the date
    sprintf(sendBuffer, "Date: %s\n", dates);
    write(sockfd, sendBuffer, strlen(sendBuffer));
    
    //Write the sender
    sprintf(sendBuffer, "From: %s\n", sender);
    write(sockfd, sendBuffer, strlen(sendBuffer));
    
    //Write the subject
    fprintf(stdout, "Subject : ");
    fgets(sendBuffer, sizeof(sendBuffer), stdin);
    
    write(sockfd, "Subject: ", strlen("Subject: "));
    write(sockfd, sendBuffer, strlen(sendBuffer));
    
    //Write the recipient
    sprintf(sendBuffer, "To: %s\n", reciever);
    write(sockfd, sendBuffer, strlen(sendBuffer));
    
    free(dates);
    free(sender);
    free(reciever);
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
    
    
    /***************************************************************************/
    
    
    //Exit from the server
    sprintf(sendBuffer, "QUIT\n");
    write(sockfd, sendBuffer, strlen(sendBuffer));
    
     //Print the response
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
    recvBuffer[msgSize] = '\0';
    fprintf(stdout, "%s", recvBuffer);
    
    /***************************************************************************/
    
    
    close(sockfd4);
    close(sockfd6);
    return 0;
}