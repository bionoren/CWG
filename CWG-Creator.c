// This program will first compile a traditional DAWG encoding from the "Word-List.txt" file.
// Next, data files are written to assist in the "CWG" creation process.
// A "Caroline Word Graph" will then be created using the intermediate data files, and stored in "CWG_Data_For_Word-List.dat".
// There is a very good reason for why this program is 1800 lines long.  The CWG is a perfect and complete hash function for English-Language in TWL06.

// 1) "Word-List.txt" is a text file with the number of words written on the very first line, and 1 word per line after that.  The words are case-insensitive.
// 2) The "CWG" encoding is very sensitive to the size and content of "Word-List.txt", so only minor alterations are guaranteed to work without code analysis.

// Include the big-three header files.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "CWG-Creator.h"
#include "CWGLib.h"
#include "tnode.h"
#include "dawg.h"
#include "breadthqueue.h"
#include "arraydawg.h"
#include "assert.h"

// The complete "CWG" graph is stored here.
#define CWG_DATA "CWG_Data_For_Word-List.dat"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TraverseTheDawgArrayRecurse(int *TheDawg, int *ListFormats, int *OnIt, int CurrentIndex, char *TheWordSoFar, int FillThisPosition, char CurrentLetter, int *WordCounter, bool PrintMe) {
    assert(FillThisPosition <= MAX);
    int CurrentChild;
    int WhatsBelowMe = 0;
    TheWordSoFar[FillThisPosition] = CurrentLetter;
    if(TheDawg[CurrentIndex] & EOW_FLAG) {
        (*WordCounter)++;
        TheWordSoFar[FillThisPosition + 1] = '\0';
        if(PrintMe) {
            printf("#|%d| - |%s|\n", *WordCounter, TheWordSoFar);
        }
        WhatsBelowMe++;
    }
    if((CurrentChild = (TheDawg[CurrentIndex] & CHILD_MASK))) {
        int ChildListFormat = ListFormats[(TheDawg[CurrentIndex] & INTERNAL_LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
        for(char i = 0; i < NUMBER_OF_ENGLISH_LETTERS; i++) {
            // Verify if the i'th letter exists in the Child-List.
            if(ChildListFormat & PowersOfTwo[i]) {
                // Because the i'th letter exists, run "ListFormatPopCount", to extract the "CorrectOffset".
                int CorrectOffset = ListFormatPopCount(ChildListFormat, i) - 1;
                WhatsBelowMe += TraverseTheDawgArrayRecurse(TheDawg, ListFormats, OnIt, CurrentChild + CorrectOffset, TheWordSoFar, FillThisPosition + 1, i + 'a', WordCounter, PrintMe);
            }
        }
    }
    // Because CWG is a compressed graph, many values of the "OnIt" array will be updated multiple times with the same values.
    OnIt[CurrentIndex] = WhatsBelowMe;
    return WhatsBelowMe;
}

int TraverseTheDawgArrayRecurseFinal(int *TheDawg, int *ListFormats, int *OnIt, int CurrentIndex, char *TheWordSoFar, int FillThisPosition, char CurrentLetter, int *WordCounter, bool PrintMe) {
    assert(FillThisPosition <= MAX);
    int WhatsBelowMe = 0;
    TheWordSoFar[FillThisPosition] = CurrentLetter;
    if(TheDawg[CurrentIndex] & EOW_FLAG) {
        (*WordCounter)++;
        TheWordSoFar[FillThisPosition + 1] = '\0';
        if(PrintMe) {
            printf("#|%d| - |%s|\n", *WordCounter, TheWordSoFar);
        }
        WhatsBelowMe++;
    }
    int CurrentChild;
    if((CurrentChild = (TheDawg[CurrentIndex] & CHILD_MASK))) {
        CurrentChild--;
        int listIndex = (TheDawg[CurrentIndex] & INTERNAL_LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT;
        bool extendedList = listIndex & PowersOfTwo[12];
        listIndex -= extendedList * PowersOfTwo[12];
        int ChildListFormat = ListFormats[listIndex];
        ChildListFormat += extendedList << (ChildListFormat >> NUMBER_OF_ENGLISH_LETTERS);

        for(char i = 0; i < NUMBER_OF_ENGLISH_LETTERS; i++) {
            // Verify if the i'th letter exists in the Child-List.
            if(ChildListFormat & PowersOfTwo[i]) {
                // Because the i'th letter exists, run "ListFormatPopCount", to extract the "CorrectOffset".
                int CorrectOffset = ListFormatPopCount(ChildListFormat, i);
                WhatsBelowMe += TraverseTheDawgArrayRecurseFinal(TheDawg, ListFormats, OnIt, CurrentChild + CorrectOffset, TheWordSoFar, FillThisPosition + 1, i + 'a', WordCounter, PrintMe);
            }
        }
    }
    // Because CWG is a compressed graph, many values of the "OnIt" array will be updated multiple times with the same values.
    OnIt[CurrentIndex] = WhatsBelowMe;
    return WhatsBelowMe;
}

void TraverseTheDawgArray(int *TheDawg, int *TheListFormats, int *BelowingMe, bool PrintToScreen, bool final) {
    int TheCounter = 0;
    char RunningWord[MAX + 1];
    for(char i = 0; i < NUMBER_OF_ENGLISH_LETTERS; i++) {
        if(!final) {
            TraverseTheDawgArrayRecurse(TheDawg, TheListFormats, BelowingMe, i + 1, RunningWord, 0, 'a' + i, &TheCounter, PrintToScreen);
        } else {
            TraverseTheDawgArrayRecurseFinal(TheDawg, TheListFormats, BelowingMe, i + 1, RunningWord, 0, 'a' + i, &TheCounter, PrintToScreen);
        }
    }
}

int createDataStructure(const WordInfo *info) {
    printf("\n  The 28-Step CWG-Creation-Process has commenced: (Hang in there, it will be over soon.)\n");
    // All of the words of similar length will be stored sequentially in the same array so that there will be (MAX + 1) arrays in total.
    char *AllWordsInEnglish[MAX + 1] = {NULL};
    int DictionarySizeIndex[MAX + 1] = {0};

    for(int i = 0; i < info->numWords; i++) {
        if(info->lengths[i] != 0) {
            DictionarySizeIndex[info->lengths[i]]++;
        }
    }
    // Allocate enough space to hold all of the words in strings so that we can add them to the trie by length.
    // The Smallest length of a string is assumed to be 2.
    for(int i = 2; i < (MAX + 1); i++) {
        AllWordsInEnglish[i] = (char*)calloc((i + 1) * DictionarySizeIndex[i], sizeof(char));
    }
    printf("\n  Word-List.txt is now in RAM.\n");

    int CurrentTracker[MAX + 1] = {0};
    // Copy all of the strings into the halfway house 1.
    int numWords = info->numWords;
    char *word;
    for(int i = 0; i < info->numWords; i++) {
        int CurrentLength = info->lengths[i];
        // Simply copy a string from its temporary ram location to the array of length equivelant strings for processing in making the DAWG.
        if(CurrentLength != 0) {// && ((CurrentLength > 8 && CurrentTracker[CurrentLength] == 0) || info->words[i * MAX] != word[0])) {
            word = &(info->words[i * MAX]);
            //printf("dict contains %.*s\n", CurrentLength, word);
            char *temp = strncpy(&(AllWordsInEnglish[CurrentLength][CurrentTracker[CurrentLength] * (CurrentLength + 1)]), word, CurrentLength);
            if(CurrentLength != 15) {
                assert(strcmp(temp, word) == 0);
            }
            CurrentTracker[CurrentLength]++;
        } else {
            numWords--;
        }
    }
    printf("\n  The words are now stored in an array according to length.\n\n");
    // Make sure that the counting has resulted in all of the strings being placed correctly.
    for(int i = 0; i < (MAX + 1); i++) {
        if(DictionarySizeIndex[i] == CurrentTracker[i]) {
            printf("  |%2d| Letter word count = |%5d| is verified.\n", i, CurrentTracker[i]);
        } else {
            printf("  Something went wrong with |%2d| letter words. (%d != %d)\n", i, DictionarySizeIndex[i], CurrentTracker[i]);
            //assert(false);
        }
    }

    printf("\n  Begin Creator init function.\n\n");

    ArrayDawgInit(AllWordsInEnglish, DictionarySizeIndex, MAX);

    //-----------------------------------------------------------------------------------
    // Begin tabulation of "NumberOfWordsToEndOfBranchList" array.
    FILE *PartOne = fopen(DIRECT_GRAPH_DATA_PART_ONE, "rb");
    FILE *PartTwo = fopen(DIRECT_GRAPH_DATA_PART_TWO, "rb");
    FILE *ListE = fopen(FINAL_NODES_DATA, "rb");
    int NumberOfPartOneNodes;
    int NumberOfPartTwoNodes;
    int NumberOfFinalNodes;
    int CurrentCount;

    fread(&NumberOfPartOneNodes, sizeof(int), 1, PartOne);
    fread(&NumberOfPartTwoNodes, sizeof(int), 1, PartTwo);
    fread(&NumberOfFinalNodes, sizeof(int), 1, ListE);
    int *PartOneArray = (int *)malloc((NumberOfPartOneNodes + 1)*sizeof(int));
    int *PartTwoArray = (int *)calloc(NumberOfPartTwoNodes, sizeof(int));
    int *FinalNodeLocations = (int *)malloc(NumberOfFinalNodes*sizeof(int));

    fread(PartOneArray + 1, sizeof(int), NumberOfPartOneNodes, PartOne);
    fread(PartTwoArray, sizeof(int), NumberOfPartTwoNodes, PartTwo);
    fread(FinalNodeLocations, sizeof(int), NumberOfFinalNodes, ListE);

    int *NumberOfWordsBelowMe = (int *)calloc((NumberOfPartOneNodes + 1), sizeof(int));
    int *NumberOfWordsToEndOfBranchList =(int *)calloc((NumberOfPartOneNodes + 1), sizeof(int));
    int *RearrangedNumberOfWordsToEndOfBranchList =(int *)calloc((NumberOfPartOneNodes + 1), sizeof(int));

    PartOneArray[0] = 0;

    fclose(PartOne);
    fclose(PartTwo);
    fclose(ListE);

    printf("\nStep 18 - Display the Mask-Format for CWG Main-Nodes:\n\n");

    char Something[38];
    ConvertIntNodeToBinaryString(CHILD_MASK, Something);
    printf("  %s - CHILD_MASK\n", Something);

    ConvertIntNodeToBinaryString(LIST_FORMAT_INDEX_MASK, Something);
    printf("  %s - INTERNAL_LIST_FORMAT_INDEX_MASK\n", Something);

    printf("\nStep 19 - Traverse the DawgArray to fill the NumberOfWordsBelowMe array.\n");

    // This function is run to fill the "NumberOfWordsBelowMe" array.
    TraverseTheDawgArray(PartOneArray, PartTwoArray, NumberOfWordsBelowMe, false, false);

    printf("\nStep 20 - Use FinalNodeLocations and NumberOfWordsBelowMe to fill the NumberOfWordsToEndOfBranchList array.\n");

    // This little piece of code compiles the "NumberOfWordsToEndOfBranchList" array.
    // The requirements are the "NumberOfWordsBelowMe" array and the "FinalNodeLocations" array.
    for(int i = 1, CurrentFinalNodeIndex = 0; i <= NumberOfPartOneNodes; i++ ) {
        CurrentCount = 0;
        for(int j = i; j <= FinalNodeLocations[CurrentFinalNodeIndex]; j++) {
            CurrentCount += NumberOfWordsBelowMe[j];
        }
        NumberOfWordsToEndOfBranchList[i] = CurrentCount;
        if(i == FinalNodeLocations[CurrentFinalNodeIndex]) {
            CurrentFinalNodeIndex++;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Now with preliminary analysis complete, it is time to rearrange the PartOne nodes and then set up PartThree.

    int ListSizeCounter[NUMBER_OF_ENGLISH_LETTERS + 1] = {0};
    int TotalNumberOfLists = 0;
    int StartOfCurrentList = 1;
    int SizeOfCurrentList = FinalNodeLocations[0];
    int EndOfCurrentList = FinalNodeLocations[0];
    int InsertionPoint = 1;
    int CurrentlyCopyingThisList = 0;
    int *PartOneRearrangedArray = (int *)malloc((NumberOfPartOneNodes + 1)*sizeof(int));
    int *CurrentAdjustments = (int *)calloc((NumberOfPartOneNodes + 1), sizeof(int));

    PartOneRearrangedArray[0] = 0;

    printf("\nStep 21 - Relocate all node-lists with WTEOBL values greater than 255, to the front of the Main CWG array.\n");

    printf("\n  All corresponding node data and End-Of-List data must also be shifted around.\n");

    // This code is responsible for rearranging the node lists inside of the CWG int array so the word-heavy lists filter to the front.
    for(int i = 1, CurrentFinalNodeIndex = 0; i <= NumberOfPartOneNodes; i++) {
        bool AreWeInBigList = false;
        if(NumberOfWordsToEndOfBranchList[i] > 255) {
            AreWeInBigList = true;
            assert(i <= NUMBER_OF_ENGLISH_LETTERS || NumberOfWordsToEndOfBranchList[i] < 65536);
        }
        if(i == EndOfCurrentList) {
            ListSizeCounter[SizeOfCurrentList]++;
            // We are now at the end of a big list that must to be moved up to the InsertionPoint.
            // This also implies moving everything between its current location and its new one.
            if(AreWeInBigList == true) {
                // First step is to copy the CurrentList into the new array at its correct position.
                for(int j = 0; j < SizeOfCurrentList; j++) {
                    PartOneRearrangedArray[InsertionPoint + j] = PartOneArray[StartOfCurrentList + j];
                    RearrangedNumberOfWordsToEndOfBranchList[InsertionPoint + j] = NumberOfWordsToEndOfBranchList[StartOfCurrentList + j];
                }
                // The following steps are required when we are actually moving the position of a list.  The first set of lists will bypass these steps.
                if(InsertionPoint != StartOfCurrentList) {
                    // Step 2 is to move all of the nodes between the original and final location, "SizeOfCurrentList" number of places back, starting from the end.
                    for(int j = EndOfCurrentList; j >= (InsertionPoint + SizeOfCurrentList); j--) {
                        PartOneArray[j] = PartOneArray[j - SizeOfCurrentList];
                        NumberOfWordsToEndOfBranchList[j] = NumberOfWordsToEndOfBranchList[j - SizeOfCurrentList];
                    }
                    // Step 3 is to copy the list we are moving up from the rearranged array back into the original.
                    for(int j = InsertionPoint; j < (InsertionPoint + SizeOfCurrentList); j++) {
                        PartOneArray[j] = PartOneRearrangedArray[j];
                        NumberOfWordsToEndOfBranchList[j] = RearrangedNumberOfWordsToEndOfBranchList[j];
                    }
                    // Step 4 is to fill the "CurrentAdjustments" array with the amount that each child references must be moved.
                    // The two arrays are identical now up to the new insertion point.
                    // At this stage, the "CurrentAdjustments" array is all zeros.
                    for(int j = 1; j <= NumberOfPartOneNodes; j++) {
                        int TheCurrentChild = (PartOneArray[j] & CHILD_MASK);
                        if((TheCurrentChild >= InsertionPoint) && (TheCurrentChild < StartOfCurrentList)) {
                            CurrentAdjustments[j] = SizeOfCurrentList;
                        }
                        if((TheCurrentChild >= StartOfCurrentList) && (TheCurrentChild <= EndOfCurrentList)) {
                            CurrentAdjustments[j] = InsertionPoint - StartOfCurrentList;
                        }
                    }
                    // Step 5 is to fix all of the child reference values in both of the arrays.
                    // Start with the rearranged array.
                    for(int j = 1; j < (InsertionPoint + SizeOfCurrentList); j++) {
                        if(CurrentAdjustments[j] != 0) {
                            PartOneRearrangedArray[j] += CurrentAdjustments[j];
                        }
                    }
                    // Finish with the original array.  Make sure to zero all the values after the adjustments have been made to get ready for the next round.
                    for(int j = 1; j <= NumberOfPartOneNodes; j++) {
                        if(CurrentAdjustments[j] != 0) {
                            PartOneArray[j] += CurrentAdjustments[j];
                            CurrentAdjustments[j] = 0;
                        }
                    }
                }
                // Step 7 is to set the new InsertionPoint and change the "FinalNodeLocations", so that they reflect the shift.
                InsertionPoint += SizeOfCurrentList;
                // Shift all of the end of lists between the "CurrentlyCopyingThisList" and "CurrentFinalNodeIndex".
                for(int j = CurrentFinalNodeIndex; j > CurrentlyCopyingThisList; j--) {
                    FinalNodeLocations[j] = FinalNodeLocations[j - 1] + SizeOfCurrentList;
                }
                FinalNodeLocations[CurrentlyCopyingThisList] = InsertionPoint - 1;
                CurrentlyCopyingThisList++;

            }
            // Even when we are not in a big list, we still need to update the current list parameters.
            CurrentFinalNodeIndex++;
            SizeOfCurrentList = FinalNodeLocations[CurrentFinalNodeIndex] - EndOfCurrentList;
            EndOfCurrentList = FinalNodeLocations[CurrentFinalNodeIndex];
            StartOfCurrentList = i + 1;
        }
    }

    printf("\n  Word-Heavy list-shifting is now complete.\n");

    // Step 8 is to copy all of the small lists from the original array to the rearranged array.  All of the references should be properly adjusted at this point.
    for(int i = InsertionPoint; i <= NumberOfPartOneNodes; i++) {
        PartOneRearrangedArray[i] = PartOneArray[i];
        RearrangedNumberOfWordsToEndOfBranchList[i] = NumberOfWordsToEndOfBranchList[i];
    }

    // Rearrangement of the DAWG lists to reduce size of the PartThree data file is complete, so check if the new and old lists are identical, because they should be.
    for(int i = 1; i <= NumberOfPartOneNodes; i++) {
        if(PartOneArray[i] != PartOneRearrangedArray[i]) {
            printf("  What A Mistake!\n");
        }
        if(RearrangedNumberOfWordsToEndOfBranchList[i] != NumberOfWordsToEndOfBranchList[i]) {
            printf("  Mistaken.\n");
        }
    }

    // The two arrays are now identical, so as a final precaution, traverse the rearranged array.
    TraverseTheDawgArray(PartOneRearrangedArray, PartTwoArray, NumberOfWordsBelowMe, false, false);

    // Check for duplicate lists.  It is now highly likely that there are some duplicates.
    // Lists of size X, can be replaced with partial lists of size X+n.  Make sure to check for this case.

    printf("\nStep 22 - Create an array to organize End-Of-List values by size.\n\n");

    // Add up the total number of lists.
    for(int i = 1; i <= NUMBER_OF_ENGLISH_LETTERS; i++) {
        TotalNumberOfLists += ListSizeCounter[i];
        printf("  Size|%2d| Lists = |%5d|\n", i, ListSizeCounter[i]);
    }
    printf("\n  TotalNumberOfLists = |%d|\n", TotalNumberOfLists);

    int **NodeListsBySize = (int**)malloc((NUMBER_OF_ENGLISH_LETTERS + 1) * sizeof(int*));
    int WhereWeAt[NUMBER_OF_ENGLISH_LETTERS + 1] = {0};

    for(int i = 1; i <= NUMBER_OF_ENGLISH_LETTERS; i++) {
        NodeListsBySize[i] = (int*)malloc(ListSizeCounter[i] * sizeof(int));
    }

    // We are now required to fill the "NodeListsBySize" array.  Simply copy over the correct "FinalNode" information.
    // Note that the "FinalNode" information reflects the readjustment that just took place.

    EndOfCurrentList = FinalNodeLocations[0];
    SizeOfCurrentList = FinalNodeLocations[0];
    for(int i = 0, CurrentFinalNodeIndex = 0; i < NumberOfFinalNodes; i++) {
        NodeListsBySize[SizeOfCurrentList][WhereWeAt[SizeOfCurrentList]] = EndOfCurrentList;
        WhereWeAt[SizeOfCurrentList]++;
        CurrentFinalNodeIndex++;
        SizeOfCurrentList = FinalNodeLocations[CurrentFinalNodeIndex] - EndOfCurrentList;
        EndOfCurrentList = FinalNodeLocations[CurrentFinalNodeIndex];
    }

    printf("\n  End-Of-List values are now organized.\n");

    int TotalNumberOfKilledLists = 0;
    int TotalNumberOfKilledNodes = 0;
    int CurrentNumberOfPartOneNodes = NumberOfPartOneNodes;
    bool EliminateCurrentList = true;

    printf("\nStep 23 - Kill more lists by using the ends of longer lists or lists of equal size.\n\n");

    // Try to eliminate lists with partial lists.
    // "i" is the list-length of lists we are trying to kill.
    for(int i = NUMBER_OF_ENGLISH_LETTERS; i >= 1; i--) {
        printf("  Try To Eliminate Lists of Size |%2d| - ", i);
        int NewNumberOfKilledLists = 0;
        // Look for partial lists at the end of "j" sized lists, to replace the "i" sized lists with.
        for(int j = NUMBER_OF_ENGLISH_LETTERS; j >= i; j--) {
            // Try to kill list # "Z".
            for(int k = 0; k < ListSizeCounter[i]; k++ ) {
                int InspectThisEndOfList = NodeListsBySize[i][k];
                // Try to replace with list # "k".
                for(int l = 0; l < ListSizeCounter[j]; l++) {
                    // Never try to replace a list with itself.
                    if(i == j && k == l) {
                        continue;
                    }
                    int MaybeReplaceWithThisEndOfList = (NodeListsBySize[j])[l];
                    for(int m = 0; m < i; m++) {
                        if(PartOneArray[InspectThisEndOfList - m] != PartOneArray[MaybeReplaceWithThisEndOfList - m]) {
                            EliminateCurrentList = false;
                            break;
                        }
                    }
                    // When eliminating a list, make sure to adjust the WTEOBL data.
                    if(EliminateCurrentList == true) {
                        // Step 1 - Replace all references to the duplicate list with the earlier equivalent.
                        for(int m = 1; m <= CurrentNumberOfPartOneNodes; m++) {
                            int TheCurrentChild = PartOneArray[m] & CHILD_MASK;
                            if(TheCurrentChild > InspectThisEndOfList - i && TheCurrentChild <= InspectThisEndOfList) {
                                int TheNewChild = MaybeReplaceWithThisEndOfList - (InspectThisEndOfList - TheCurrentChild);
                                PartOneArray[m] -= TheCurrentChild;
                                PartOneArray[m] += TheNewChild;
                            }
                        }
                        // Step 2 - Eliminate the dupe list by moving the higher lists forward.
                        for(int m = (InspectThisEndOfList - i + 1); m <= (CurrentNumberOfPartOneNodes - i); m++) {
                            PartOneArray[m] = PartOneArray[m + i];
                            NumberOfWordsToEndOfBranchList[m] = NumberOfWordsToEndOfBranchList[m + i];
                        }
                        // Step 3 - Change CurrentNumberOfPartOneNodes.
                        CurrentNumberOfPartOneNodes -= i;
                        // Step 4 - Lower all references to the nodes coming after the dupe list.
                        for(int m = 1; m <= CurrentNumberOfPartOneNodes; m++) {
                            int TheCurrentChild = PartOneArray[m] & CHILD_MASK;
                            if(TheCurrentChild > InspectThisEndOfList) {
                                PartOneArray[m] -= i;
                            }
                        }
                        // Step 5 - Readjust all of the lists after "k" forward 1 and down i to the value, and lower ListSizeCounter[i] by 1.
                        for(int m = k; m < ListSizeCounter[i] - 1; m++) {
                            NodeListsBySize[i][m] = NodeListsBySize[i][m + 1] - i;
                        }
                        ListSizeCounter[i]--;
                        // Step 6 - Lower any list, of any size, greater than (NodeListsBySize[i])[k], down by i.
                        for(int m = 1; m <= i - 1; m++) {
                            for(int n = 0; n < ListSizeCounter[m]; n++) {
                                if(NodeListsBySize[m][n] > InspectThisEndOfList) {
                                    NodeListsBySize[m][n] -= i;
                                }
                            }
                        }
                        for(int m = (i + 1); m <= NUMBER_OF_ENGLISH_LETTERS; m++) {
                            for(int n = 0; n < ListSizeCounter[m]; n++) {
                                if(NodeListsBySize[m][n] > InspectThisEndOfList) {
                                    NodeListsBySize[m][n] -= i;
                                }
                            }
                        }
                        // Step 7 - Lower "k" by 1 and increase "TotalNumberOfKilledLists".
                        k--;
                        TotalNumberOfKilledLists++;
                        NewNumberOfKilledLists++;
                        TotalNumberOfKilledNodes += i;
                        break;
                    }
                    EliminateCurrentList = true;
                }
            }
        }
        if(NewNumberOfKilledLists > 0) {
            printf("Killed |%d| lists.\n", NewNumberOfKilledLists);
        } else {
            printf("Empty handed.\n");
        }
    }

    printf("\n  Removal of the new-redundant-lists is now complete:\n");
    printf("\n  |%5d| = Original # of lists.\n", TotalNumberOfLists);
    printf("  |%5d| = Killed # of lists.\n", TotalNumberOfKilledLists);
    printf("  |%5d| = Remaining # of lists.\n", TotalNumberOfLists = TotalNumberOfLists - TotalNumberOfKilledLists);

    printf("\n  |%6d| = Original # of nodes.\n", NumberOfPartOneNodes);
    printf("  |%6d| = Killed # of nodes.\n", TotalNumberOfKilledNodes);
    printf("  |%6d| = Remaining # of nodes.\n", CurrentNumberOfPartOneNodes);

    // Try to eliminate lists with partial lists again to check that we've got em all.
    printf("\nStep 24 - Run the redundant-list-analysis one more time to test that no-more exist.\n\n");
//TODO: Needs to be refactored into a function

    printf("\n  The no-more redundant-list-test is now complete:\n");
    printf("\n  |%5d| = Original # of lists.\n", TotalNumberOfLists);
    printf("  |%5d| = Killed # of lists.\n", TotalNumberOfKilledLists);
    printf("  |%5d| = Remaining # of lists.\n", TotalNumberOfLists - TotalNumberOfKilledLists);

    printf("\n  |%6d| = Original # of nodes.\n", NumberOfPartOneNodes);
    printf("  |%6d| = Killed # of nodes.\n", TotalNumberOfKilledNodes);
    printf("  |%6d| = Remaining # of nodes.\n", CurrentNumberOfPartOneNodes);

    // verify that the reduction procedures have resulted in a valid word graph.

    // "FinalNodeLocations" needs to be recompiled from what is left in the "NodeListsBySize" arrays.

    printf("\nStep 25 - Recompile the FinalNodeLocations array and display the distribution.\n\n");

    TotalNumberOfLists = 0;
    for(int i = 1; i <= NUMBER_OF_ENGLISH_LETTERS; i++) {
        TotalNumberOfLists += ListSizeCounter[i];
        printf("  List Size|%2d| - Number Of Lists|%5d|\n", i, ListSizeCounter[i]);
    }
    printf("\n  TotalNumberOfLists|%d|\n", TotalNumberOfLists);

    // Set all initial values in "FinalNodeLocations" array to BOGUS numbers.
    for(int i = 0; i < TotalNumberOfLists; i++ ) {
        FinalNodeLocations[i] = 1000000;
    }

    // Filter all of the living "FinalNode" values into the "FinalNodeLocations" array.
    for(int i = NUMBER_OF_ENGLISH_LETTERS; i >= 1; i-- ) {
        for(int j = 0; j < ListSizeCounter[i]; j++) {
            FinalNodeLocations[TotalNumberOfLists - 1] = NodeListsBySize[i][j];
            // The new list has been placed at the end of the "FinalNodeLocations" array, now filter it up to where it should be.
            for(int k = (TotalNumberOfLists - 1); k > 0; k--) {
                if(FinalNodeLocations[k - 1] > FinalNodeLocations[k]) {
                    int TempValue = FinalNodeLocations[k - 1];
                    FinalNodeLocations[k - 1] = FinalNodeLocations[k];
                    FinalNodeLocations[k] = TempValue;
                } else {
                    break;
                }
            }
        }
    }


    // Test for logical errors in the list elimination procedure.
    for(int i = 0; i < (TotalNumberOfLists - 1); i++) {
        if(FinalNodeLocations[i] == FinalNodeLocations[i + 1]) {
            printf("\nNo Two Lists Can End On The Same Node. |%d|%d|\n", i, FinalNodeLocations[i]);
            assert(false);
        }
    }

    printf("\nCompacting child lists...\n");

    typedef struct indexedFlag {
        int index;
        bool flag;
    } indexedFlag;

    int indexMapLen = NumberOfPartTwoNodes;
    indexedFlag *indexMap = malloc(NumberOfPartTwoNodes * sizeof(indexedFlag));
    for(int i = 0; i < NumberOfPartTwoNodes; i++) {
        indexMap[i].index = i;
        indexMap[i].flag = false;
    }

    //remove duplicates
    int killedChildLists = 0;
    for(int i = 0; i < NumberOfPartTwoNodes; i++) {
        for(int j = i + 1; j < NumberOfPartTwoNodes; j++) {
            //if there's a duplicate...
            if(PartTwoArray[i] == PartTwoArray[j]) {
                //...remove the duplicate
                for(int k = 0; k < indexMapLen; k++) {
                    if(indexMap[k].index == j) {
                        indexMap[k].index = i;
                    } else if(indexMap[k].index > j) {
                        indexMap[k].index--;
                    }
                }
                //...compact the lists
                for(int k = j + 1; k < NumberOfPartTwoNodes; k++) {
                    PartTwoArray[k - 1] = PartTwoArray[k];
                }
                NumberOfPartTwoNodes--;
                killedChildLists++;
            }
        }
    }

    //"+1" compaction. Note that it *matters* what ordering you try compaction in
    for(int len = NUMBER_OF_ENGLISH_LETTERS - 1; len >= 0; len--) {
        for(int i = 0; i < NumberOfPartTwoNodes; i++) {
            for(int k = 0; k < indexMapLen; k++) {
                if(indexMap[k].index == i && indexMap[k].flag) {
                    goto COMPACT_FAIL;
                }
            }
            if(__builtin_popcount(PartTwoArray[i]) == len) {
                for(int j = 0; j < NumberOfPartTwoNodes; j++) {
                    //use extra 5 bits to compact "+1" lists
                    int xor = PartTwoArray[i] ^ PartTwoArray[j];
                    if(xor && !moreThanOneBitSet(xor) && !(xor & PartTwoArray[i])) {
                        assert(i != j);
                        for(int k = 0; k < indexMapLen; k++) {
                            if(indexMap[k].index == j && indexMap[k].flag) {
                                goto COMPACT_FAIL2;
                            }
                        }

                        //add in the "+1" letter
                        int bit = __builtin_ffs(xor) - 1; //-1 here lets us avoid the subtraction when we walk the graph later, but it makes this section harder to validate
                        //assert(bit && bit <= NUMBER_OF_ENGLISH_LETTERS);
                        PartTwoArray[i] |= bit << NUMBER_OF_ENGLISH_LETTERS;
                        //assert(PartTwoArray[i] >> NUMBER_OF_ENGLISH_LETTERS);

                        //compact the lists
                        for(int k = j + 1; k < NumberOfPartTwoNodes; k++) {
                            PartTwoArray[k - 1] = PartTwoArray[k];
                        }
                        i -= j < i; // if we just moved the list back on ourselves, back up

                        //fix the indexes
                        for(int k = 0; k < indexMapLen; k++) {
                            if(indexMap[k].index == j) {
                                assert(!indexMap[k].flag);
                                indexMap[k].index = i;
                                indexMap[k].flag = true;
                            } else if(indexMap[k].index > j) {
                                indexMap[k].index--;
                            }
                        }

                        NumberOfPartTwoNodes--;
                        killedChildLists++;
                        break;
                    }
                COMPACT_FAIL2:
                    ;
                }
            }
        COMPACT_FAIL:
            ;
        }
    }
    for(int i = 0; i < indexMapLen; i++) {
        //assert(!indexMap[i].flag || PartTwoArray[indexMap[i].index] >> NUMBER_OF_ENGLISH_LETTERS);
    }
    assert(NumberOfPartTwoNodes < PowersOfTwo[12]);

    //fix the node links
    for(int i = 1; i <= CurrentNumberOfPartOneNodes; i++) {
        int node = PartOneArray[i];
        int oldIndex = (node & INTERNAL_LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT;
        node -= node & INTERNAL_LIST_FORMAT_INDEX_MASK;
        int newIndex = indexMap[oldIndex].index | (indexMap[oldIndex].flag << 12);
        node += newIndex << LIST_FORMAT_BIT_SHIFT;
        PartOneArray[i] = node;
    }

    free(indexMap);
    printf("Killed %d child lists\n", killedChildLists);

    /**
     for putting nodes closer together, Drew recommends a breadth first sort with "force based" insertion.
     */

    printf("\n  The FinalNodeLocations array is now compiled and tested for obvious errors.\n");

    printf("\nStep 26 - Recompile WTEOBL array by graph traversal, and test equivalence with the one modified during list-killing.\n");

    // Compile "RearrangedNumberOfWordsToEndOfBranchList", and verify that it is the same as "NumberOfWordsToEndOfBranchList".
    TraverseTheDawgArray(PartOneArray, PartTwoArray, NumberOfWordsBelowMe, false, true);

    // This little piece of code compiles the "RearrangedNumberOfWordsToEndOfBranchList" array.
    // The requirements are the "NumberOfWordsBelowMe" array and the "FinalNodeLocations" array.
    for(int i = 1, CurrentFinalNodeIndex = 0; i <= CurrentNumberOfPartOneNodes; i++ ) {
        CurrentCount = 0;
        for(int j = i; j <= FinalNodeLocations[CurrentFinalNodeIndex]; j++) {
            CurrentCount += NumberOfWordsBelowMe[j];
        }
        RearrangedNumberOfWordsToEndOfBranchList[i] = CurrentCount;
        if(i == FinalNodeLocations[CurrentFinalNodeIndex]) {
            CurrentFinalNodeIndex++;
        }
    }

    printf("\n  New WTEOBL array is compiled, so test for equality.\n");

    for(int i = 1; i <= CurrentNumberOfPartOneNodes; i++) {
        assert(RearrangedNumberOfWordsToEndOfBranchList[i] == NumberOfWordsToEndOfBranchList[i]);
    }

    printf("\n  Equality test complete.\n");

    printf("\nStep 27 - Determine the final node index that requires a short integer for its WTEOBL value.\n");

    // Find out the final index number that requires an integer greater in size than a byte to hold it. Part 3 of the data structure will be held in three arrays.
    int FurthestBigNode = 0;
    for(int i = 1; i <= CurrentNumberOfPartOneNodes; i++) {
        if(RearrangedNumberOfWordsToEndOfBranchList[i] > 0XFF) {
            FurthestBigNode = i;
        }
    }

    for(int i = 0; i < TotalNumberOfLists; i++) {
        if(FinalNodeLocations[i] >= FurthestBigNode) {
            printf("\n  End of final short-integer WTEOBL list = |%d|.\n", FinalNodeLocations[i]);
            FurthestBigNode = FinalNodeLocations[i];
            break;
        }
    }

    int FirstSmallNode = FurthestBigNode + 1;

    printf("\n  Index of first node requiring only an unsigned-char for its WTEOBL = |%d|.\n", FirstSmallNode);

    // The first 26 nodes are the only ones in need of 4-Byte int variables to hold their WTEOBL values.
    // Being the entry points for the graph, it makes sense to hold these values in a "const int" array, defined in the program code.

    // The "short int" array holding the medium size WTEOBL values will hold '0's for elements [0, 26], inclusive.
    // The "unsigned char" array must be unsigned because many of the values require 8-bit representation.

    // The entire CWG will be stored inside of the one "CWG_Data_For_Word-List.dat" data file.
    // The first integer will be the total number of words in the graph.
    // The next five integers will be the array sizes.
    // After these header values, each array will then be written to the file in order, using the correct integer type.

    printf("\nStep 28 - Separate the final 3 WTEOBL arrays, and write all 5 arrays to the FinalProduct CWG data file.\n");

    int ArrayOneSize = CurrentNumberOfPartOneNodes + 1;
    int ArrayTwoSize = NumberOfPartTwoNodes;
    int ArrayThreeSize = NUMBER_OF_ENGLISH_LETTERS + 1;
    int ArrayFourSize = FurthestBigNode + 1;
    int ArrayFiveSize = CurrentNumberOfPartOneNodes - FurthestBigNode;

    // Allocate the final three arrays.
    int *PartThreeArray = (int*)malloc(ArrayThreeSize * sizeof(int));
    short int *PartFourArray = (short int*)malloc(ArrayFourSize * sizeof(short int));
    unsigned char *PartFiveArray = (unsigned char*)malloc(ArrayFiveSize * sizeof(unsigned char));

    // Fill the final three CWG arrays.
    for(int i = 0; i < ArrayThreeSize; i++) {
        PartThreeArray[i] = RearrangedNumberOfWordsToEndOfBranchList[i];
        PartFourArray[i] = 0;
    }
    for(int i = ArrayThreeSize; i < ArrayFourSize; i++) {
        PartFourArray[i] = (short)RearrangedNumberOfWordsToEndOfBranchList[i];
        //assert(PartFourArray[i] > 255);
    }
    for(int i = 0; i < ArrayFiveSize; i++) {
        PartFiveArray[i] = (char)RearrangedNumberOfWordsToEndOfBranchList[ArrayFourSize + i];
    }

    FILE* FinalProduct = fopen(CWG_DATA, "wb");
    char *tempDebugName = malloc((strlen(CWG_DATA) + 4) * sizeof(char));
    strcpy(tempDebugName, CWG_DATA);
    strcat(tempDebugName, ".txt");
    printf("name = %s\n", tempDebugName);
    FILE* FinalProductDebug = fopen(tempDebugName, "w");
    assert(FinalProductDebug);

    fprintf(FinalProductDebug, "numWords = %d\n", PartThreeArray[0]);
    fwrite(&ArrayOneSize, sizeof(int), 1, FinalProduct);
    fprintf(FinalProductDebug, "numNodes = %d\n", ArrayOneSize);
    fwrite(&ArrayTwoSize, sizeof(int), 1, FinalProduct);
    fprintf(FinalProductDebug, "numListFormats = %d\n", ArrayTwoSize);
    fwrite(&ArrayThreeSize, sizeof(int), 1, FinalProduct);
    fprintf(FinalProductDebug, "numRootWTEOBLs = %d\n", ArrayThreeSize);
    fwrite(&ArrayFourSize, sizeof(int), 1, FinalProduct);
    fprintf(FinalProductDebug, "numShortWTEOBLs = %d\n", ArrayFourSize);
    fwrite(&ArrayFiveSize, sizeof(int), 1, FinalProduct);
    fprintf(FinalProductDebug, "numCharWTEOBLs = %d\n", ArrayFiveSize);

    fwrite(PartOneArray, sizeof(int), ArrayOneSize, FinalProduct);
    fprintf(FinalProductDebug, "\nNodes:\n");
    fprintf(FinalProductDebug, "-----------------------------------------------------------------------------\n");
    fprintf(FinalProductDebug, "  Num |  EOW  List Format        Child       | Child ( dist ) | List | WTEOBL\n");
    fprintf(FinalProductDebug, "-----------------------------------------------------------------------------\n");
    char TheNodeInBinary[32+5+1];
    int maxJump = 0;
    int zeroNodes = 0;
    int maxZeroNodes = 0;
    int maxZeroIndex = 0;
    for(int i = 0; i < ArrayOneSize; i++) {
        ConvertIntNodeToBinaryString(PartOneArray[i], TheNodeInBinary);
        int wtebol = 0;
        if(i <= NUMBER_OF_ENGLISH_LETTERS) {
            wtebol = PartThreeArray[i];
        } else if(i <= ArrayFourSize) {
            wtebol = PartFourArray[i];
        } else {
            wtebol = PartFiveArray[i];
        }
        int dist = abs((PartOneArray[i] & CHILD_MASK) - i);
        if((PartOneArray[i] & CHILD_MASK) == 0) {
            dist = 0;
            zeroNodes++;
        } else {
            if(zeroNodes > maxZeroNodes) {
                maxZeroNodes = zeroNodes;
                maxZeroIndex = i - 1;
            }
            zeroNodes = 0;
        }
        fprintf(FinalProductDebug, "%6d  %s %6d (%6d)  %6d  %6d\n", i, TheNodeInBinary, PartOneArray[i] & CHILD_MASK, dist, (PartOneArray[i] & INTERNAL_LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT, wtebol);
        if(dist > 65536) {
            maxJump++;
        }
    }
    printf("Max jumps = %d\n", maxJump);
    printf("Max consecutive zero nodes = %d (index %d)\n", maxZeroNodes, maxZeroIndex);
    fwrite(PartTwoArray, sizeof(int), ArrayTwoSize, FinalProduct);
    fprintf(FinalProductDebug, "\nChildLists:\n");
    fprintf(FinalProductDebug, "------------------------------------------------------------------------\n");
    fprintf(FinalProductDebug, "  Num |  +letter z     ChildListBinary    a | Letters [ + letter]       \n");
    fprintf(FinalProductDebug, "------------------------------------------------------------------------\n");
    char TheChildListInBinary[32+4+1];
    for(int i = 0; i < ArrayTwoSize; i++) {
        ConvertChildListIntToBinaryString(PartTwoArray[i], TheChildListInBinary);
        fprintf(FinalProductDebug, "%6d  %s ", i, TheChildListInBinary);
        int numLetters = 0;
        for(int j = NUMBER_OF_ENGLISH_LETTERS - 1; j >= 0; j--) {
            if(PowersOfTwo[j] & PartTwoArray[i]) {
                fprintf(FinalProductDebug, "%c", j + 'a');
                numLetters++;
            }
        }
        assert(!(PartTwoArray[i] & 0x80000000));
        if(PartTwoArray[i] >> NUMBER_OF_ENGLISH_LETTERS) {
            fprintf(FinalProductDebug, " + %c", '`' + (PartTwoArray[i] >> NUMBER_OF_ENGLISH_LETTERS));
        }
        fprintf(FinalProductDebug, "\n");
    }
    fwrite(PartThreeArray, sizeof(int), ArrayThreeSize, FinalProduct);
    fwrite(PartFourArray, sizeof(short int), ArrayFourSize, FinalProduct);
    fwrite(PartFiveArray, sizeof(unsigned char), ArrayFiveSize, FinalProduct);

    fclose(FinalProductDebug);
    fclose(FinalProduct);

    printf("\n  The new CWG is ready to use.\n\n");

    int dawgSize = 1 * sizeof(int) + ArrayOneSize * sizeof(int) + ArrayTwoSize * sizeof(int);
    int CWGSize = 4 * sizeof(int) + ArrayThreeSize * sizeof(int) + ArrayFourSize * sizeof(short int) + ArrayFiveSize * sizeof(unsigned char);
    int totalSize = dawgSize + CWGSize;
    printf("Size = %d kb (%d + %d bytes)\n", (int)round(totalSize / 1000.0), dawgSize, CWGSize);

    return 0;
}
