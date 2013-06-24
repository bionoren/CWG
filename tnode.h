//
//  tnode.h
//  wwfmax
//
//  Created by Bion Oren on 11/5/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#ifndef wwfmax_tnode_h
#define wwfmax_tnode_h

#import <stdbool.h>

struct tnode {
    struct tnode* Next;
    struct tnode* Child;
    struct tnode* ParentalUnit;
    struct tnode* ReplaceMeWith;
    // When populating the DAWG array, you must know the index assigned to each "Child".
    // To get this information, "ArrayIndex" is stored in every node, so that we can mine the information from the reduced pointer-style-trie.
    int ArrayIndex;
    char DirectChild;
    char Letter;
    char MaxChildDepth;
    char Level;
    char NumberOfChildren;
    char Dangling;
    char Protected;
    char EndOfWordFlag;
};

typedef struct tnode Tnode;
typedef Tnode* TnodePtr;

TnodePtr TnodeChild (TnodePtr ThisTnode);
TnodePtr TnodeInit(char Chap, TnodePtr OverOne, char WordEnding, char Leveler, int StarterDepth, TnodePtr Parent, char IsaChild);
TnodePtr TnodeFindParaNode(TnodePtr ThisTnode, char ThisLetter);
void TnodeInsertParaNode(TnodePtr AboveTnode, char ThisLetter, char WordEnder, int StartDepth);
void TnodeSetMaxChildDepth(TnodePtr ThisTnode, int NewDepth);
char TnodeMaxChildDepth(TnodePtr ThisTnode);
void TnodeFlyEndOfWordFlag(TnodePtr ThisTnode);
void TnodeSetNext(TnodePtr ThisTnode, TnodePtr Nexis);
TnodePtr TnodeNext(TnodePtr ThisTnode);
char TnodeDangling(TnodePtr ThisTnode);
void TnodeProtect(TnodePtr ThisTnode);
void TnodeSetReplaceMeWith(TnodePtr ThisTnode, TnodePtr Living);
int TnodeArrayIndex(TnodePtr ThisTnode);
void TnodeSetArrayIndex(TnodePtr ThisTnode, int TheWhat);
char TnodeDirectChild(TnodePtr ThisTnode);
bool TnodeProtectionCheck(TnodePtr ThisTnode, bool CheckThisTnode);
char TnodeAreWeTheSame(TnodePtr FirstNode, TnodePtr SecondNode);
int TnodeDangle(TnodePtr ThisTnode);
void TnodeGraphTabulateRecurse(TnodePtr ThisTnode, int Level, int* Tabulator);
void FreeTnodeRecurse(TnodePtr ThisTnode);
void TnodeLawnMowerRecurse(TnodePtr ThisTnode);
bool TnodeProtectAndReplaceBranchStructure(TnodePtr EliminateThis, TnodePtr ReplaceWith, bool ValidReplacement);

#endif