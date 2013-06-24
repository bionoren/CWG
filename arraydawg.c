//
//  arraydawg.c
//  wwfmax
//
//  Created by Bion Oren on 11/10/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "arraydawg.h"
#include "CWGLib.h"
#include "dawg.h"
#include "breadthqueue.h"
#include "assert.h"

#define TRADITIONAL_CHILD_SHIFT 5
#define TRADITIONAL_EOW_FLAG 0X00800000
#define TRADITIONAL_EOL_FLAG 0X00400000

// The "BinaryNode" string must be at least 32 + 5 + 1 bytes in length.  Space for the bits, the seperation pipes, and the end of string char.
// This function is used to fill the text file used to inspect the graph created in the first segment of the program.
void ConvertIntNodeToBinaryString(int TheNode, char *BinaryNode) {
    BinaryNode[0] = '[';
    // Bit 31 is not being used.  It will always be '0'.
    BinaryNode[1] = '_';
    BinaryNode[2] = '|';
    // Bit 30 holds the End-Of-Word, EOW_FLAG.
    BinaryNode[3] = (TheNode & PowersOfTwo[30])?'1':'0';
    BinaryNode[4] = '|';
    // 13 Bits, (29-->17) represent the child-format index.
    for(int i = 5, Bit = 29; i <= 17; i++, Bit--) {
        BinaryNode[i] = (TheNode & PowersOfTwo[Bit])?'1':'0';
    }
    BinaryNode[18] = '|';
    // The "Child" index requires 17 bits, and it will complete the 32 bit int.
    for(int i = 19, Bit = 16; i <= 35; i++, Bit--) {
        BinaryNode[i] = (TheNode & PowersOfTwo[Bit])?'1':'0';
    }
    BinaryNode[36] = ']';
    BinaryNode[37] = '\0';
}

// The "BinaryChildList" string must be at least 32 + 3 + 1 bytes in length.  Space for the bits, the seperation pipes, and the end of string char.
void ConvertChildListIntToBinaryString(int TheChildList, char *BinaryChildList) {
    BinaryChildList[0] = '[';
    BinaryChildList[1] = '_';
    BinaryChildList[2] = '|';
    for(int i = 3, Bit = 30; i <= 7; i++, Bit--) {
        BinaryChildList[i] = (TheChildList & PowersOfTwo[Bit])?'1':'0';;
    }
    BinaryChildList[8] = '|';
    for(int i = 9, Bit = 25; i <= 34; i++, Bit--) {
        BinaryChildList[i] = (TheChildList & PowersOfTwo[Bit])?'1':'0';
    }
    BinaryChildList[35] = ']';
    BinaryChildList[36] = '\0';
}

void ArrayDnodeInit(ArrayDnodePtr ThisArrayDnode, char Chap, int Nextt, int Childd, char EndingFlag, char Breadth) {
    ThisArrayDnode->Letter = Chap;
    ThisArrayDnode->EndOfWordFlag = EndingFlag;
    ThisArrayDnode->Next = Nextt;
    ThisArrayDnode->Child = Childd;
    ThisArrayDnode->Level = Breadth;
}

void ArrayDnodeTnodeTranspose(ArrayDnodePtr ThisArrayDnode, TnodePtr ThisTnode) {
    ThisArrayDnode->Letter = ThisTnode->Letter;
    ThisArrayDnode->EndOfWordFlag = ThisTnode->EndOfWordFlag;
    ThisArrayDnode->Level = ThisTnode->Level;
    if(ThisTnode->Next == NULL) {
        ThisArrayDnode->Next = 0;
    } else {
        ThisArrayDnode->Next = (ThisTnode->Next)->ArrayIndex;
    }
    if(ThisTnode->Child == NULL) {
        ThisArrayDnode->Child = 0;
    } else {
        ThisArrayDnode->Child = (ThisTnode->Child)->ArrayIndex;
    }
}

int ArrayDnodeNext(ArrayDnodePtr ThisArrayDnode) {
    return ThisArrayDnode->Next;
}

int ArrayDnodeChild (ArrayDnodePtr ThisArrayDnode) {
    return ThisArrayDnode->Child;
}

char ArrayDnodeLetter(ArrayDnodePtr ThisArrayDnode) {
    return ThisArrayDnode->Letter;
}

char ArrayDnodeEndOfWordFlag(ArrayDnodePtr ThisArrayDnode) {
    return ThisArrayDnode->EndOfWordFlag;
}

int ArrayDnodeNumberOfChildrenPlusString(ArrayDnodePtr DoggieDog, int Index, char* FillThisString) {
    if(DoggieDog[Index].Child == 0) {
        FillThisString[0] = '\0';
        return 0;
    }
    int CurrentArrayPosition = DoggieDog[Index].Child;
    for(int i = 0; i < NUMBER_OF_ENGLISH_LETTERS; i++) {
        FillThisString[i] = DoggieDog[CurrentArrayPosition].Letter;
        if(DoggieDog[CurrentArrayPosition].Next == 0) {
            FillThisString[i + 1] = '\0';
            return i + 1;
        }
        CurrentArrayPosition++;
    }
    return 0;
}

void printDawgDotRecurse(ArrayDawgPtr dawg, FILE *dawgdot, int parent, int index) {
    ArrayDnode node = dawg->DawgArray[index];

    fprintf(dawgdot, "%d [label=\"%c(%d)\"];\n", index, node.Letter, index);
    if(node.EndOfWordFlag) {
        fprintf(dawgdot, "%d [color=blue, style=bold]", index);
    }

    if(node.Child) {
        fprintf(dawgdot, "%d -> %d;\n", index, node.Child);
        printDawgDotRecurse(dawg, dawgdot, index, node.Child);
    }
    if(node.Next) {
        fprintf(dawgdot, "%d -> %d;\n", parent, node.Next);
        printDawgDotRecurse(dawg, dawgdot, parent, node.Next);
    }
}

void printDawgDot(ArrayDawgPtr dawg, int numNodes) {
    FILE *dawgdot = fopen("dawg.dot", "w");
    fprintf(dawgdot, "digraph dawg {\n");

    for(int i = 0; i < numNodes; i++) {
        if(dawg->DawgArray[i].Level == 1) {
            fprintf(dawgdot, "%d [color=red, style=bold]", i);
            printDawgDotRecurse(dawg, dawgdot, i, i);
        }
    }

    fprintf(dawgdot, "}");
    fclose(dawgdot);
}

// This function is the core of the dawg creation procedure.  Pay close attention to the order of the steps involved.

ArrayDawgPtr ArrayDawgInit(char **Dictionary, int *SegmentLenghts, int MaxStringLength) {
    printf("Step 0 - Allocate the framework for the intermediate Array-Data-Structure.\n");
    // Dynamically allocate the upper Data-Structure.
    ArrayDawgPtr Result = (ArrayDawgPtr)malloc(sizeof(ArrayDawg));
    // set MinStringLength, MaxStringLength, and NumberOfStrings.

    int index = 0;
    while(SegmentLenghts[index] == 0) {
        index++;
    }
    Result->MinStringLength = (char)index;
    Result->MaxStringLength = (char)MaxStringLength;
    Result->NumberOfStrings = 0;
    for(int i = Result->MinStringLength; i <= Result->MaxStringLength; i++) {
        Result->NumberOfStrings += SegmentLenghts[i];
    }

    printf("\nStep 1 - Create a Temporary-Working-Trie and begin filling it with the |%d| words.\n", Result->NumberOfStrings);
    /// Create a Temp Trie structure and then feed in the given dictionary.
    DawgPtr TemporaryTrie = DawgInit();
    for(int j = Result->MinStringLength; j <= Result->MaxStringLength; j++) {
        for(int i = 0; i < SegmentLenghts[j]; i++) {
            DawgAddWord(TemporaryTrie, &(Dictionary[j][(j + 1) * i]));
        }
    }

    printf("\nStep 2 - Finished adding words to the Temporary-Working-Trie.\n");
    // Allocate two "Tnode" counter arrays.
    int *NodeNumberCounter = (int*)calloc((Result->MaxStringLength), sizeof(int));
    int *NodeNumberCounterInit = (int*)calloc((Result->MaxStringLength), sizeof(int));

    // Count up the number of "Tnode"s in the Raw-Trie according to MaxChildDepth.
    printf("\nStep 3 - Count the total number of Tnodes in the Raw-Trie according to MaxChildDepth.\n");
    DawgGraphTabulate(TemporaryTrie, NodeNumberCounter);

    printf("\nStep 4 - Initial Tnode counting is complete, so display results:\n\n");
    int TotalNodeSum = 0;
    for(int i = 0; i < Result->MaxStringLength; i++) {
        NodeNumberCounterInit[i] = NodeNumberCounter[i];
        TotalNodeSum += NodeNumberCounter[i];
    }
    for(int i = 0; i < Result->MaxStringLength; i++) {
        printf("  Initial Tnode Count For MaxChildDepth =|%2d| is |%6d|\n", i, NodeNumberCounterInit[i]);
    }
    printf("\n  Total Tnode Count For The Raw-Trie = |%d| \n", TotalNodeSum);
    // We will have exactly enough space for all of the Tnode pointers.

    printf("\nStep 5 - Allocate a 2 dimensional array of Tnode pointers to search for redundant Tnodes.\n");
    TnodePtr ** HolderOfAllTnodePointers = (TnodePtr **)malloc((Result->MaxStringLength)*sizeof(TnodePtr *));
    for(int i = 0; i < MAX; i++) {
        HolderOfAllTnodePointers[i] = (TnodePtr*)malloc(NodeNumberCounterInit[i] * sizeof(TnodePtr));
    }
    // A breadth-first traversal is used when populating the final array.
    // It is then much more likely for living "Tnode"s to appear first, if we fill "HolderOfAllTnodePointers" breadth first.

    printf("\nStep 6 - Populate the 2 dimensional Tnode pointer array.\n");
    // Use a breadth first traversal to populate the "HolderOfAllTnodePointers" array.
    BreadthQueuePtr Populator = BreadthQueueInit();
    BreadthQueuePopulateReductionArray(Populator, (TemporaryTrie->First)->Child, HolderOfAllTnodePointers);

    printf("\nStep 7 - Population complete.\n");
    // Flag all of the reduntant "Tnode"s, and store a "ReplaceMeWith" "Tnode" reference inside the "Dangling" "Tnode"s.
    // Flagging requires the "TnodeAreWeTheSame()" function, and beginning with the highest "MaxChildDepth" "Tnode"s will reduce the processing time.
    int NumberDangled = 0;
    int DangledNow;
    int NumberAtHeight;
    int TotalDangled = 0;

    // keep track of the number of nodes of each MaxChildDepth dangled recursively so we can check how many remaining nodes we need for the optimal array.
    int DangleCount[Result->MaxStringLength];
    for(int i = 0; i < Result->MaxStringLength; i++) {
        DangleCount[i] = 0;
    }

    printf("\nStep 8 - Tag redundant Tnodes as Dangling - Use recursion, because only DirectChild Tnodes are considered for elimination:\n");
    printf("\n  This procedure is at the very heart of the CWG creation alogirthm, and it would be much slower, without heavy optimization.\n");
    printf("\n  ---------------------------------------------------------------------------------------------------------------------------\n");
    // *** Test the other way.  Start at the largest "MaxChildDepth" and work down from there for recursive reduction to take place.
    for(int j = Result->MaxStringLength - 1; j >= 0; j--) {
        NumberDangled = 0;
        NumberAtHeight = 0;
        // "X" is the index of the node we are trying to kill.
        for(int i = NodeNumberCounterInit[j] - 1; i >= 0; i--) {
            // If the node is "Dangling" already, or it is not a "DirectChild", then "continue".
            if(TnodeDangling(HolderOfAllTnodePointers[j][i])) {
                continue;
            }
            if(!TnodeDirectChild(HolderOfAllTnodePointers[j][i])) {
                continue;
            }
            // Make sure that we don't emiminate "Tnodes" being pointed to by other "Tnodes" in the graph.
            // This is a tricky procedure because node beneath "X" can be "Protected".
            // "W" will be the index of the first undangled "Tnode" with the same structure, if one exists.
            for(int k = 0; k < NodeNumberCounterInit[j]; k++) {
                if(k == i) {
                    continue;
                }
                if(!TnodeDangling(HolderOfAllTnodePointers[j][k])) {
                    if(TnodeAreWeTheSame(HolderOfAllTnodePointers[j][i], HolderOfAllTnodePointers[j][k])) {
                        // In the special case where the node being "Dangled" has "Protected" nodes beneath it, more needs to be done.
                        // When we "Dangle" a "Protected" "Tnode", we must set it's "ReplaceMeWith", and a recursive function is needed for this special case.
                        // This construct deals with regular and "Crosslink"ed branch structures.
                        // It happens when "Protected" "Tnodes come beneath the one we want to "Dangle".
                        if(TnodeProtectionCheck(HolderOfAllTnodePointers[j][i], false)) {
                            while(!TnodeProtectAndReplaceBranchStructure(HolderOfAllTnodePointers[j][i], HolderOfAllTnodePointers[j][k], false));
                            TnodeProtectAndReplaceBranchStructure(HolderOfAllTnodePointers[j][i], HolderOfAllTnodePointers[j][k], true);
                        }
                        // Set the "Protected" and "ReplaceMeWith" status of the corresponding top-level "Tnode"s.
                        TnodeProtect(HolderOfAllTnodePointers[j][k]);
                        TnodeSetReplaceMeWith(HolderOfAllTnodePointers[j][i], HolderOfAllTnodePointers[j][k]);
                        // "Dangle" all nodes under "HolderOfAllTnodePointers[Y][X]", and update the "Dangle" counters.
                        NumberAtHeight++;
                        DangledNow = TnodeDangle(HolderOfAllTnodePointers[j][i]);
                        NumberDangled += DangledNow;
                        break;
                    }
                }
            }
        }
        printf("  Dangled |%5d| Tnodes, and |%5d| Tnodes In all, through recursion, for MaxChildDepth of |%2d|\n", NumberAtHeight, NumberDangled, j);
        DangleCount[j] = NumberDangled;
        TotalDangled += NumberDangled;
    }
    printf("  ---------------------------------------------------------------------------------------------------------------------------\n");

    int NumberOfLivingNodes;
    printf("\n  |%6d| = Original # of Tnodes.\n", TotalNodeSum);
    printf("  |%6d| = Dangled # of Tnodes.\n", TotalDangled);
    printf("  |%6d| = Remaining # of Tnodes.\n", NumberOfLivingNodes = TotalNodeSum - TotalDangled);

    printf("\nStep 9 - Count the number of living Tnodes by traversing the Raw-Trie to check the Dangling numbers.\n\n");
    DawgGraphTabulate(TemporaryTrie, NodeNumberCounter);
    for(int i = 0; i < Result->MaxStringLength; i++ ) {
        printf("  New count for MaxChildDepth |%2d| Tnodes is |%5d|.  Tnode count was |%6d| in Raw-Trie pre-Dangling.  Killed |%6d| Tnodes.\n", i, NodeNumberCounter[i], NodeNumberCounterInit[i], NodeNumberCounterInit[i] - NodeNumberCounter[i]);
    }
    int TotalDangledCheck = 0;
    for(int i = 0; i < MAX; i++) {
        TotalDangledCheck += NodeNumberCounterInit[i] - NodeNumberCounter[i];
    }
    if(TotalDangled == TotalDangledCheck) {
        printf("\n  Tnode Dangling count is consistent.\n");
    } else {
        printf("\n  MISMATCH for Tnode Dangling count.\n");
    }

    printf("\nStep 9.5 - Run a final check to verify that all redundant nodes have been Dangled.\n\n");

    for(int j = Result->MaxStringLength - 1; j >= 0; j--) {
        NumberAtHeight = 0;
        // "X" is the index of the node we are trying to kill.
        for(int i = NodeNumberCounterInit[j] - 1; i >= 0; i-- ) {
            // If the node is "Dangling" already, or it is not a "DirectChild", then "continue".
            if(TnodeDangling(HolderOfAllTnodePointers[j][i])) {
                continue;
            }
            if(!TnodeDirectChild(HolderOfAllTnodePointers[j][i])) {
                continue;
            }
            // "W" will be the index of the first undangled "Tnode" with the same structure, if one slipped through the cracks.
            for(int k = 0; k < NodeNumberCounterInit[j]; k++) {
                if(k == i) {
                    continue;
                }
                if(!TnodeDangling(HolderOfAllTnodePointers[j][k])) {
                    if(TnodeAreWeTheSame(HolderOfAllTnodePointers[j][i], HolderOfAllTnodePointers[j][k])) {
                        NumberAtHeight++;
                        break;
                    }
                }
            }
        }
        printf("  MaxChildDepth |%2d| - Identical living nodes found = |%2d|.\n", j, NumberAtHeight);
    }

    printf("\nstep 10 - Kill the Dangling Tnodes using the internal \"ReplaceMeWith\" values.\n");
    // Node replacement has to take place before indices are set up so nothing points to redundant nodes.
    // - This step is absolutely critical.  Mow The Lawn so to speak!  Then Index.
    DawgLawnMower(TemporaryTrie);
    printf("\n  Killing complete.\n");

    printf("\nStep 11 - Dawg-Lawn-Mowing is now complete, so assign array indexes to all living Tnodes using a Breadth-First-Queue.\n");
    BreadthQueuePtr OrderMatters = BreadthQueueInit();
    // The Breadth-First-Queue must assign an index value to each living "Tnode" only once.
    // "HolderOfAllTnodePointers[MAX - 1][0]" becomes the first node in the new DAWG array.
    int IndexCount = BreadthQueueUseToIndex(OrderMatters, HolderOfAllTnodePointers[MAX - 1][0]);
    printf("\n  Index assignment is now complete.\n");
    printf("\n  |%d| = NumberOfLivingNodes from after the Dangling process.\n", NumberOfLivingNodes);
    printf("  |%d| = IndexCount from the breadth-first assignment function.\n", IndexCount);

    // Allocate the space needed to store the "DawgArray".
    Result->DawgArray = (ArrayDnodePtr)calloc((NumberOfLivingNodes + 1), sizeof(ArrayDnode));
    int IndexFollow = 0;
    int IndexFollower = 0;
    int TransposeCount = 0;
    // Roll through the pointer arrays and use the "ArrayDnodeTnodeTranspose" function to populate it.
    // Set the dummy entry at the beginning of the array.
    ArrayDnodeInit(&(Result->DawgArray[0]), 0, 0, 0, 0, 0);
    Result->First = 1;

    printf("\nStep 12 - Populate the new Working-Array-Dawg structure, which is used to verify validity and create the final integer-graph-encodings.\n");
    // Scroll through "HolderOfAllTnodePointers" and look for un"Dangling" "Tnodes", if so then transpose them into "Result->DawgArray".
    for(int i = Result->MaxStringLength - 1; i >= 0; i--) {
        for(int k = 0; k < NodeNumberCounterInit[i]; k++ ) {
            if(!TnodeDangling(HolderOfAllTnodePointers[i][k])) {
                IndexFollow = TnodeArrayIndex(HolderOfAllTnodePointers[i][k]);
                ArrayDnodeTnodeTranspose(&(Result->DawgArray[IndexFollow]), HolderOfAllTnodePointers[i][k]);
                TransposeCount++;
                if(IndexFollow > IndexFollower) {
                    IndexFollower = IndexFollow;
                }
            }
        }
    }
    printf("\n  |%d| = IndexFollower, which is the largest index assigned in the Working-Array-Dawg.\n", IndexFollower);
    printf("  |%d| = TransposeCount, holds the number of Tnodes transposed into the Working-Array-Dawg.\n", TransposeCount);
    printf("  |%d| = NumberOfLivingNodes.  Make sure that these three values are equal, because they must be.\n", NumberOfLivingNodes);
    if((IndexFollower == TransposeCount) && (IndexFollower == NumberOfLivingNodes)) {
        printf("\n  Equality assertion passed.\n");
    } else {
        printf("\n  Equality assertion failed.\n");
    }

    // Conduct dynamic-memory-cleanup and free the whole Raw-Trie, which is no longer needed.
    for(int i = 0; i < Result->MaxStringLength; i++ ) {
        for(int j = 0; j < NodeNumberCounterInit[i]; j++) {
            free(HolderOfAllTnodePointers[i][j]);
        }
    }
    free(TemporaryTrie);
    free(NodeNumberCounter);
    free(NodeNumberCounterInit);
    for(int i = 0; i < Result->MaxStringLength; i++) {
        free(HolderOfAllTnodePointers[i]);
    }
    free(HolderOfAllTnodePointers);

    printf("\nStep 13 - Creation of the traditional-DAWG is complete, so store it in a binary file for use.\n");

    FILE *Data = fopen(TRADITIONAL_DAWG_DATA,"wb");
    // The "NULL" node in position "0" must be counted now.
    int CurrentNodeInteger = NumberOfLivingNodes + 1;
    // It is critical, especially in a binary file, that the first integer written to the file be the number of nodes stored in the file.
    fwrite(&CurrentNodeInteger, sizeof(int), 1, Data);
    // Write the "NULL" node to the file first.
    CurrentNodeInteger = 0;
    fwrite(&CurrentNodeInteger, sizeof(int), 1, Data);
    for(int i = 1; i <= NumberOfLivingNodes; i++) {
        CurrentNodeInteger = (Result->DawgArray)[i].Child;
        CurrentNodeInteger <<= TRADITIONAL_CHILD_SHIFT;
        CurrentNodeInteger += ((Result->DawgArray)[i].Letter) - 'a';
        if((Result->DawgArray)[i].EndOfWordFlag) {
            CurrentNodeInteger += TRADITIONAL_EOW_FLAG;
        }
        if((Result->DawgArray)[i].Next == 0) {
            CurrentNodeInteger += TRADITIONAL_EOL_FLAG;
        }
        fwrite(&CurrentNodeInteger, sizeof(int), 1, Data);
    }
    fclose(Data);
    printf("\n  The Traditional-DAWG-Encoding data file is now written.\n");

    printf("\nStep 14 - Create a preliminary encoding of the more advanced CWG, and store these intermediate arrays into data files.\n");

    FILE *Text = fopen(TRADITIONAL_DAWG_TEXT_DATA,"w");

    FILE *Main = fopen(DIRECT_GRAPH_DATA_PART_ONE,"wb");
    FILE *Secondary = fopen(DIRECT_GRAPH_DATA_PART_TWO,"wb");

    // The following variables will be used when setting up the child-List-Format integer values.
    char CurrentChildLetterString[NUMBER_OF_ENGLISH_LETTERS + 1];
    CurrentChildLetterString[0] = '\0';
    char TheNodeInBinary[32+5+1];
    char TheChildListInBinary[32+4+1];
    TheNodeInBinary[0] = '\0';

    int CurrentOffsetNumberIndex;
    int CompleteThirtyTwoBitNode;
    fwrite(&NumberOfLivingNodes, 4, 1, Main);

    int EndOfListCount = 0;
    int EOLTracker = 0;
    int *EndOfListIndicies;
    FILE *ListE = fopen(FINAL_NODES_DATA, "wb");

    // Set up an array to hold all of the unique child strings for the reduced lexicon DAWG.  The empty placeholder will be all zeds.
    int NumberOfUniqueChildStrings = 0;
    int InsertionPoint = 0;
    char **HolderOfUniqueChildStrings = (char**)malloc(NumberOfLivingNodes * sizeof(char*));
    for(int i = 0; i < NumberOfLivingNodes; i++) {
        HolderOfUniqueChildStrings[i] = (char*)malloc((NUMBER_OF_ENGLISH_LETTERS + 1) * sizeof(char));
        strcpy(HolderOfUniqueChildStrings[i], "ZZZZZZZZZZZZZZZZZZZZZZZZZZ");
    }

    // Right here we will tabulate the child information so that it can be turned into an "int" array and stored in a data file.
    // Also, we need to count the number of unique list structures, and calculate the number of bits required to store index values for them.
    // The idea is that there are a small number of actual values that these 26 bits will hold due to patterns in the English Language.
    for(int i = 1; i <= NumberOfLivingNodes; i++) {
        ArrayDnodeNumberOfChildrenPlusString(Result->DawgArray, i, CurrentChildLetterString);
        // Insert the "CurrentChildLetterString" into the "HolderOfUniqueChildStrings" if, and only if, it is unique.
        bool IsSheUnique = true;
        for(int j = 0; j < NumberOfUniqueChildStrings; j++) {
            if(strcmp(CurrentChildLetterString, HolderOfUniqueChildStrings[j]) == 0) {
                IsSheUnique = false;
                InsertionPoint = 0;
                break;
            }
            if(strcmp(CurrentChildLetterString, HolderOfUniqueChildStrings[j]) < 0) {
                IsSheUnique = true;
                InsertionPoint = j;
                break;
            }
        }
        if(IsSheUnique) {
            char *TempHolder = HolderOfUniqueChildStrings[NumberOfUniqueChildStrings];
            strcpy(TempHolder, CurrentChildLetterString);
            memmove(HolderOfUniqueChildStrings + InsertionPoint + 1, HolderOfUniqueChildStrings + InsertionPoint, (NumberOfUniqueChildStrings - InsertionPoint)*sizeof(char*));
            HolderOfUniqueChildStrings[InsertionPoint] = TempHolder;
            NumberOfUniqueChildStrings++;
        }
    }

    printf("\nStep 15 - NumberOfUniqueChildStrings = |%d|.\n", NumberOfUniqueChildStrings);

    int *ChildListValues = (int*)calloc(NumberOfUniqueChildStrings, sizeof(int));

    // Encode the unique child strings as "int"s, so that each corresponding bit is popped.
    for(int i = 0; i < NumberOfUniqueChildStrings; i++) {
        strcpy(CurrentChildLetterString, HolderOfUniqueChildStrings[i]);
        int CurrentNumberOfChildren = (int)strlen(CurrentChildLetterString);
        int CompleteChildList = 0;
        for(int j = 0; j < CurrentNumberOfChildren; j++) {
            CompleteChildList += PowersOfTwo[CurrentChildLetterString[j] - 'a'];
        }
        ChildListValues[i] = CompleteChildList;
    }

    fprintf(Text, "Behold, the |%d| graph nodes are decoded below.\n\n", NumberOfLivingNodes);
    fprintf(Text, "--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(Text, "  Num | EOW   List Format     Children          | NodeVal  | Level |char|EOW| Next| Child |NumChilds |    CurrChildLetterStrs    |         z     ChildListBinary    a | ChildVal\n");
    fprintf(Text, "--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

    // We are now ready to output to the text file, and the "Main" intermediate binary data file.
    for(int i = 1; i <= NumberOfLivingNodes; i++) {
        int CurrentNumberOfChildren = ArrayDnodeNumberOfChildrenPlusString(Result->DawgArray, i, CurrentChildLetterString);

        // Get the correct offset index to store into the current node
        for(int j = 0; j < NumberOfUniqueChildStrings; j++ ) {
            if(strcmp(CurrentChildLetterString, HolderOfUniqueChildStrings[j]) == 0) {
                CurrentOffsetNumberIndex = j;
                break;
            }
        }

        CompleteThirtyTwoBitNode = CurrentOffsetNumberIndex;
        CompleteThirtyTwoBitNode <<= BIT_COUNT_FOR_CHILD_INDEX;
        CompleteThirtyTwoBitNode += (Result->DawgArray)[i].Child;
        // The first "BIT_COUNT_FOR_GRAPH_INDEX" are for the first child index.  The EOW_FLAG is stored in the 2^30 bit.
        // The remaining bits will hold a reference to the child list configuration.  No space is used for a letter.
        // The 2's complement sign bit is not needed, so a signed integer is acceptable.
        if((Result->DawgArray)[i].EndOfWordFlag == 1) {
            CompleteThirtyTwoBitNode += EOW_FLAG;
        }
        fwrite(&CompleteThirtyTwoBitNode, sizeof(int), 1, Main);
        ConvertIntNodeToBinaryString(CompleteThirtyTwoBitNode, TheNodeInBinary);
        ConvertChildListIntToBinaryString(ChildListValues[CurrentOffsetNumberIndex], TheChildListInBinary);
        assert((CurrentNumberOfChildren == 0 && Result->DawgArray[i].EndOfWordFlag) || CurrentNumberOfChildren > 0);
        fprintf(Text, "%6d  %s    %10d      %2d ", i, TheNodeInBinary, CompleteThirtyTwoBitNode, (Result->DawgArray)[i].Level);
        fprintf(Text, " { %c  %d %6d", Result->DawgArray[i].Letter, Result->DawgArray[i].EndOfWordFlag, Result->DawgArray[i].Next);
        fprintf(Text, " %6d}        %2d   %26s ", Result->DawgArray[i].Child , CurrentNumberOfChildren, CurrentChildLetterString);
        fprintf(Text, " %s  %8d\n", TheChildListInBinary, ChildListValues[CurrentOffsetNumberIndex] );
        if(CompleteThirtyTwoBitNode == 0) {
            printf("\n  Error in node encoding process.\n");
            assert(false);
        }
        if(CurrentNumberOfChildren == 1) {
            assert(PowersOfTwo[CurrentChildLetterString[0] - 'a'] == ChildListValues[CurrentOffsetNumberIndex]);
        }
    }
    fclose(Main);

    printDawgDot(Result, NumberOfLivingNodes);

    fwrite(&NumberOfUniqueChildStrings, sizeof(int), 1, Secondary);
    fwrite(ChildListValues, sizeof(int), NumberOfUniqueChildStrings, Secondary);
    fclose(Secondary);

    fprintf(Text, "\nNumber Of Living Nodes |%d| Plus The NULL Node.  Also, there are %d child list ints.\n\n"
            , NumberOfLivingNodes, NumberOfUniqueChildStrings);

    for(int i = 0; i < NumberOfUniqueChildStrings; i++) {
        fprintf(Text, "#%4d - |%26s| - |%8d|\n", i, HolderOfUniqueChildStrings[i], ChildListValues[i]);
    }

    // free all of the memory used to compile the Child-List-Format array.
    for(int i = 0; i < NumberOfLivingNodes; i++) {
        free(HolderOfUniqueChildStrings[i]);
    }
    free(HolderOfUniqueChildStrings);
    free(ChildListValues);

    printf("\nStep 16 - Create an array with all End-Of-List index values.\n");

    for(int i = 1; i <= NumberOfLivingNodes; i++) {
        if((Result->DawgArray)[i].Next == 0){
            EndOfListCount++;
        }
    }
    EndOfListIndicies = (int*)malloc(EndOfListCount*sizeof(int));
    fwrite(&EndOfListCount, sizeof(int), 1, ListE);

    for(int i = 1; i <= NumberOfLivingNodes; i++) {
        if((Result->DawgArray)[i].Next == 0) {
            EndOfListIndicies[EOLTracker] = i;
            EOLTracker++;
        }
    }

    printf("\n  EndOfListCount = |%d|\n", EndOfListCount);

    fwrite(EndOfListIndicies, sizeof(int), EndOfListCount, ListE);

    fclose(ListE);

    fprintf(Text, "\nEndOfListCount |%d|\n\n", EndOfListCount);

    for(int i = 0; i < EndOfListCount; i++) {
        fprintf(Text, "#%5d - |%d|\n", i, EndOfListIndicies[i]);
    }

    fclose(Text);

    printf("\nStep 17 - Creation of Traditional-DAWG-Encoding file, Intermediate-Array files, and text-inspection-file complete.\n");

    return Result;
}