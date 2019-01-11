#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

char buffer[255];
int sockfd;

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

void sigHandler(int sig_num) {
    signal(SIGALRM, sigHandler);
	exit(0);
}

/*
Function to handle keboard interrupt
Inputs: sig_num
Returns: void
CALL_ON_INTERPUTION
*/
void *writeHandler()
{
    while (1) {
	    /* Reset handler to catch SIGINT next time.
	       Refer http://en.cppreference.com/w/c/program/signal */
	    // signal(SIGALRM, sigHandler);
		// Clearing chat buffer
	    bzero(buffer, 255);
		fgets(buffer, 255, stdin);
		printf("\x1B[A");
	    printf("%c[2K", 27);
	    printf("\b\bYou: %s",buffer);
		// while ((getchar()) != '\n');
		
		// sending the message
		// if the sent message is "Bye"
		// terminates the connection
		if (write(sockfd, buffer, strlen(buffer)) < 0) {
			error("Error on writing");
		}
		if (strncmp("Bye", buffer, 3) == 0)	{
			close(sockfd);
			exit(0);
		}
		bzero(buffer, 255);
	    fflush(stdout);
	}
}

/*
main Function

Inputs: raw server address, port number
Returns: 0
Call: ./clientChat 127.0.0.1 9898
*/
int main(int argc, char *argv[]) {
	/*
	sockfd: socket file descriptor
	portno: Port number
	buffer: message buffer
	serv_addr: contains server local address and port number
	server: server address
	*/
	int portno;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	if (argc < 3) {
		fprintf(stderr,"usage %s hostname port\n",argv[0]);
		exit(1);
	}

	portno = atoi(argv[2]);
	// Initializing socket with IPv4 TCP protocols
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("Error opening Socket.");
	}

	// Getting server address from raw address
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr, "Error, no such host");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	// sending request to accept the connection to the server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("Connetion Failed");
	} else {
		printf("Connection Established!\n");
	}
	printf("Waiting for incomming message... \n\n");

	pthread_t t;
	pthread_create(&t, NULL, writeHandler, NULL);
	while (1) {
		// signal(SIGINT, writeHandler);
		bzero(buffer, 255);
		// recieving the incoming message
		int n = read(sockfd, buffer, 255);
		if (n < 0) {
			error("Error on reading");
		} else if (n == 0) {
			pthread_cancel(t);
			break;
		}
		
		printf("\nServer: %s\n", buffer);
		// If the message from server is "Bye", 
		// terminate the connection
		if (strncmp("Bye", buffer, 3) == 0)	{
			if (write(sockfd, buffer, strlen(buffer)) < 0) {
				error("Error on closing connection.");
			}
			// printf("Server has left. Please press ENTER to securly terminate the connection!\n");
			pthread_cancel(t);
			break;
		}
	}
	// alarm(1);
	pthread_join(t,NULL);

	// Closing the socket
	close(sockfd);
	return 0;

}
