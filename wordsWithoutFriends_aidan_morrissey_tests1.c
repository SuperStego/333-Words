#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
int *getLetterDistribution(int*,char*);
bool compareCounts(char*,char*);
char *removeLetter(char*,int);
int stringLen(char*);
int main(int argc, char **argv)
{
	initialization();
	gameLoop();
	tearDown();
	
	//Tests
	//getLetterDistribution() Test
	/*int dist[26];
	char* test1 = "FWOGGYY";
	TestDist(test1,dist);
	getLetterDistribution(dist, test1);
	TestDist(test1,dist);*/
	
	//compareCounts() test
	/*char* testWord = "CATANOAR";
	char* testCandidate = "ZZZZ";
	if(!compareCounts(testWord,testCandidate))
		printf("Test passed\n");*/

	return 0;
}
//Sets up the game
int initialization()
{
	srand(time(NULL));
	return 0;
}
//Returns if the program is done
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
	}while(!isDone);
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
	char str[20];
	fgets(str,20,stdin);
	for(int i = 0; str[i] != '\0'; i++)
	{
		//Remove carriage returns and line feeds so just the letters will be printed
		while(str[i] == '\r' || str[i] == '\n')
			for(int l = i; l < 20 && str[l] != '\0'; l++)
				str[l] = str[l+1];
		str[i] = toupper(str[i]);
	}
	printf("%s\n",str);
}
//Ends the program
void tearDown()
{
	printf("All Done\n");
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
	char* newWord = inputWord;
	for(int i = 0; candidate[i] != '\0'; i++)
	{
		bool found = false;
		//Loop will end if found is ever set to true since the desired letter has been found
		for(int j = 0; found == false && newWord[j] != '\0'; j++)
		{
			if(candidate[i] == newWord[j])
			{
				//Sets to lowercase letter since it's not a capital letter, but if set to '\0' 
				//the program would end when this spot in the array is iterated over again
				//TODO: Problem child
				//inputWord[j] = 'a';
				removeLetter(newWord, j);
				found = true;
			}
		}
		//If one letter isn't there the function doesn't need to go any further
		if(found == false)
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
char *removeLetter(char* str, int index)
{
	char dest[stringLen(str)-1];
	for(int i = 0; i<index;i++)
		dest[i] = str[i];
	for(int i = index; str[i+1]!= '\0'; i++)
		dest[i] = str[i+1];
	return dest;
}
int stringLen(char* str)
{
	int i = 0;
	for(int j = 0; str[j]!='\0'; j++)
		i++;
	return i;
}
