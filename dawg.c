//
//  dawg.c
//  wwfmax
//
//  Created by Bion Oren on 11/10/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include "dawg.h"

// Set up the parent nodes in the Dawg.
DawgPtr DawgInit(void) {
    DawgPtr Result = (Dawg *)malloc(sizeof(Dawg));
    Result->NumberOfTotalWords = 0;
    Result->NumberOfTotalNodes = 0;
    Result->First = TnodeInit('0', NULL, false, 0, 0, NULL, false);
    return Result;
}

// This function is responsible for adding "Word" to the "Dawg" under its root node.  It returns the number of new nodes inserted.
int TnodeDawgAddWord(TnodePtr ParentNode, const char *Word) {
    int Result = 0;
    int WordLength = (int)strlen(Word);
    TnodePtr Current = ParentNode;
    for(int i = 0; i < WordLength; i++) {
        TnodePtr HangPoint = TnodeFindParaNode(TnodeChild(Current), Word[i]);
        if(HangPoint == NULL) {
            TnodeInsertParaNode(Current, Word[i], (i == WordLength - 1 ? true : false), WordLength - i - 1);
            Result++;
            Current = TnodeFindParaNode(TnodeChild(Current), Word[i]);
            for(int j = i + 1; j < WordLength; j++) {
                TnodeInsertParaNode(Current, Word[j], (j == WordLength - 1 ? true : false), WordLength - j - 1);
                Result += 1;
                Current = TnodeChild(Current);
            }
            break;
        } else {
            if(TnodeMaxChildDepth(HangPoint) < WordLength - i - 1) {
                TnodeSetMaxChildDepth(HangPoint, WordLength - i - 1);
            }
        }
        Current = HangPoint;
        // The path for the word that we are trying to insert already exists, so just make sure that the end flag is flying on the last node.
        // This should never happen if we are to add words in increasing word length.
        if(i == WordLength - 1) {
            TnodeFlyEndOfWordFlag(Current);
        }
    }
    return Result;
}

// Add "NewWord" to "ThisDawg", which at this point is a "Trie" with a lot of information in each node.
// "NewWord" must not exist in "ThisDawg" already.
void DawgAddWord(DawgPtr ThisDawg, char *NewWord) {
    ThisDawg->NumberOfTotalWords++;
    int NodesAdded = TnodeDawgAddWord(ThisDawg->First, NewWord);
    ThisDawg->NumberOfTotalNodes += NodesAdded;
}

// Count the "Living" "Tnode"s of each "MaxChildDepth" in "ThisDawg", and store the values in "Count".
void DawgGraphTabulate(DawgPtr ThisDawg, int* Count) {
    int Numbers[MAX];
    for(int i = 0; i < MAX; i++) {
        Numbers[i] = 0;
    }
    if(ThisDawg->NumberOfTotalWords > 0) {
        TnodeGraphTabulateRecurse(ThisDawg->First, 0, Numbers);
        for(int i = 0; i < MAX; i++) {
            Count[i] = Numbers[i];
        }
    }
}

// This function can never be called after a trie has been "Mowed" because this will result in pointers being freed twice.
// A core dump is bad.
void FreeDawg(DawgPtr ThisDawg) {
    if(ThisDawg->NumberOfTotalWords > 0) {
        FreeTnodeRecurse(ThisDawg->First);
    }
    free(ThisDawg);
}

// Replaces all pointers to "Dangling" "Tnodes" in the "ThisDawg" Trie with living ones.
void DawgLawnMower(DawgPtr ThisDawg) {
    TnodeLawnMowerRecurse(ThisDawg->First);
}