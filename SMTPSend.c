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
#include "base64.h"
int serv_connect(const char* server, const char* protocol);
int handshake(int sockfd);
void writeMetadata(int sockfd, const char* sender, const char* receiver);
int setAttachment(int sockfd, char *pesan, const char* path,int attempt);
int login(int sockfd);
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
    while((sockfd = serv_connect(domain, "smtp")) == -1){
    	printf("Can't connect SMTP server, please try again.\n");
    	printf("Please insert the mail server domain :\n");
   		printf("smtp://");
   		scanf("%s", domain);
	}
    /***************************************************************************/

    //Handshaking the server
    /***************************************************************************/
    int retval;
    if((retval = handshake(sockfd)) != 0) return 1;
    /***************************************************************************/
    //Login
    if(!login(sockfd)) return 1;
    //Specify the Sender and Receiver
    /***************************************************************************/
    int result;
    int msgSize;
    char recvBuffer[1024], sendBuffer[2048];
    memset(sendBuffer, '\0', sizeof(sendBuffer));
    memset(recvBuffer, '\0', sizeof(recvBuffer));
    char *mail = (char*)malloc(sizeof(char) * 50);
    //Specify the sender
        write(sockfd, "EHLO\n", strlen("EHLOn"));
        //Print the response
        msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
        //printf("%s",recvBuffer);
    do{
        //Get the sender
        fprintf(stdout, "Specify the sender : ");
        fscanf(stdin, "%s", mail);
        sprintf(sendBuffer,"MAIL FROM: %s\n",mail);
    printf("%s",sendBuffer);
        write(sockfd, sendBuffer, strlen(sendBuffer));

        //Print the response
        msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
        recvBuffer[msgSize] = '\0';
        printf("%s",recvBuffer);
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
        fprintf(stdout, "%s", recvBuffer);
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

    char pesan[10000]={0},attach,path[100];
    //Write the message
    /***************************************************************************/
    fprintf(stdout, "Write your message below\n");
    do{
        fgets(sendBuffer, sizeof(sendBuffer), stdin);
        result = strcmp(sendBuffer, ".\n");
        if(!result)break;
        strcat(pesan,sendBuffer);
    } while (result != 0);
    //strcat(pesan,". \n");
    fprintf(stdout,"Any Attachment(y/n)? ");
    fscanf(stdin,"%c",&attach);
    if(attach=='y'){
    fprintf(stdout,"File Path->");
    fscanf(stdin,"%s",path);
    setAttachment(sockfd,pesan,path,1);
    }
    write(sockfd,"\r\n.\r\n",sizeof("\r\n.\r\n"));
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
    recvBuffer[msgSize] = '\0';
    fprintf(stdout, "%s", recvBuffer);
    /***************************************************************************/

    //Exit from the server
    write(sockfd, "QUIT\n", strlen("QUIT\n"));

     //Print the response
    msgSize = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
    recvBuffer[msgSize] = '\0';
    //fprintf(stdout, "%s", recvBuffer);


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

    return 0;
}
int login(sockfd){
    char user[50],sendBuffer[300]="AUTH LOGIN\n",pass[50],recvBuffer[100],encodeduser[100],encodedpass[100];
    int msgSize;
    fprintf(stdout,"Login Authentication\nUsername: ");
   // write(sockfd, sendBuffer, sizeof(sendBuffer));
    //read(sockfd,recvBuffer,sizeof(recvBuffer));

    fscanf(stdin,"%s",user);
    b64_encode(user, encodeduser);
    strcat(encodeduser,"\n");
    //write(sockfd, encodeduser, sizeof(encodeduser));
    //read(sockfd,recvBuffer,sizeof(recvBuffer));
    fprintf(stdout,"Password: ");
    fscanf(stdin,"%s",pass);
    b64_encode(pass, encodedpass);
    strcat(encodedpass,"\n");
    strcat(sendBuffer,encodeduser);
    strcat(sendBuffer,encodedpass);
    write(sockfd, sendBuffer, sizeof(sendBuffer));
    read(sockfd,recvBuffer,sizeof(recvBuffer));
    //fprintf(stdout,"%s ",recvBuffer);
    return 1;
}
int setAttachment(int sockfd, char *pesan, const char* path,int attempt){

    char temp[1000000]={0};
    char filebuffer[1024]={0};
    char fileencoded[100000]={0};
    int filed = open(path, O_RDONLY);
	if(attempt == 1)  {
        strcat(temp,"MIME-Version: 1.0\r\n");
		strcat(temp,"Content-Type: multipart/mixed; boundary=047d7b6251b0e4dbce04e92b1110\r\n");
		strcat(temp, "\r\n--047d7b6251b0e4dbce04e92b1110\r\n");
		strcat(temp, "Content-Type: multipart/alternative; boundary=047d7b6251b0e4dbca04e92b110e\r\n");
		strcat(temp, "\r\n--047d7b6251b0e4dbca04e92b110e\r\n");
		strcat(temp, "Content-Type: application/zip; charset=ISO-8859-1\r\n\r\n");
		strcat(temp,pesan);  // isi pesan
		strcat(temp,"\r\n--047d7b6251b0e4dbca04e92b110e--\r\n");
		strcat(temp, "\r\n--047d7b6251b0e4dbce04e92b1110\r\n");
		}
	else {
		strcat(temp, "\r\n--047d7b6251b0e4dbce04e92b1110\r\n");
		}
    strcat(temp,"Content-Type: application/octet-stream\r\n");
    strcat(temp,"Content-Disposition: attachment; filename=\"");
    strcat(temp, path);
    strcat(temp,"\"\r\n");
    strcat(temp,"Content-Transfer-Encoding: base64\r\n");
    strcat(temp, "X-Attachment-Id: f_hn2soh39\r\n\r\n");
	//string str((std::istreambuf_iterator<char>(fin.rdbuf())),std::istreambuf_iterator<char>());
	//string encoded = base64_encode(reinterpret_cast<const unsigned char*>(str.c_str()), str.length());
	//temp += encoded;
	int msgSize;
	//memset(filebuf, '\0', sizeof(filebuf));
	do{
        msgSize = read(filed, filebuffer, sizeof(filebuffer) - 1);
	}while(msgSize > 0);
	b64_encode(filebuffer,fileencoded);
	strcat(temp,fileencoded);
	strcat(temp,"--047d7b6251b0e4dbce04e92b1110--");
    write(sockfd,temp,strlen(temp));
//	if(attempt == 1)	*data = temp;
	//else *data += temp;
}
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
