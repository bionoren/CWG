// This is a very simple word-search and word-hash-function program.
// In addition to its primary functions, the program writes a "Numbered-Word-List.txt" file, for inspection.
// This incremental-hash-function for 178,691 words fits in less than 600K, so it's very small.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "CWGLib.h"
#include "assert.h"
#include "arraydawg.h"

#define GRAPH_DATA "CWG_Data_For_Word-List.dat"
#define OUT_LIST "Numbered-Word-List.txt"

#define INPUT_BUFFER_SIZE 100
#define ESCAPE_SEQUENCE "999"
#define TWO_UP_EIGHT 256

// Tests the capitalized user input string for valid length and valid characters.
// Returns "true" for valid, and "false" for invalid.
bool ValidateInput(char *CappedInput) {
    size_t Length = strlen(CappedInput);
    if(Length > WORD_LENGTH) {
        return false;
    }
    for(int X = 0; X < Length; X++) {
        if ( CappedInput[X] < 'a' || CappedInput[X] > 'z' ) {
            return false;
        }
    }
    return true;
}

// The CWG basic-type arrays will have global scope to reduce function-argument overhead.
int *TheNodeArray;
int *TheListFormatArray;
int *TheRoot_WTEOBL_Array;
short *TheShort_WTEOBL_Array;
unsigned char *TheUnsignedChar_WTEOBL_Array;

// These two values are needed for the CWG Hash-Function.
int WTEOBL_Transition;

// Use the first two CWG arrays to return a boolean value indicating if "TheCandidate" word is in the lexicon.
bool SingleWordSearchboolean(char *TheCandidate, size_t CandidateLength) {
    int CurrentNodeIndex = TheCandidate[0] - 'a' + 1;
    for(int i = 1; i < CandidateLength; i++) {
        if(!(TheNodeArray[CurrentNodeIndex] & CHILD_MASK)) {
            return false;
        }

        bool extendedList = TheNodeArray[CurrentNodeIndex] & EXTENDED_LIST_FLAG;
        int CurrentChildListFormat = TheListFormatArray[(TheNodeArray[CurrentNodeIndex] & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
        CurrentChildListFormat += extendedList << (CurrentChildListFormat >> NUMBER_OF_ENGLISH_LETTERS);

        int CurrentLetterPosition = TheCandidate[i] - 'a';
        if(!(CurrentChildListFormat & PowersOfTwo[CurrentLetterPosition])) {
            return false;
        } else {
            CurrentNodeIndex = (TheNodeArray[CurrentNodeIndex] & CHILD_MASK) + ListFormatPopCount(CurrentChildListFormat, CurrentLetterPosition) - 1;
        }
    }
    return TheNodeArray[CurrentNodeIndex] & EOW_FLAG;
}

// Using a novel graph mark-up scheme, this function returns the hash index of "TheCandidate", and "0" if it does not exist.
// This function uses the additional 3 WTEOBL arrays.
int SingleWordHashFunction(char *TheCandidate, size_t CandidateLength) {
    int CurrentNodeIndex = TheCandidate[0] - 'a' + 1;
    int CurrentHashMarker = TheRoot_WTEOBL_Array[CurrentNodeIndex];
    for(int i = 1; i < CandidateLength; i++) {
        if(!(TheNodeArray[CurrentNodeIndex] & CHILD_MASK)) {
            return 0;
        }

        bool extendedList = TheNodeArray[CurrentNodeIndex] & EXTENDED_LIST_FLAG;
        int CurrentChildListFormat = TheListFormatArray[(TheNodeArray[CurrentNodeIndex] & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
        CurrentChildListFormat += extendedList << (CurrentChildListFormat >> NUMBER_OF_ENGLISH_LETTERS);

        int CurrentLetterPosition = TheCandidate[i] - 'a';
        if(!(CurrentChildListFormat & PowersOfTwo[CurrentLetterPosition])) {
            return 0;
        } else {
            CurrentNodeIndex = TheNodeArray[CurrentNodeIndex] & CHILD_MASK;
            // Use "TheShort_WTEOBL_Array".
            int popCount = ListFormatPopCount(CurrentChildListFormat, CurrentLetterPosition) - 1;
            if(CurrentNodeIndex < WTEOBL_Transition) {
                CurrentHashMarker -= TheShort_WTEOBL_Array[CurrentNodeIndex];
                CurrentNodeIndex += popCount;
                CurrentHashMarker += TheShort_WTEOBL_Array[CurrentNodeIndex];
            } else { // Use "TheUnsignedChar_WTEOBL_Array".
                CurrentHashMarker -= TheUnsignedChar_WTEOBL_Array[CurrentNodeIndex - WTEOBL_Transition];
                CurrentNodeIndex += popCount;
                CurrentHashMarker += TheUnsignedChar_WTEOBL_Array[CurrentNodeIndex - WTEOBL_Transition];
            }
            if(TheNodeArray[CurrentNodeIndex] & EOW_FLAG) {
                CurrentHashMarker--;
            }
        }
    }
    if(TheNodeArray[CurrentNodeIndex] & EOW_FLAG) {
        return TheRoot_WTEOBL_Array[1] - CurrentHashMarker;
    }
    return 0;
}

// List output variables.
FILE *WordDump;
int LastPosition = 0;

// A recursive function to print the lexicon contained in the CWG to a text file.
// The function also tests the hash-tracking logic.  If the words are output using successive numbers, the graph works perfectly.
void Print_CWG_Word_ListRecurse(int ThisIndex, int FillThisPlace, char ThisLetter, char *WorkingWord, int CurrentHashMarker) {
    int node = TheNodeArray[ThisIndex];
    int TheChildIndex = node & CHILD_MASK;

    WorkingWord[FillThisPlace] = ThisLetter;

    if(node & EOW_FLAG) {
        WorkingWord[FillThisPlace + 1] = '\0';
        CurrentHashMarker--;
        int HashCheck = (TheRoot_WTEOBL_Array[1] - CurrentHashMarker);
        //fprintf(WordDump, "%d [color=blue, style=bold];\n", ThisIndex);
        //fprintf(WordDump, "[%6d]-|%15s| - %6d (%d)\n", HashCheck, WorkingWord, (shortList?TheShort_WTEOBL_Array[TheChildIndex]:TheUnsignedChar_WTEOBL_Array[TheChildIndex - WTEOBL_Transition]), shortList);
        assert(HashCheck == ++LastPosition);
        assert(HashCheck == SingleWordHashFunction(WorkingWord, strlen(WorkingWord)));
        assert(SingleWordSearchboolean(WorkingWord, strlen(WorkingWord)));
    }
    if(TheChildIndex) {
        bool extendedList = node & EXTENDED_LIST_FLAG;
        int TheChildListFormat = TheListFormatArray[(node & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
        TheChildListFormat += extendedList << (TheChildListFormat >> NUMBER_OF_ENGLISH_LETTERS);

        const bool shortList = TheChildIndex < WTEOBL_Transition;
        int hashconst = CurrentHashMarker - (shortList?TheShort_WTEOBL_Array[TheChildIndex]:TheUnsignedChar_WTEOBL_Array[TheChildIndex - WTEOBL_Transition]);
        FillThisPlace++;
        for(char i = 0; i < NUMBER_OF_ENGLISH_LETTERS; i++) {
            if(TheChildListFormat & PowersOfTwo[i]) {
                //fprintf(WordDump, "%d -> %d [label=\"%c\"];\n", ThisIndex, TheChildIndex, i + 'a');
                Print_CWG_Word_ListRecurse(TheChildIndex, FillThisPlace, i + 'a', WorkingWord, hashconst + (shortList?TheShort_WTEOBL_Array[TheChildIndex]:TheUnsignedChar_WTEOBL_Array[TheChildIndex - WTEOBL_Transition]));
                TheChildIndex++;
            }
        }
    }
}

// This function will print the word list to a numbered text file.
void Print_CWG_Word_List() {
    char MessWithMe[WORD_LENGTH + 1];
    WordDump = fopen(OUT_LIST, "w");
    //fprintf(WordDump, "digraph CWG {\n");
    for(char i = 0; i < NUMBER_OF_ENGLISH_LETTERS; i++) {
        //fprintf(WordDump, "%d [color=red, style=bold, label=\"%c(%d)\"];\n", i, i + '`', i);
        Print_CWG_Word_ListRecurse(i + 1, 0, i + 'a', MessWithMe, TheRoot_WTEOBL_Array[i + 1]);
    }

    //fprintf(WordDump, "}");
    fclose(WordDump);
}

int debug() {
    // Array size variables.
    int NodeArraySize;
    int ListFormatArraySize;
    int Root_WTEOBL_ArraySize;
    int Short_WTEOBL_ArraySize;
    int UnsignedChar_WTEOBL_ArraySize;

    // Read the CWG graph, from the "GRAPH_DATA" file, into the global arrays.
    FILE *Data = fopen(GRAPH_DATA, "rb");
    assert(Data);

    // Read the array sizes.
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
    WTEOBL_Transition = Short_WTEOBL_ArraySize;

    printf("The CWG data-structure is in memory and ready to use.\n\n");
    printf("CWG Header Values = |%7d |%7d |%5d |%3d |%5d |%7d |\n", TheRoot_WTEOBL_Array[1], NodeArraySize, ListFormatArraySize,
    Root_WTEOBL_ArraySize, Short_WTEOBL_ArraySize, UnsignedChar_WTEOBL_ArraySize);

    Print_CWG_Word_List();

    // Free the dynamically allocated memory.
    free(TheNodeArray);
    free(TheListFormatArray);
    free(TheRoot_WTEOBL_Array);
    free(TheShort_WTEOBL_Array);
    free(TheUnsignedChar_WTEOBL_Array);

    return 0;
}
