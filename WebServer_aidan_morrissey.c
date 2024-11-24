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
#include <ctype.h>
#include <time.h>

bool compareCounts(char*,char*);
char* prepareWord(char*);
struct wordListNode* getRandomWord(int);
void findWords(char* masterWord);
void cleanupWordListNodes();
void cleanupGameListNodes();

struct wordListNode{char word[30];struct wordListNode* next;};
struct gameListNode{char word[30]; bool found; struct gameListNode* next;};

struct wordListNode* root = NULL;
struct gameListNode* gameRoot = NULL;

char directoryPath[1000] = {0};
char *htmlHeader = "HTTP/1.1 200 OK\r\ncontent-type: text/html; charset=UTF-8\r\n\r\n";
bool alreadyCreated = false;
int numWords = 0;
char masterWord[30] = {0};

//Sets up the game
int initialization()
{
	srand(time(0));
	/*Open a file with every word*/
	FILE *fp;

	root = malloc(sizeof(struct wordListNode));
	fp = fopen("2of12.txt","r");
	if(fp == NULL) {
		printf("Oopsie, no file found!");
		exit(1);
	}
	char* wordReturn = NULL;
	root->next = NULL;
	struct wordListNode* newNode=root;
	struct wordListNode* placeholderNode = malloc(sizeof(struct wordListNode));
	placeholderNode->next = NULL;

	//Loop through the file and fill the linked list with words
	wordReturn = fgets(root->word, 30, fp);

	//Setting up root first
	if(wordReturn!=NULL){
		prepareWord(root->word);
		root->next = placeholderNode;
		wordReturn = fgets(placeholderNode->word,30,fp);
	}
	int numWords = 0;
	//Loop through rest of file
	while(wordReturn!=NULL){
		prepareWord(placeholderNode->word);
		numWords++;
		newNode->next = placeholderNode;
		newNode = newNode->next;
		placeholderNode = malloc(sizeof(struct wordListNode));
		placeholderNode->next = NULL;
		wordReturn = fgets(placeholderNode->word,30,fp);
	}
	free(placeholderNode);
	if(fclose(fp) == EOF) printf("Oopsie, error closing file!");
	return numWords;
}

bool isDone()
{
	struct gameListNode *currentNode = gameRoot;

	// Iterate through the gameListNode linked list and return false if any word has not been found
	while(currentNode != NULL)
	{
		if(currentNode->found == false)
			return false;
		currentNode = currentNode->next;
	}
	return true;
}

// This will sort a word in ascending order
void sortWord(char* masterWord)
{
	char character = 0; // Temp char for the sorting

	// Sort masterWord
	for(int i = 0; masterWord[i]!='\0'; i++)
	{
		for(int j = i+1; masterWord[j]!='\0'; j++)
		{
			if(masterWord[i]>masterWord[j])
			{
				character = masterWord[i];
				masterWord[i] = masterWord[j];
				masterWord[j] = character;
			}
		}
	}
}

// Will add the word to the line for easier testing
void addWordForTesting(char *buffer, char *currentWord)
{
	strcat(buffer, currentWord);
}

// This will display the words within the gameListNode linked list. If the word is found it will send "FOUND:XXXX"
// to the clientwith XXXX being the word, but if it's not found it will send "_" to the client for each character
// in the word
void displayGameNodeStatus(char *buffer, int socket)
{
	struct gameListNode *currentNode = gameRoot;
	for(int i = 0; currentNode!=NULL; i++)
	{
		// If the word is not found print "_" for each character in the word
		if(currentNode->found == false)
		{
			int length = strlen(currentNode->word); // Saving space by making length a variable first

			// Trying to separate it so there will be 10 words per line to make it look cleaner
			if(i%10 == 0) strcat(buffer, "<P> ");
			else strcat(buffer, "&emsp;");
			for(int i = 0; i < length; i++)
			{
				strcat(buffer, "_ ");
			}

			// test method
			//addWordForTesting(buffer, currentNode->word);

			// Make a newline after for cleanliness
			if(i%10 == 9) strcat(buffer, "\n");
		}
		else
		{
			// The %10 is an attempt to gather 10 words along each line to make it look cleaner
			if(i%10 == 0) strcat(buffer, "<P> FOUND: ");
			else strcat(buffer, "&emsp; FOUND: ");

			// Add the word to the buffer and a new line if this is the last word in the line
			strcat(buffer, currentNode->word);
			if(i%10 == 9) strcat(buffer, "\n");
		}

		// Move to next node
		currentNode = currentNode->next;
	}

	strcat(buffer, "</body></html>");

	send(socket, (char*)buffer, strlen(buffer), 0);
}

//Will print the puzzle
void displayWorld(char* masterWord, int socket)
{
	// Sort the masterWord in ascending order
	sortWord(masterWord);

	// Create a buffer and add the html header to the start of it
	char buffer[100000] = {0};
	strcat(buffer, htmlHeader);

	strcat(buffer, "<html><title>Words Without Friends</title><body>\n<P> <font size = \"5\"><b>Words Without Friends</b></font>\n<P>How to play:\n<P>Use the available letters to make words (the number of underscrores corresponds to the number of letters within a word).\n<P>Some included words are just a single letter.\n<P>There are a maximum of 10 words in each line.\n<P>Available letters: ");

	// add the sorted masterWord to the buffer (with spaces between letters) and a buffer line
	for(int i = 0; masterWord[i] != '\0'; i++)
	{
		// Create a new string comprising of the current charcter in the master word and a space
		char masterWordCharWithSpace[3] = {0};
		masterWordCharWithSpace[0] = masterWord[i];
		masterWordCharWithSpace[1] = ' ';
		masterWordCharWithSpace[2] = '\0';

		strcat(buffer, masterWordCharWithSpace);
	}

	// Add a buffer line
	char *bufferLine = "\n<P> -------------------------------------------------------------------------------\n";
	strcat(buffer, bufferLine);

	// Add an input text box to the html
	strcat(buffer, "<P>Enter your guess into the text box:\n<form submit =\"words\"><input type =\"text\" name=move autofocus ></input></form>\n");

	displayGameNodeStatus(buffer, socket);
}

// Compares the input to every word within the gameListNode linked list
void compareToGameList(char* input)
{
	struct gameListNode *currentNode = gameRoot;

	// Iterate through the list and if the input is equal to the word, change found to true
	while(currentNode!=NULL)
	{
		if(strcmp(currentNode->word, input) == 0)
			currentNode->found = true;
		currentNode = currentNode->next;
	}
}

// This will start the game for the webserver
void startGame(int socket)
{
	if(!alreadyCreated)
	{
		// Create the wordListNodes since they haven't been created before
		numWords = initialization();

		// Get a new master word and generate a list of words that contain the letters in master word
		// with findWords()
		strcat(masterWord, getRandomWord(numWords)->word);
		findWords(masterWord);

		// Set alreadyCreated to true for future reference
		alreadyCreated = true;
	}
	else
	{
		// Cleanup the previous game since it existed
		cleanupGameListNodes();

		// Get a new master word and generate a list of words that contain the letters in master word
		// with findWords()
		strcat(masterWord, getRandomWord(numWords)->word);
		findWords(masterWord);
	}

	// Display the game in the web browser
	displayWorld(masterWord, socket);
}

// Displays a game over screen on the webserver
void displayGameOverScreen(int socket)
{
	// Create the game over message
	char *gameOverMessage = "<html><title>You Win!</title><body><P><font size=\"7\"><b>You Win!</b></font>\n<P>Congratulations! You solved it! \n<P><a href = \"words\">Another?</a></body></html>";

	// Make a full message that includes the html header
	char fullMessage[strlen(gameOverMessage)+strlen(htmlHeader)+1];
	strcat(fullMessage, htmlHeader);
	strcat(fullMessage, gameOverMessage);

	// Ensure null termination
	fullMessage[strlen(fullMessage)] = '\0';

	// Send the message to the server
	send(socket, (char *)fullMessage, strlen(fullMessage), 0);
}

//Finds the initial input from the user
void acceptInput(char *url, int socket)
{
	if(url[5] == '\0')
		startGame(socket);
	else
	{
		int urlStart = 11; // The start of the guess in the url
		char str[30] = {0}; // Will contain the guess

		// Populate str with the guess values from the url
		for(int i = 0; i < 30 && url[i+urlStart] != '\0'; i++)
			str[i] = url[i+urlStart];

		// Prepare the word and compare it to every word in the gameListNode linked list
		prepareWord(str);
		compareToGameList(str);

		if(isDone())
			displayGameOverScreen(socket);
		else
		{
			displayWorld(masterWord, socket);
		}
	}
}

//Returns a struct containing a random word from the linked list with more than 6 letters
struct wordListNode* getRandomWord(int size){
	int position = rand()%size;
	struct wordListNode* currentNode = root;
	for(int i = 0; i<position && currentNode!=NULL; i++)
		currentNode = currentNode->next;
	while(currentNode!=NULL){
		if(strlen(currentNode->word)>=6)
			return currentNode;
		currentNode = currentNode->next;
	}
	//If it reaches the end of the list call the function again to find a better word
	if(currentNode==NULL)
		return getRandomWord(size);
	return currentNode;
}

//Populates the gameListNode linked list with words that contain some of the letters in the master word
void findWords(char* masterWord){
	//Setting up some gameListNodes
	gameRoot = malloc(sizeof(struct gameListNode));
	gameRoot->found = false;
	gameRoot->next = NULL;
	struct gameListNode* newGameNode;
	struct gameListNode* gameNode = malloc(sizeof(struct gameListNode));
	gameNode->found = false;
	gameNode->next = NULL;

	//Sets placeholderNode to root
	struct wordListNode *rootRepresentation = NULL;
	if(root != NULL)
		rootRepresentation = root;

	bool firstWordFound = false;

	//Iterating through wordListNode
	while(rootRepresentation != NULL) {
		if(compareCounts(masterWord, rootRepresentation->word)){
			//Different process for the first time (gameRoot) and the second time through
			//so the second iteration won't damage the root
			if(!firstWordFound){
				strcpy(gameRoot->word, rootRepresentation->word);
				gameRoot->found = false;
				newGameNode = gameRoot;
				gameRoot->next = gameNode;
				firstWordFound = true;
			}
			else{
				strcpy(gameNode->word, rootRepresentation->word);
				newGameNode->found = false;
				newGameNode->next = gameNode;
				newGameNode = newGameNode->next;
				gameNode = malloc(sizeof(struct gameListNode));
				gameNode->found = false;
				gameNode->next = NULL;
			}
		}
		rootRepresentation = rootRepresentation->next;
	}
	free(gameNode);
}

//This will increment an int according to which letter is found, letters must be capitalized
//Every instance of the letter in word will increment the desired location by one
int *getLetterDistribution(int letters[26], char* word)
{
	for(int i = 0; word[i] != '\0'; i++)
	{
		//13 is added so A(65) will be added to 78
		//so when %26 is added, A will correspond to 0
		letters[(word[i]+13)%26]++;
	}
	return letters;
}

//This will see if the candidate word contains the letters in the inputWord
bool compareCounts(char* inputWord, char* candidate)
{
	int inputArray[26] = {0};
	int candidateArray[26] = {0};

	getLetterDistribution(inputArray, inputWord);
	getLetterDistribution(candidateArray, candidate);

	for(int i = 0; i < 26; i++){
		if(candidateArray[i]>inputArray[i])
			return false;
	}
	return true;
}

//Test method
void TestDist(char *str, int *dist)
{
	printf("%s\n",str);
	for(int i = 0; i < 26; i++)
		printf("%d",dist[i]);
	printf("\n");
}

//Removes the carriage returns, line feeds and capitalizes letters in a string
char *prepareWord(char* str){
	for(int i = 0; str[i] != '\0'; i++)
	{
		//Remove carriage returns and line feeds so just the letters will be printed
		if(str[i] == '\r' || str[i] == '\n')
			//'r' and '\n' will only be at the back in this program
			str[i] = '\0';
		else
			str[i] = toupper(str[i]);
	}
	return str;
}

// Frees the memory associated with the current game
void cleanupGameListNodes()
{
	//Free the gameListNode linked list
	struct gameListNode* newGameNode = NULL;
	while(gameRoot!=NULL){
		newGameNode = gameRoot;
		gameRoot = gameRoot->next;
		free(newGameNode);
	}
}

//Prints the contents of the gameListNode
void printGameListNode(){
	struct gameListNode* newGameNode = gameRoot;
	while(newGameNode!=NULL){
		printf("%s\n",newGameNode->word);
		newGameNode = newGameNode->next;
	}
}

// Frees the memory associated with the dictionary
void cleanupWordListNodes()
{
	struct wordListNode* placeholderNode = NULL;
	while(root!=NULL){
		placeholderNode = root;
		root = root->next;
		free(placeholderNode);
	}
}

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
void *sendMessage(void *voidfd)
{
	int new_fd = *((int*)voidfd);

	// Get the file name from client input
	char header[1000] = {0};
	recv(new_fd, (char *)header, sizeof(char)*1000, 0);

	// Parse the header from the "GET" and store the file name in fileName
	char fileName[1000] = {0};
	char message[1000] = {0};

	int returnID = parseHeader(header, fileName);

	// if returnID == 1, the parseHeader function did not encounter "GET" so we cannot get a file
	if(returnID != 0) perror("Unidentified Command");
	// Otherwise, parseHeader found no errors so we can proceed
	else
	{
		if(fileName[0] == 'w' && fileName[1] == 'o' && fileName[2] == 'r' && fileName[3] == 'd' && fileName[4] == 's')
			acceptInput(fileName, new_fd);
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
			close(file);
			close(new_fd);
		}
	}
}

int main(int argc, char** argv)
{
	// If there is no file path parameter we must exit the program
	if(argc < 2)
	{
		printf("Missing File Path\n");
		return 1;
	}
	else
	{
		// Place the file path from the command line argument into a string
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

			// convert new_fd to a pointer so it can be passed as a parameter
			int* ptr = &new_fd;

			// Make a thread that will send a message back to the user, We need new_fd, header and directoryPath
			pthread_t id;
			pthread_create(&id, NULL, sendMessage, (void *)ptr);
		}
		close(socketfd);
	}
}
