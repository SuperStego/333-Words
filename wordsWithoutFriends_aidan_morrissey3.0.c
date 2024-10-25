#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int *getLetterDistribution(int*,char*);
bool compareCounts(char*,char*);
char* prepareWord(char*);
void printGameListNode();
struct wordListNode* getRandomWord(int);
void acceptInput();
void gameLoop(char*);
void tearDown();
int initialization();
void findWords(char* masterWord);
void displayWorld();
void printWordListNode();
void cleanupWordListNodes();
void cleanupGameListNodes();

struct wordListNode{char word[30];struct wordListNode* next;};
struct gameListNode{char word[30]; bool found; struct gameListNode* next;};

struct wordListNode* root = NULL;
struct gameListNode* gameRoot = NULL;

int main(int argc, char **argv)
{
	int size = initialization();
	char* masterWord = getRandomWord(size)->word;
	findWords(masterWord);
	gameLoop(masterWord);

	tearDown();

	return 0;
}

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

//Runs the game
void gameLoop(char* masterWord)
{
	do
	{
		displayWorld(masterWord);
		acceptInput();
	}while(!isDone());
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

// This will print the words within the gameListNode out. If the word is found it will print "FOUND:XXXX" with
// XXXX being the word, but if it's not found it will print "_" for each character in the word
void printGameNodeStatus()
{
	struct gameListNode *currentNode = gameRoot;
	while(currentNode!=NULL)
	{
		// If the word is not found print "_" for each character in the word
		if(currentNode->found == false)
		{
			int length = strlen(currentNode->word); // Saving space by making length a variable first
			for(int i = 0; i < length; i++)
			{
				printf("_ ");
			}

			// Make a newline after for cleanliness
			printf("\n");
		}
		else
		{
			printf("FOUND : %s\n", currentNode->word);
		}

		// Move to next node
		currentNode = currentNode->next;
	}
}

//Will print the puzzle
void displayWorld(char* masterWord)
{
	// Sort the masterWord in ascending order
	sortWord(masterWord);

	//Print the sorted masterWord (with spaces between letters) and a buffer line
	for(int i = 0; masterWord[i] != '\0'; i++)
	{
		printf("%c ", masterWord[i]);
	}
	printf("\n-----------------------\n");

	printGameNodeStatus();
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

//Finds the initial input from the user
void acceptInput()
{
	printf("Enter a guess: ");

	// Collect guess and prepare to compare it with other words
	char str[30];
	fgets(str,30,stdin);
	prepareWord(str);

	// Compare the input to every word in the gameListNode linked list
	compareToGameList(str);

}

//Ends the program
void tearDown()
{

	// Print the final status
	printGameNodeStatus();

	// Free memory for dictionary
	cleanupWordListNodes();

	// Free memory for selected words from dictionary
	cleanupGameListNodes();

	printf("All Done\n");
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

//Prints the contents of the wordListNode
void printWordListNode(){
	struct wordListNode* placeholderNode = root;
	while(placeholderNode!=NULL){
		printf("%s\n",placeholderNode->word);
		placeholderNode = placeholderNode->next;
	}
}
