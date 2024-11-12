#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/wait.h>

struct fileParams {int fd; char fileHeader[1000]; char fileDirectoryPath[1000];};

void testConnect();

// This function will set up the network, if something goes wrong it will exit the program
int setupNetwork()
{
	// Set up a hints structure
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_PASSIVE;

	// Create a new addrinfo struct for the list of nodes in the
	// result and a node to represent the node we are currently using
	struct addrinfo *resultList, *currentNode;
	memset(&resultList, 0, sizeof(resultList));
	memset(&currentNode, 0, sizeof(currentNode));

	// Get address info with a port of 8000 and error check result
	int worked = getaddrinfo(NULL, "8000", &hints, &resultList);
	if(worked!=0)
	{
		perror("Address Info");
		exit(3);
	}

	int socketfd = 0; // Will hold the socket file descriptor

	for(currentNode = resultList; currentNode != NULL; currentNode = currentNode->ai_next)
	{
		// Attempt to make a socket
		socketfd = socket(currentNode->ai_family, currentNode->ai_socktype, currentNode->ai_protocol);
		if(socketfd == -1) continue; // This socket doesn't work, redo loop

		// Bind the socket
		worked = bind(socketfd, currentNode->ai_addr, currentNode->ai_addrlen);

		// If error, redo loop
		if(worked == -1)
		{
			close(socketfd);
			continue;
		}
		break; // We found a working, binded socket so exit the loop
	}
	freeaddrinfo(resultList);

	int BACKLOG = 1000; // variable to represent how long the listen backlog should be

	// listen for the socket
	worked = listen(socketfd, BACKLOG);
	if(worked == -1)
	{
		perror("Listen");
		exit(4);
	}

	return socketfd;
}

// This will parse the header to only take the file name from the header and stores it into fileName
int parseHeader(char* header, char* fileName)
{
	// Check for each letter of GET and the space
	if(header[0] != 'G') { printf("G failed\n"); return 1; }
	if(header[1] != 'E') { printf("E failed\n"); return 1; }
	if(header[2] != 'T') { printf("T failed\n"); return 1; }
	if(header[3] != ' ') { printf("Space failed\n"); return 1; }

	bool startOfFileFound = false;
	int nextSpaceInFileName = 0;

	// Loop through the string until the end
	for(int i = 4; header[i] != '\0'; i++)
	{
		// If we haven't found the start of the file and it's a meaningful char, we've found the first
		// meaningful char
		if(!startOfFileFound && header[i] != ' ' && header[i] != '/')
		{
			fileName[nextSpaceInFileName++] = header[i];
			startOfFileFound = true;
		}
		// while there is no space, keep adding to the string
		else if(startOfFileFound && header[i] != ' ')
			fileName[nextSpaceInFileName++] = header[i];
		else if(startOfFileFound) break;
	}

	return 0;
}

// This method will send a message to the client based on if the file is accessed or not, if the file is accessible
// send back the contents of the file to the client
void *sendMessage(void *voidFileParams)
{
	// Make the void pointer into the struct it actually is and extract the variables we need
	struct fileParams *myFileParams = (struct fileParams *)voidFileParams;

	char* header = myFileParams->fileHeader;
	char* directoryPath = myFileParams->fileDirectoryPath;
	int new_fd = myFileParams->fd;

	// Parse the header from the "GET" and store the file name in fileName
	char fileName[1000] = {0};
	char message[1000] = {0};

	int returnID = parseHeader(header, fileName);

	// if returnID == 1, the parseHeader function did not encounter "GET" so we cannot get a file
	if(returnID != 0) perror("Unidentified Command");
	// Otherwise, parseHeader found no errors so we can proceed
	else
	{
		// Take the directory and concat it with the fileName to get the full file path
		char fullPath[1000] = {0};
		strcat(fullPath, directoryPath);
		strcat(fullPath, fileName);

		// Open the file
		int file = open(fullPath, O_RDONLY);

		// File does not exist so send an error message
		if(file < 0)
		{
			char* errorMessage = "HTTP/1.0 404 Not Found\r\nFile not found in directory.\r\n\r\n";
			strcat(message, errorMessage);
			send(new_fd, message, strlen(message), 0);
		}
		// File does exist so return the file and the length of its contents
		else
		{
			// Create the message we will send back, code is 200 since we found the file
			char* successMessage = malloc(sizeof(char)*1000);
			successMessage[0] = '\0'; //Add a null terminator
			strcat(successMessage, "HTTP/1.0 200 OK\r\nContent-Length: ");

			// Find the number of bytes in the file using stat
			struct stat *statBuffer = malloc(sizeof(struct stat));
			int statWorked = stat(fullPath, statBuffer);
			int numBytes = statBuffer->st_size;
			char stringNumBytes[10];
			sprintf(stringNumBytes, "%d", numBytes);
			strcat(successMessage, stringNumBytes);

			free(statBuffer);

			// Add two new lines to the successMessage in accordance with HTTP
			char* twoNewLines = "\r\n\r\n";
			strcat(successMessage, twoNewLines);

			// Send the successMessage
			send(new_fd, (char *)successMessage, strlen(successMessage), 0);
			free(successMessage);

			// Go through the file and add send the file in chunks
			char buffer[1000];
			ssize_t numBytesRead = 0;
			while((numBytesRead = read(file, buffer, sizeof(buffer))) > 0)
			{
				send(new_fd, (char *)buffer, numBytesRead, 0);
			}
		}

		// teardown
		close(file);
	}
}

// This is a method meant to test if this thing works
void testConnect()
{
	struct addrinfo hints, *servinfo, *current;
	int sock, worked;

	// Set up hints structure
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Get the address info of the server
	worked = getaddrinfo("0.0.0.0", "8000", &hints, &servinfo);
	if(worked != 0)
	{
		perror("getaddrinfo");
		exit(1);
	}

	// Connect to a socket
	for(current = servinfo; current != NULL; current = current->ai_next)
	{
		sock = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
		if(sock == -1) continue;

		if(connect(sock, current->ai_addr, current->ai_addrlen) == -1)
		{
			close(sock);
			continue;
		}
		break;
	}
	// Free the address info since we no longer need it
	freeaddrinfo(servinfo);

	// Send a message to the server, THIS DETERMINES THE FILE TO LOOK FOR IN THIS FUNCTION
	char* sentMessage = "GET /2of12.txt ";
	send(sock, sentMessage, strlen(sentMessage), 0);

	// Receive a message from the server
	char messageReceived[1000] = {0};
	messageReceived[0] = '\0';
	ssize_t numBytesReceived = 0;
	while((numBytesReceived = recv(sock, (char*)messageReceived, sizeof(messageReceived), 0)) > 0)
	{
		messageReceived[numBytesReceived] = '\0';
		printf("%s", messageReceived);
	}
}

int main(int argc, char** argv)
{
	// If there is no file path parameter we must exit the program
	if(argc < 2)
	{
		//testConnect() //This was used for testing the WebServer
		printf("Missing File Path\n");
		return 1;
	}
	else
	{
		// Place the file path from the command line argument into a string
		char directoryPath[1000] = {0};
		sscanf(argv[1], "%s", directoryPath);

		// Get the socket from setupNetwork
		int socketfd = setupNetwork();

		// Set up variables for accept() loop
		socklen_t sin_size;
		struct sockaddr_storage their_addr;
		int new_fd;

		// accept() whenever it's available
		while(1)
		{
			sin_size = sizeof(their_addr);
			new_fd = accept(socketfd, (struct sockaddr *)&their_addr, &sin_size);

			// Get the file name from client input
			char header[1000] = {0};
			recv(new_fd, (char *)header, sizeof(char)*1000, 0);

			// Initialize a struct that will be passed to the thread with
			// the file descriptor, header and directoryPath
			struct fileParams *myFileParams = malloc(sizeof(struct fileParams));

			// We have to initialize fileHeader and fileDirectoryPath with a terminating character
			myFileParams->fileHeader[0] = '\0';
			myFileParams->fileDirectoryPath[0] = '\0';

			myFileParams->fd = new_fd;
			strcat(myFileParams->fileHeader, header);
			strcat(myFileParams->fileDirectoryPath, directoryPath);

			// Make a thread that will send a message back to the user, We need new_fd, header and directoryPath
			pthread_t id;
			pthread_create(&id, NULL, sendMessage, (void *)myFileParams);
			pthread_join(id, NULL);

			// teardown
			free(myFileParams);
			close(new_fd);
		}
	}
}
