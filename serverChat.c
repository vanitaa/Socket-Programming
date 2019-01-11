#include <stdio.h>
#include <stdlib.h>
#include <errno.h> 
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>    //close 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 

#define TRUE 1
#define FALSE 0

/*
opt: Option (TRUE/FALSE)
client_socket: list of all the client file descriptors
max_client: Max client that can connect to server
sd: socket descriptor iterator
max_sd: maximum socket descriptor
sockfd: socket file descriptor
newsockfd: new file descriptor for accepted connection
portno: Port number
sent: flag for successful write operation
buffer: message buffer
serv_addr: contains server local address and port number
cli_addr: contains client address
clilen: size of strct cli_addr
readfds: set of socket descriptors
*/
int opt = TRUE, client_socket[30], max_clients = 30, sd, max_sd, sockfd, newsockfd, portno, sent;
char buffer[255];
struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen;
fd_set readfds;  

void error(const char *);
void writeHandler(int);
void listn();
void close_all_sd();
void chat();

/*
Function to print error message
Inputs: Error message
Returns: void
Call: error((char *)message)
*/
void error(const char *msg) {
	perror(msg);
	exit(1);
}

/*
Function to handle keboard interrupt
Inputs: sig_num
Returns: void
CALL_ON_INTERPUTION
*/
void writeHandler(int sig_num)
{
    /* Reset handler to catch SIGINT next time.
       Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, writeHandler);
    int temp_sd;
    printf("\b\bEnter receivers socket fd: ");
    scanf("%d",&temp_sd);
	while ((getchar()) != '\n');
    printf("\b\bServer: ");
    bzero(buffer, 255);
	fgets(buffer, 255, stdin);
	// sending the message
	if (write(temp_sd, buffer, strlen(buffer)) < 0) {
		error("Error on writing");
	}
	sent = TRUE;
	// if the sent message is "Bye"
	// terminates the connection
	if (strncmp("Bye", buffer, 3) == 0)	{
		// close(sockfd);
		close_all_sd();
		exit(0);
	}
	bzero(buffer, 255);
    fflush(stdout);
}


/*
Function to:
	-listen for clients on socket
	-Accept incoming connection
	-Call the chat function to start the client server communication

Inputs: void
Returns: void
Call: listn()
*/
void listn() {
	//Listening to atmost 4 pending connections
	listen(sockfd, 4);
	printf("Accepting Connections...\n");
	clilen = sizeof(cli_addr);

	chat();


}

/*
Function to:
	-Safely close all the open connection ports
	-Safely terminate the program

Inputs: void
Returns: void
Call: close_all_sd()
*/
void close_all_sd() {
	for (int i = 0 ; i < max_clients ; i++) {  
		//socket descriptor 
		sd = client_socket[i];
		close( sd );  
		client_socket[i] = 0;  
	}
	close (sockfd);
	exit(0);
}

/*
Function to:
	-deploy client serve communication
	-message transmission is done, one message at a time

Inputs: void
Returns: void
Call: chat()
*/
void chat() {
	while (1) {
		signal(SIGINT, writeHandler);

		FD_ZERO(&readfds);  
    
        //add master socket to set 
        FD_SET(sockfd, &readfds);  
        max_sd = sockfd;  
            
        //add child sockets to set 
        for (int i = 0 ; i < max_clients ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];  
                
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }  
    
        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        if ((select( max_sd + 1 , &readfds , NULL , NULL , NULL) < 0) && (errno!=EINTR))  
        {  
            printf("Error in Select");  
        }  
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (!sent && FD_ISSET(sockfd, &readfds))  
        {  
            if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t*)&clilen))<0)  
            {  
                error("Error on accept"); 
            }  
            
            //inform user of socket number - used in send and receive commands 
            printf("New Connection Established! Socket fd is %d , IP is : %s , Port : %d\n" , newsockfd , inet_ntoa(cli_addr.sin_addr) , ntohs(cli_addr.sin_port));  
                          
            //add new socket to array of sockets 
            for (int i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = newsockfd;  
                    printf("Adding to list of sockets as %d\n" , i);  
                        
                    break;  
                }  
            }  
            printf("Waiting for incomming message... \nPress ctrl+c whenever you want to send message!\n\n");
        }  
            
        //else its some IO operation on some other socket
        for (int i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
            if (!sent && FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if (read( sd , buffer, 255) == 0 )
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&cli_addr , (socklen_t*)&clilen);  

					printf("\nHost disconnected , ip %s , port %d \nEnd portal? (y/n): ", inet_ntoa(cli_addr.sin_addr) , ntohs(cli_addr.sin_port));

					bzero(buffer, 255);
					char e;
					scanf("%c",&e);
					while ((getchar()) != '\n');
					if (e == 'y')	{
						close_all_sd();
					}
                        
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i] = 0;  
                }  
                    
                //Echo back the message that came in 
                else
                {  
                	printf("\nClient (sd: %d): %s\n",sd , buffer);
                }  
            }  
        } 
        sent = FALSE;
		
	}
}

/*
main Function

Inputs: port number
Returns: 0
Call: ./serverChat 9898
*/
int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr,"Port number not provided. Program terminated.\n");
		exit(1);
	}
	
	//initialise all client_socket[] to 0 so not checked 
    for (int i = 0; i < max_clients; i++)  {  
        client_socket[i] = 0;  
    } 

    //create a master socket 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("Error opening Socket.");
	}

    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	// Binding the socket to localhost aka 127.0.0.1
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("Binding Failed");

	// Initial call to listen
	listn();

	// Closing all the sockets
	close(newsockfd);
	close(sockfd);
	return 0;

}
