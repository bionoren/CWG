//
//  DictionaryManager.m
//  wwfmax
//
//  Created by Bion Oren on 12/9/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#import "DictionaryManager.h"
#import <string.h>
#import <stdio.h>
#import <stdlib.h>
#import "assert.h"

#define GRAPH_DATA "CWG_Data_For_Word-List.dat"

// The CWG basic-type arrays will have global scope to reduce function-argument overhead.
static int *nodeArray;
static int *listFormatArray;
static int *root_WTEOBL_Array;
static short *short_WTEOBL_Array;
static unsigned char *unsignedChar_WTEOBL_Array;

// These two values are needed for the CWG Hash-Function.
static int WTEOBL_Transition;

// Use the first two CWG arrays to return a boolean value indicating if "TheCandidate" word is in the lexicon.
bool isValidWord(const char *TheCandidate, int CandidateLength) {
    int CurrentNodeIndex = TheCandidate[0] - 'a' + 1;
    for(int i = 1; i < CandidateLength; i++) {
        int node = nodeArray[CurrentNodeIndex];
        if(!(node & CHILD_MASK)) {
            return false;
        }

        bool extendedList = node & EXTENDED_LIST_FLAG;
        int CurrentChildListFormat = listFormatArray[(node & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
        CurrentChildListFormat += extendedList << (CurrentChildListFormat >> NUMBER_OF_ENGLISH_LETTERS);

        int CurrentLetterPosition = TheCandidate[i] - 'a';
        if(!(CurrentChildListFormat & PowersOfTwo[CurrentLetterPosition])) {
            return false;
        } else {
            CurrentNodeIndex = (node & CHILD_MASK) + ListFormatPopCount(CurrentChildListFormat, CurrentLetterPosition) - 1;
        }
    }
    return nodeArray[CurrentNodeIndex] & EOW_FLAG;
}

// Using a novel graph mark-up scheme, this function returns the hash index of "TheCandidate", and "0" if it does not exist.
// This function uses the additional 3 WTEOBL arrays.
int hashWord(const char *TheCandidate, const int CandidateLength) {
    int CurrentLetterPosition = TheCandidate[0] - 'a';
    int CurrentNodeIndex = CurrentLetterPosition + 1;
    int CurrentHashMarker = root_WTEOBL_Array[CurrentNodeIndex];
    for(int i = 1; i < CandidateLength; i++) {
        int node = nodeArray[CurrentNodeIndex];
        if(!(node & CHILD_MASK)) {
            return 0;
        }

        bool extendedList = node & EXTENDED_LIST_FLAG;
        int CurrentChildListFormat = listFormatArray[(node & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
        CurrentChildListFormat += extendedList << (CurrentChildListFormat >> NUMBER_OF_ENGLISH_LETTERS);

        CurrentLetterPosition = TheCandidate[i] - 'a';
        if(!(CurrentChildListFormat & PowersOfTwo[CurrentLetterPosition])) {
            return 0;
        } else {
            CurrentNodeIndex = node & CHILD_MASK;
            int popCount = ListFormatPopCount(CurrentChildListFormat, CurrentLetterPosition) - 1;
            // Use "TheShort_WTEOBL_Array".
            if(CurrentNodeIndex < WTEOBL_Transition) {
                CurrentHashMarker -= short_WTEOBL_Array[CurrentNodeIndex];
                CurrentNodeIndex += popCount;
                CurrentHashMarker += short_WTEOBL_Array[CurrentNodeIndex];
            } else { // Use "TheUnsignedChar_WTEOBL_Array".
                CurrentHashMarker -= unsignedChar_WTEOBL_Array[CurrentNodeIndex - WTEOBL_Transition];
                CurrentNodeIndex += popCount;
                CurrentHashMarker += unsignedChar_WTEOBL_Array[CurrentNodeIndex - WTEOBL_Transition];
            }
            if(nodeArray[CurrentNodeIndex] & EOW_FLAG) {
                CurrentHashMarker--;
            }
        }
    }
    if(nodeArray[CurrentNodeIndex] & EOW_FLAG) {
        return root_WTEOBL_Array[1] - CurrentHashMarker;
    }
    return 0;
}

//cf. Justin-CWG-Search.c
int nextWord(DictionaryManager *mgr, char *outWord) {
    OSSpinLockLock(&mgr->lock);
    if(mgr->stackDepth < 0) {
        OSSpinLockUnlock(&mgr->lock);
        return 0;
    }
    //1. go down as far as you can
    //2. go up 1 AND over 1 - childLetterIndexOffset++ (via childLetterFormatOffset += from childListFormat)
    //3. goto 1
    //stop if you've gone up past 0
    //break if you hit EOW

    assert(mgr->stackDepth >= 0 && mgr->stackDepth <= WORD_LENGTH);
    //load state
    dictStack *item = &(mgr->stack[mgr->stackDepth]);

    //if there are unexplored children
    //else drop back until there are unexplored children
    if(item->index == 0) {
        do {
            for(item->childLetterFormatOffset++; item->childLetterFormatOffset < NUMBER_OF_ENGLISH_LETTERS; item->childLetterFormatOffset++) {
                if(item->childListFormat & PowersOfTwo[item->childLetterFormatOffset]) {
                    ++item->childLetterIndexOffset;
                    goto LOOP_END;
                }
            }
            item--;
        } while(--(mgr->stackDepth) >= 0);
        OSSpinLockUnlock(&mgr->lock);
        return 0;
    }
LOOP_END:;

    //and explore them
    int node;
    do {
        //get the node
        node = nodeArray[item->index + item->childLetterIndexOffset];

        //store the letter and go down
        mgr->tmpWord[(mgr->stackDepth)++] = 'a' + item->childLetterFormatOffset;
        assert(mgr->stackDepth <= WORD_LENGTH);

        //setup the next node
        item++;
        item->index = node & CHILD_MASK;
        item->childLetterIndexOffset = 0;
        item->childLetterFormatOffset = 0;

        bool extendedList = node & EXTENDED_LIST_FLAG;
        int childListFormat = listFormatArray[(node & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
        childListFormat += extendedList << (childListFormat >> NUMBER_OF_ENGLISH_LETTERS);
        item->childListFormat = childListFormat;

        //seek to the first letter in the list
        for(; item->childLetterFormatOffset < NUMBER_OF_ENGLISH_LETTERS && !(item->childListFormat & PowersOfTwo[item->childLetterFormatOffset]); item->childLetterFormatOffset++);
    } while(!(node & EOW_FLAG));

    assert(mgr->stackDepth > 1 && mgr->stackDepth <= WORD_LENGTH);
    strncpy(outWord, mgr->tmpWord, mgr->stackDepth);
    //NSLog(@"Evaluating %.*s", length, tmpWord);

#if HASH_DEBUG
    assert(hashWord(outWord, mgr->stackDepth) == ++(mgr->lastHash));
    assert(isValidWord(outWord, mgr->stackDepth));
#endif
    int ret = mgr->stackDepth;
    OSSpinLockUnlock(&mgr->lock);
    return ret;
}

int numWords() {
    return root_WTEOBL_Array[1];
}

void resetManager(DictionaryManager *mgr) {
#ifdef DEBUG
    printf("Reseting dictionary iterator...\n");
#endif
    mgr->stack[0].index = 1;
    mgr->stack[0].childLetterIndexOffset = 0;
    mgr->stack[0].childLetterFormatOffset = 0;
    mgr->stack[0].childListFormat = 0xFFFFFFFF;
    mgr->stackDepth = 0;
#if HASH_DEBUG
    mgr->lastHash = 0;
#endif
}

DictionaryManager *createDictManager() {
    if(!nodeArray) {
        // Array size variables.
        int NodeArraySize;
        int ListFormatArraySize;
        int Root_WTEOBL_ArraySize;
        int Short_WTEOBL_ArraySize;
        int UnsignedChar_WTEOBL_ArraySize;

        // Read the CWG graph, from the "GRAPH_DATA" file, into the global arrays.
        FILE *data = fopen(GRAPH_DATA, "rb");
        assert(data);

        // Read the array sizes.
        fread(&NodeArraySize, sizeof(int), 1, data);
        assert(NodeArraySize > 0);
        fread(&ListFormatArraySize, sizeof(int), 1, data);
        assert(ListFormatArraySize > 0);
        fread(&Root_WTEOBL_ArraySize, sizeof(int), 1, data);
        assert(Root_WTEOBL_ArraySize > 0);
        fread(&Short_WTEOBL_ArraySize, sizeof(int), 1, data);
        assert(Short_WTEOBL_ArraySize > 0);
        fread(&UnsignedChar_WTEOBL_ArraySize, sizeof(int), 1, data);
        assert(UnsignedChar_WTEOBL_ArraySize > 0);

        // Allocate memory to hold the arrays.
        nodeArray = (int *)malloc(NodeArraySize*sizeof(int));
        listFormatArray = (int *)malloc(ListFormatArraySize*sizeof(int));
        root_WTEOBL_Array = (int *)malloc(Root_WTEOBL_ArraySize*sizeof(int));
        short_WTEOBL_Array = (short int *)malloc(Short_WTEOBL_ArraySize*sizeof(short int));
        unsignedChar_WTEOBL_Array = (unsigned char *)malloc(UnsignedChar_WTEOBL_ArraySize*sizeof(unsigned char));

        // Read the 5 arrays into memory.
        fread(nodeArray, sizeof(int), NodeArraySize, data);
        fread(listFormatArray, sizeof(int), ListFormatArraySize, data);
        fread(root_WTEOBL_Array, sizeof(int), Root_WTEOBL_ArraySize, data);
        fread(short_WTEOBL_Array, sizeof(short int), Short_WTEOBL_ArraySize, data);
        fread(unsignedChar_WTEOBL_Array, sizeof(unsigned char), UnsignedChar_WTEOBL_ArraySize, data);

        fclose(data);

        // Make the proper assignments and adjustments to use the CWG.
        WTEOBL_Transition = Short_WTEOBL_ArraySize;
    }

    DictionaryManager *ret = malloc(sizeof(DictionaryManager));
    resetManager(ret);
    ret->lock = OS_SPINLOCK_INIT;

    return ret;
}

void freeDictManager(DictionaryManager *mgr) {
    free(mgr);
}

void destructDictManager() {
    free(nodeArray);
    free(listFormatArray);
    free(root_WTEOBL_Array);
    free(short_WTEOBL_Array);
    free(unsignedChar_WTEOBL_Array);
}
