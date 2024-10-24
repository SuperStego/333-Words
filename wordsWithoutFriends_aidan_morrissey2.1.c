#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int *getLetterDistribution(int*,char*);
bool compareCounts(char*,char*);
char* removeCarriage(char*);
void printGameListNode();
struct wordListNode* getRandomWord(int);
void acceptInput();
void gameLoop();
void tearDown();
int initialization();
void findWords(char* masterWord, int size);
void displayWorld();
void printWordListNode();

struct wordListNode{char word[30];struct wordListNode* next;};
struct gameListNode{char gameWord[30]; bool found; struct gameListNode* next;};

struct wordListNode* root = NULL;
struct gameListNode* gameRoot = NULL;

/*Howdy grader, to save you some time here's the problems I found, but can't figure out how to fix:
words from the gameListNode list are added to the wordListNode during findWords() for a reason I can't see
Occasional seg fault from iterating through gameListNode or wordListNode (likely because of the last issue)
*/
int main(int argc, char **argv)
{
	//TODO: Test compareCounts
	/*if(compareCounts("CAT","DOG"))
		printf("Oops compare failure 1\n");
	if(!compareCounts("CAT", "CAT"));
		printf("Oops compare failure 2\n");
	if(compareCounts("CONCACA", "CCCC"));
		printf("Oops compare failure 3\n");
	if(compareCounts("CATS", "CATSA"));
		printf("Oops compare failure 4\n");*/
	int size = initialization();
	char* masterWord = getRandomWord(size)->word;
	//TODO: printing masterWord
	printf("%s\n", masterWord);
	//TODO:removed for testing right now
	findWords(masterWord, size);
	gameLoop();
	tearDown();

	return 0;
}
//Sets up the game
int initialization()
{
	srand(time(0));
	/*Open a file with every word*/
	FILE *fp;
	int numWords = 0;

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
		removeCarriage(root->word);
		root->next = placeholderNode;
		wordReturn = fgets(placeholderNode->word,30,fp);
	}
	//Loop through rest of file
	while(wordReturn!=NULL){
		removeCarriage(placeholderNode->word);
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
	return true;
}

//Runs the game
void gameLoop()
{
	do
	{
		displayWorld();
		acceptInput();
	}while(!isDone());
}

//Will print the puzzle (once program is done)
void displayWorld()
{
	printf("-----------------------\n");
}

//Finds the initial input from the user
void acceptInput()
{
	printf("Enter a guess: ");
	char str[30];
	fgets(str,30,stdin);
	removeCarriage(str);
	printf("%s\n",str);
}

//Ends the program
void tearDown()
{

	//TODO: printing gameListNode
	//printGameListNode();

	//TODO: printing wordListNode
	//printWordListNode();

	//Free the wordListNode linked list
	struct wordListNode* placeholderNode = NULL;
	while(root!=NULL){
		placeholderNode = root;
		root = root->next;
		free(placeholderNode);
	}

	//Free the gameListNode linked list
	struct gameListNode* newGameNode = NULL;
	while(gameRoot!=NULL){
		newGameNode = gameRoot;
		gameRoot = gameRoot->next;
		free(newGameNode);
	}

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
void findWords(char* masterWord, int size){
	//Setting up some gameListNodes
	gameRoot = malloc(sizeof(struct gameListNode));
	gameRoot->next = NULL;
	struct gameListNode* newGameNode;
	struct gameListNode* gameNode = malloc(sizeof(struct gameListNode));
	gameNode->next = NULL;

	//Sets placeholderNode to root
	struct wordListNode *rootRepresentation = NULL;
	if(root != NULL)
		rootRepresentation = root;

	bool firstWordFound = false;

	//int count;

	//Iterating through wordListNode
	while(rootRepresentation != NULL) {
		if(compareCounts(masterWord, rootRepresentation->word)){
			//Different process for the first time (gameRoot) and the second time through
			//so the second iteration won't damage the root
			if(!firstWordFound){
				strcpy(gameRoot->gameWord, rootRepresentation->word);
				gameRoot->found = false;
				newGameNode = gameRoot;
				gameRoot->next = gameNode;
				firstWordFound = true;
			}
			else{
				strcpy(gameNode->gameWord, rootRepresentation->word);
				newGameNode->found = false;
				newGameNode->next = gameNode;
				newGameNode = newGameNode->next;
				gameNode = malloc(sizeof(struct gameListNode));
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
//TODO: broken?
bool compareCounts(char* inputWord, char* candidate)
{
	int inputArray[26] = {0};
	int candidateArray[26] = {0};
	getLetterDistribution(inputArray, inputWord);
	getLetterDistribution(candidateArray, candidate);
	/*TODO: test printing these
	for(int i = 0; i < 26; i++){
		printf("%d ", inputArray[i]);
	}
	printf("\n");
	for(int i = 0; i < 26; i++){
		printf("%d ", candidateArray[i]);
	}
	printf("\n");
	*/
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
char *removeCarriage(char* str){
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

//Prints the contents of the gameListNode
void printGameListNode(){
	struct gameListNode* newGameNode = gameRoot;
	while(newGameNode!=NULL){
		printf("%s\n",newGameNode->gameWord);
		newGameNode = newGameNode->next;
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
