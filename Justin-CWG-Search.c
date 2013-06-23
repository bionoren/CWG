// This is a very simple word-search and word-hash-function program.
// In addition to its primary functions, the program writes a "Numbered-Word-List.txt" file, for inspection.
// This incremental-hash-function for 178,691 words fits in less than 600K, so it's very small.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GRAPH_DATA "CWG_Data_For_Word-List.dat"
#define OUT_LIST "Numbered-Word-List.txt"

#define NUMBER_OF_ENGLISH_LETTERS 26
#define LOWER_IT 32
#define INPUT_BUFFER_SIZE 100
#define MAX 15
#define THREE 3
#define ESCAPE_SEQUENCE "999"
#define TWO_UP_EIGHT 256

#define EOW_FLAG 1073741824
#define LIST_FORMAT_INDEX_MASK 0X3FFE0000
#define LIST_FORMAT_BIT_SHIFT 17
#define CHILD_MASK 0X0001FFFF

// Define a Boolean, enumerated data type.
typedef enum { FALSE = 0, TRUE = 1 } Bool;

const int PowersOfTwo[NUMBER_OF_ENGLISH_LETTERS] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384,
 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216, 33554432 };
// When using the "ChildListMasks", the letter being investigated is known to exist, so its bit will not be included in the claculated offset.
const int ChildListMasks[NUMBER_OF_ENGLISH_LETTERS] = { 0X0, 0X1, 0X3, 0X7, 0XF, 0X1F, 0X3F, 0X7F, 0XFF, 0X1FF, 0X3FF, 0X7FF, 0XFFF,
 0X1FFF, 0X3FFF, 0X7FFF, 0XFFFF, 0X1FFFF, 0X3FFFF, 0X7FFFF, 0XFFFFF, 0X1FFFFF, 0X3FFFFF, 0X7FFFFF, 0XFFFFFF, 0X1FFFFFF };
const char PopCountTable[TWO_UP_EIGHT] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4,
 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3,
 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5,
 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3,
 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5,
 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6,
 6, 7, 6, 7, 7, 8 };

// Using an explicit jump table, this is an optimized 4-byte integer pop-count.
// The function returns the offset of the "LetterPosition"th letter from the "FirstChild" node in a list.
// If "LetterPosition" holds the first letter, then "0" is returned.
int ListFormatPopCount(int CompleteChildList, int LetterPosition){
	const static void *PositionJumpTable[NUMBER_OF_ENGLISH_LETTERS] = { &&Ze, &&On, &&On, &&On, &&On, &&On, &&On, &&On, &&Tw, &&Tw,
	&&Tw, &&Tw, &&Tw, &&Tw, &&Tw, &&Tw, &&Th, &&Th, &&Th, &&Th, &&Th, &&Th, &&Th, &&Th, &&Fo, &&Fo };
	int Result = 0;
	// By casting the internal integer variable "CompleteChildList" as an "unsigned char *" we can access each byte within it.
	// Remember that computer-programs in C use "little-endian" byte-order.
	unsigned char *ByteZero = (unsigned char *)&CompleteChildList;
	
	// Mask "CompleteChildList" so that
	CompleteChildList &= ChildListMasks[LetterPosition];
	goto *PositionJumpTable[LetterPosition];
	Fo:
		Result += PopCountTable[*(ByteZero + 3)];
	Th:
		Result += PopCountTable[*(ByteZero + 2)];
	Tw:
		Result += PopCountTable[*(ByteZero + 1)];
	On:
		Result += PopCountTable[*ByteZero];
	Ze:
	return Result;
}


// This simple function clips off the extra chars for each "fgets()" line.  Works for Linux and Windows text format.
void CutOffExtraChars(char *ThisLine){
	if ( ThisLine[strlen(ThisLine) - 2] == '\r' ) ThisLine[strlen(ThisLine) - 2] = '\0';
	else if ( ThisLine[strlen(ThisLine) - 1] == '\n' ) ThisLine[strlen(ThisLine) - 1] = '\0';
}

// This Function converts any lower case letters inside "RawWord" to capitals, so that the whole string is made of capital letters.
void MakeMeAllCapital(char *RawWord){
	int X;
	int Length = strlen(RawWord);
	for ( X = 0; X < Length; X++ ) {
		if ( RawWord[X] >= 'a' && RawWord[X] <= 'z' ) RawWord[X] = RawWord[X] - LOWER_IT;
	}
}

// Tests the capitalized user input string for valid length and valid characters.
// Returns "TRUE" for valid, and "FALSE" for invalid.
Bool ValidateInput(char *CappedInput){
	int X;
	int Length = strlen(CappedInput);
	if ( Length > MAX ) return FALSE;
	for ( X = 0; X < Length; X++ ) {
		if ( CappedInput[X] < 'A' || CappedInput[X] > 'Z' ) return FALSE;
	}
	return TRUE;
}

// The CWG basic-type arrays will have global scope to reduce function-argument overhead.
int *TheNodeArray;
int *TheListFormatArray;
int *TheRoot_WTEOBL_Array;
short *TheShort_WTEOBL_Array;
unsigned char *TheUnsignedChar_WTEOBL_Array;

// These two values are needed for the CWG Hash-Function.
int WTEOBL_Transition;
int IndexCorrection;

// Use the first two CWG arrays to return a Boolean value indicating if "TheCandidate" word is in the lexicon.
Bool SingleWordSearchBoolean(char *TheCandidate, int CandidateLength){
	int X;
	int CurrentLetterPosition = TheCandidate[0] - 'A';
	int CurrentNodeIndex = CurrentLetterPosition + 1;
	int CurrentChildListFormat;
	for ( X = 1; X < CandidateLength; X++ ) {
		if ( !(TheNodeArray[CurrentNodeIndex] & CHILD_MASK) ) return FALSE;
		CurrentChildListFormat = TheListFormatArray[(TheNodeArray[CurrentNodeIndex] & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
		CurrentLetterPosition = TheCandidate[X] - 'A';
		if ( !(CurrentChildListFormat & PowersOfTwo[CurrentLetterPosition]) ) return FALSE;
		else CurrentNodeIndex = (TheNodeArray[CurrentNodeIndex] & CHILD_MASK) + ListFormatPopCount(CurrentChildListFormat, CurrentLetterPosition);
	}
	if ( TheNodeArray[CurrentNodeIndex] & EOW_FLAG ) return TRUE;
	return FALSE;
}

// Using a novel graph mark-up scheme, this function returns the hash index of "TheCandidate", and "0" if it does not exist.
// This function uses the additional 3 WTEOBL arrays.
int SingleWordHashFunction(char *TheCandidate, int CandidateLength){
	int X;
	int CurrentLetterPosition = TheCandidate[0] - 'A';
	int CurrentNodeIndex = CurrentLetterPosition + 1;
	int CurrentChildListFormat;
	int CurrentHashMarker = TheRoot_WTEOBL_Array[CurrentNodeIndex];
	for ( X = 1; X < CandidateLength; X++ ) {
		if ( !(TheNodeArray[CurrentNodeIndex] & CHILD_MASK) ) return 0;
		CurrentChildListFormat = TheListFormatArray[(TheNodeArray[CurrentNodeIndex] & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
		CurrentLetterPosition = TheCandidate[X] - 'A';
		if ( !(CurrentChildListFormat & PowersOfTwo[CurrentLetterPosition]) ) return 0;
		else {
			CurrentNodeIndex = TheNodeArray[CurrentNodeIndex] & CHILD_MASK;
			// Use "TheShort_WTEOBL_Array".
			if ( CurrentNodeIndex < WTEOBL_Transition ) {
				CurrentHashMarker -= TheShort_WTEOBL_Array[CurrentNodeIndex];
				CurrentNodeIndex += ListFormatPopCount(CurrentChildListFormat, CurrentLetterPosition);
				CurrentHashMarker += TheShort_WTEOBL_Array[CurrentNodeIndex];
			}
			// Use "TheUnsignedChar_WTEOBL_Array".
			else {
				CurrentHashMarker -= TheUnsignedChar_WTEOBL_Array[CurrentNodeIndex - WTEOBL_Transition];
				CurrentNodeIndex += ListFormatPopCount(CurrentChildListFormat, CurrentLetterPosition);
				CurrentHashMarker += TheUnsignedChar_WTEOBL_Array[CurrentNodeIndex - WTEOBL_Transition];
			}
			if ( TheNodeArray[CurrentNodeIndex] & EOW_FLAG ) CurrentHashMarker -= 1;
		}
	}
	if ( TheNodeArray[CurrentNodeIndex] & EOW_FLAG ) return IndexCorrection - CurrentHashMarker;
	return 0;
}

// List output variables.
FILE *WordDump;
int LastPosition = 0;

// A recursive function to print the lexicon contained in the CWG to a text file.
// The function also tests the hash-tracking logic.  If the words are output using successive numbers, the graph works perfectly.
void Print_CWG_Word_ListRecurse(int ThisIndex, int FillThisPlace, char ThisLetter, char *WorkingWord, int CurrentHashMarker){
	int X;
	int TheChildIndex = TheNodeArray[ThisIndex] & CHILD_MASK;
	int TheChildListFormat;
	int ConstHashChange;
	int HashCheck;
	
	WorkingWord[FillThisPlace] = ThisLetter;
	if ( TheNodeArray[ThisIndex] & EOW_FLAG ) {
		WorkingWord[FillThisPlace + 1] = '\0';
		CurrentHashMarker -= 1;
		fprintf(WordDump, "[%d]-|%s|\r\n", HashCheck = (IndexCorrection - CurrentHashMarker), WorkingWord);
		if ( HashCheck != (LastPosition + 1) ) printf("Major Mistake|%d|!\n", HashCheck);
		LastPosition = HashCheck;
	}
	if ( TheChildIndex ) {
		TheChildListFormat = TheListFormatArray[(TheNodeArray[ThisIndex] & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
		if ( TheChildIndex < WTEOBL_Transition ) {
			ConstHashChange = TheShort_WTEOBL_Array[TheChildIndex];
			for ( X = 0; X < NUMBER_OF_ENGLISH_LETTERS; X++ ) {
				if ( TheChildListFormat & PowersOfTwo[X] ) {
					Print_CWG_Word_ListRecurse(TheChildIndex, FillThisPlace + 1, X + 'A', WorkingWord,
					CurrentHashMarker - ConstHashChange + TheShort_WTEOBL_Array[TheChildIndex]);
					TheChildIndex += 1;
				}
			}
		}
		else {
			ConstHashChange = TheUnsignedChar_WTEOBL_Array[TheChildIndex - WTEOBL_Transition];
			for ( X = 0; X < NUMBER_OF_ENGLISH_LETTERS; X++ ) {
				if ( TheChildListFormat & PowersOfTwo[X] ) {
					Print_CWG_Word_ListRecurse(TheChildIndex, FillThisPlace + 1, X + 'A', WorkingWord,
					CurrentHashMarker - ConstHashChange + TheUnsignedChar_WTEOBL_Array[TheChildIndex - WTEOBL_Transition]);
					TheChildIndex += 1;
				}
			}
		}
	}
}

// This function will print the word list to a numbered text file.
void Print_CWG_Word_List(void){
	int X;
	char MessWithMe[MAX + 1];
	WordDump = fopen(OUT_LIST, "w");
	for ( X = 1; X <= NUMBER_OF_ENGLISH_LETTERS; X ++ ) {
		Print_CWG_Word_ListRecurse(X, 0, X + '@', MessWithMe, TheRoot_WTEOBL_Array[X]);
	}
	fclose(WordDump);
}

int main(){
	int X;
	int HashReturnValue;
	
	// Array size variables.
	int TotalNumberOfWords;
	int NodeArraySize;
	int ListFormatArraySize;
	int Root_WTEOBL_ArraySize;
	int Short_WTEOBL_ArraySize;
	int UnsignedChar_WTEOBL_ArraySize;
	
	char RawUserInput[INPUT_BUFFER_SIZE];
	
	// Read the CWG graph, from the "GRAPH_DATA" file, into the global arrays.
	FILE *Data = fopen(GRAPH_DATA, "rb");
	
	// Read the array sizes.
	fread(&TotalNumberOfWords, sizeof(int), 1, Data);
	fread(&NodeArraySize, sizeof(int), 1, Data);
	fread(&ListFormatArraySize, sizeof(int), 1, Data);
	fread(&Root_WTEOBL_ArraySize, sizeof(int), 1, Data);
	fread(&Short_WTEOBL_ArraySize, sizeof(int), 1, Data);
	fread(&UnsignedChar_WTEOBL_ArraySize, sizeof(int), 1, Data);
	
	// Allocate memory to hold the arrays.
	TheNodeArray = (int *)malloc(NodeArraySize*sizeof(int));
	TheListFormatArray = (int *)malloc(ListFormatArraySize*sizeof(int));
	TheRoot_WTEOBL_Array = (int *)malloc(Root_WTEOBL_ArraySize*sizeof(int));
	TheShort_WTEOBL_Array = (short int *)malloc(Short_WTEOBL_ArraySize*sizeof(short int));
	TheUnsignedChar_WTEOBL_Array = (unsigned char *)malloc(UnsignedChar_WTEOBL_ArraySize*sizeof(unsigned char));
	
	// Read the 5 arrays into memory.
	fread(TheNodeArray, sizeof(int), NodeArraySize, Data);
	fread(TheListFormatArray, sizeof(int), ListFormatArraySize, Data);
	fread(TheRoot_WTEOBL_Array, sizeof(int), Root_WTEOBL_ArraySize, Data);
	fread(TheShort_WTEOBL_Array, sizeof(short int), Short_WTEOBL_ArraySize, Data);
	fread(TheUnsignedChar_WTEOBL_Array, sizeof(unsigned char), UnsignedChar_WTEOBL_ArraySize, Data);
	
	fclose(Data);
	
	// Make the proper assignments and adjustments to use the CWG.
	IndexCorrection = TotalNumberOfWords;
	WTEOBL_Transition = Short_WTEOBL_ArraySize;
	
	printf("The CWG data-structure is in memory and ready to use.\n\n");
	printf("CWG Header Values = |%7d |%7d |%5d |%3d |%5d |%7d |\n", TotalNumberOfWords, NodeArraySize, ListFormatArraySize,
	Root_WTEOBL_ArraySize, Short_WTEOBL_ArraySize, UnsignedChar_WTEOBL_ArraySize);
	
	Print_CWG_Word_List();
	
	// Run the demonstration program loop.
	while ( TRUE ) {
		printf("\nEnter a valid word to search for in the CWG...  Enter \"999\" to exit.\nInput: ");
		fgets(RawUserInput, INPUT_BUFFER_SIZE, stdin);
		CutOffExtraChars(RawUserInput);
		if ( !strncmp(RawUserInput, ESCAPE_SEQUENCE, THREE) ) break;
		MakeMeAllCapital(RawUserInput);
		if ( !ValidateInput(RawUserInput) ) {
			printf("\nInvalid user input...  Try again.\n\n");
			continue;
		}
		if ( SingleWordSearchBoolean(RawUserInput, strlen(RawUserInput)) ) printf("\n|%s|=[FOUND]", RawUserInput);
		else printf("\n|%s|=[BOGUS]", RawUserInput);
		HashReturnValue = SingleWordHashFunction(RawUserInput, strlen(RawUserInput));
		printf(", Hash Value|%d|\n\n", HashReturnValue);
	}
	
	// Free the dynamically allocated memory.
	free(TheNodeArray);
	free(TheListFormatArray);
	free(TheRoot_WTEOBL_Array);
	free(TheShort_WTEOBL_Array);
	free(TheUnsignedChar_WTEOBL_Array);
	
	return 0;
}
