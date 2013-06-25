//
//  DictionaryManager.h
//  wwfmax
//
//  Created by Bion Oren on 12/9/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#import "CWGLib.h"
#import <stdbool.h>
#include <libkern/OSAtomic.h>

#define HASH_DEBUG (defined DEBUG && (NUM_THREADS == 1))

typedef struct dictStack {
    //current node list. This will get you a node, which lets you go down a level.
    int index; //index into the start of a child group in the nodeArray
    char childLetterFormatOffset; //english letter offset into the child group (a=0 to z)
    int childListFormat; //z-a bitstring (NOT index into the childLists table)
} dictStack;

typedef struct DictionaryManager {
    dictStack stack[WORD_LENGTH+ 1];
    //word we've currently built / are building
    char tmpWord[WORD_LENGTH];
    //position in all of the above stacks
    int stackDepth;
    OSSpinLock lock;
#if HASH_DEBUG
    int lastHash;
#endif
} DictionaryManager;

bool isValidWord(const char *TheCandidate, int CandidateLength);
int hashWord(const char *TheCandidate, const int CandidateLength);
int nextWord(DictionaryManager *mgr, char *outWord);
int numWords();
void resetManager(DictionaryManager *mgr);

DictionaryManager *createDictManager();
void freeDictManager(DictionaryManager *mgr);
void destructDictManager();
