//
//  tnode.c
//  wwfmax
//
//  Created by Bion Oren on 11/5/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#include "tnode.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*Trie to Dawg TypeDefs*/

// Functions to access internal "Tnode" members.
int TnodeArrayIndex(TnodePtr ThisTnode) {
    return ThisTnode->ArrayIndex;
}

char TnodeDirectChild(TnodePtr ThisTnode) {
    return ThisTnode->DirectChild;
}

TnodePtr TnodeNext(TnodePtr ThisTnode) {
    return ThisTnode->Next;
}

TnodePtr TnodeChild (TnodePtr ThisTnode) {
    return ThisTnode->Child;
}

TnodePtr TnodeParentalUnit(TnodePtr ThisTnode) {
    return ThisTnode->ParentalUnit;
}

TnodePtr TnodeReplaceMeWith(TnodePtr ThisTnode) {
    return ThisTnode->ReplaceMeWith;
}

char TnodeLetter(TnodePtr ThisTnode) {
    return ThisTnode->Letter;
}

char TnodeMaxChildDepth(TnodePtr ThisTnode) {
    return ThisTnode->MaxChildDepth;
}

char TnodeNumberOfChildren(TnodePtr ThisTnode) {
    return ThisTnode->NumberOfChildren;
}

char TnodeEndOfWordFlag(TnodePtr ThisTnode) {
    return ThisTnode->EndOfWordFlag;
}

char TnodeLevel(TnodePtr ThisTnode) {
    return ThisTnode->Level;
}

char TnodeDangling(TnodePtr ThisTnode) {
    return ThisTnode->Dangling;
}

char TnodeProtected(TnodePtr ThisTnode) {
    return ThisTnode->Protected;
}

// Allocate a "Tnode" and fill it with initial values.
TnodePtr TnodeInit(char Chap, TnodePtr OverOne, char WordEnding, char Leveler, int StarterDepth, TnodePtr Parent, char IsaChild) {
    TnodePtr Result = (TnodePtr)malloc(sizeof(Tnode));
    Result->Letter = Chap;
    Result->ArrayIndex = 0;
    Result->NumberOfChildren = 0;
    Result->MaxChildDepth = StarterDepth;
    Result->Next = OverOne;
    Result->Child = NULL;
    Result->ParentalUnit = Parent;
    Result->Dangling = false;
    Result->Protected = false;
    Result->ReplaceMeWith = NULL;
    Result->EndOfWordFlag = WordEnding;
    Result->Level = Leveler;
    Result->DirectChild = IsaChild;
    return Result;
}

// Modify internal "Tnode" member values.
void TnodeSetArrayIndex(TnodePtr ThisTnode, int TheWhat) {
    ThisTnode->ArrayIndex = TheWhat;
}

void TnodeSetChild(TnodePtr ThisTnode, TnodePtr Chump) {
    ThisTnode->Child = Chump;
}

void TnodeSetNext(TnodePtr ThisTnode, TnodePtr Nexis) {
    ThisTnode->Next = Nexis;
}

void TnodeSetParentalUnit(TnodePtr ThisTnode, TnodePtr Parent) {
    ThisTnode->ParentalUnit = Parent;
}

void TnodeSetReplaceMeWith(TnodePtr ThisTnode, TnodePtr Living) {
    ThisTnode->ReplaceMeWith = Living;
}

void TnodeSetMaxChildDepth(TnodePtr ThisTnode, int NewDepth) {
    ThisTnode->MaxChildDepth = NewDepth;
}

void TnodeSetDirectChild(TnodePtr ThisTnode, char Status) {
    ThisTnode->DirectChild = Status;
}

void TnodeFlyEndOfWordFlag(TnodePtr ThisTnode) {
    ThisTnode->EndOfWordFlag = true;
}

// This function Dangles a node, but also recursively dangles every node under it as well.
// Dangling a "Tnode" means that it will be exculded from the "DAWG".
// By recursion, nodes that are not direct children will get dangled.
// The function returns the total number of nodes dangled as a result.
int TnodeDangle(TnodePtr ThisTnode){
    if(ThisTnode->Dangling == true) {
        return 0;
    }
    int Result = 0;
    if((ThisTnode->Next) != NULL) {
        Result += TnodeDangle(ThisTnode->Next);
    }
    if((ThisTnode->Child) != NULL) {
        Result += TnodeDangle(ThisTnode->Child);
    }
    ThisTnode->Dangling = true;
    return Result + 1;
}


// This function "Protects" a node being directly referenced in the elimination process.
// "Protected" nodes can be "Dangled", but special precautions need to be taken to ensure graph-integrity.
void TnodeProtect(TnodePtr ThisTnode) {
    ThisTnode->Protected = true;
}

// This function removes "Protection" status from "ThisNode".
void TnodeUnProtect(TnodePtr ThisTnode) {
    ThisTnode->Protected = false;
}

// This function returns a boolean value indicating if a node coming after "ThisTnode" is "Protected".
// The boolean argument "CheckThisNode" determines if "ThisTnode" is included in the search.
// A "Tnode" being eliminated with "Protected" nodes beneath it requires special precautions.
bool TnodeProtectionCheck(TnodePtr ThisTnode, bool CheckThisTnode) {
    if(ThisTnode == NULL) {
        return false;
    }
    if(CheckThisTnode) {
        if(ThisTnode->Protected) {
            return true;
        }
    }
    if(TnodeProtectionCheck(ThisTnode->Next, true)) {
        return true;
    }
    if(TnodeProtectionCheck(ThisTnode->Child, true)) {
        return true;
    }
    return false;
}

// This function returns the pointer to the Tnode in a parallel list of nodes with the letter "ThisLetter", and returns NULL if the Tnode does not exist.
// If the function returns NULL, then an insertion is required.
TnodePtr TnodeFindParaNode(TnodePtr ThisTnode, char ThisLetter) {
    if(ThisTnode == NULL) {
        return NULL;
    }
    TnodePtr Result = ThisTnode;
    if(Result->Letter == ThisLetter) {
        return Result;
    }
    while(Result->Letter < ThisLetter) {
        Result = Result->Next;
        if(Result == NULL) {
            return NULL;
        }
    }
    if(Result->Letter == ThisLetter) {
        return Result;
    } else {
        return NULL;
    }
}

// This function inserts a new Tnode before a larger letter node or at the end of a para list If the list does not esist then it is put at the beginnung.
// The new node has ThisLetter in it.  AboveTnode is the node 1 level above where the new node will be placed.
// This function should never be passed a "NULL" pointer.  "ThisLetter" should never exist in the "Child" para-list.
void TnodeInsertParaNode(TnodePtr AboveTnode, char ThisLetter, char WordEnder, int StartDepth) {
    AboveTnode->NumberOfChildren++;
    TnodePtr Holder = NULL;
    TnodePtr Currently = NULL;
    // Case 1: ParaList does not exist yet so start it.
    if(AboveTnode->Child == NULL) {
        AboveTnode->Child = TnodeInit(ThisLetter, NULL, WordEnder, AboveTnode->Level + 1, StartDepth, AboveTnode, true);
    } else if(((AboveTnode->Child)->Letter) > ThisLetter) { // Case 2: ThisLetter should be the first in the ParaList.
        Holder = AboveTnode->Child;
        // The holder node is no longer a direct child so set it as such.
        TnodeSetDirectChild(Holder, false);
        AboveTnode->Child = TnodeInit(ThisLetter, Holder, WordEnder, AboveTnode->Level + 1, StartDepth, AboveTnode, true);
        // The parent node needs to be changed on what used to be the child. it is the Tnode in "Holder".
        Holder->ParentalUnit = AboveTnode->Child;
    } else if((AboveTnode->Child)->Letter < ThisLetter) { // Case 3: The ParaList exists and ThisLetter is not first in the list.
        Currently = AboveTnode->Child;
        while(Currently->Next !=NULL) {
            if(TnodeLetter(Currently->Next) > ThisLetter) {
                break;
            }
            Currently = Currently->Next;
        }
        Holder = Currently->Next;
        Currently->Next = TnodeInit(ThisLetter, Holder, WordEnder, AboveTnode->Level + 1, StartDepth, Currently, false);
        if(Holder != NULL) {
            Holder->ParentalUnit = Currently->Next;
        }
    }
}

// The "MaxChildDepth" of the two nodes cannot be assumed equal due to the recursive nature of this function, so we must check for equivalence.
char TnodeAreWeTheSame(TnodePtr FirstNode, TnodePtr SecondNode) {
    if(FirstNode == SecondNode) {
        return true;
    }
    if(FirstNode == NULL || SecondNode == NULL) {
        return false;
    }
    if(FirstNode->Letter != SecondNode->Letter) {
        return false;
    }
    if(FirstNode->MaxChildDepth != SecondNode->MaxChildDepth) {
        return false;
    }
    if(FirstNode->NumberOfChildren != SecondNode->NumberOfChildren) {
        return false;
    }
    if(FirstNode->EndOfWordFlag != SecondNode->EndOfWordFlag) {
        return false;
    }
    if(!TnodeAreWeTheSame(FirstNode->Child, SecondNode->Child)) {
        return false;
    }
    if(!TnodeAreWeTheSame(FirstNode->Next, SecondNode->Next)) {
        return false;
    }
    return true;
}

// This is a standard depth first preorder tree traversal.
// The objective is to count "Living" "Tnodes" of various "MaxChildDepths", and store these values into "Tabulator".
void TnodeGraphTabulateRecurse(TnodePtr ThisTnode, int Level, int* Tabulator) {
    if(Level == 0) {
        TnodeGraphTabulateRecurse(TnodeChild(ThisTnode), 1, Tabulator);
    } else {
        // We will only ever be concerned with "Living" nodes.  "Dangling" Nodes will be eliminated, so don't count them.
        if(!ThisTnode->Dangling) {
            Tabulator[ThisTnode->MaxChildDepth] += 1;
            // Go Down if possible.
            if(ThisTnode->Child != NULL) {
                TnodeGraphTabulateRecurse(TnodeChild(ThisTnode), Level + 1, Tabulator);
            }
            // Go Right if possible.
            if(ThisTnode->Next != NULL) {
                TnodeGraphTabulateRecurse(TnodeNext(ThisTnode), Level, Tabulator);
            }
        }
    }
}

// Recursively replaces all redundant nodes under "ThisTnode".
// "DirectChild" "Tnode" in a "Dangling" state have "ReplaceMeWith" set within them.
// Crosslinking of "Tnode"s being eliminated must be taken-care-of before this function gets called.
// When "Tnode" branches are crosslinked, the living branch has members being replaced with "Tnode"s in the branch being killed.
void TnodeLawnMowerRecurse(TnodePtr ThisTnode) {
    if(ThisTnode->Level == 0) {
        TnodeLawnMowerRecurse(ThisTnode->Child);
    } else {
        if(ThisTnode->Next == NULL && ThisTnode->Child == NULL) {
            return;
        }
        // The first "Tnode" being eliminated will always be a "DirectChild".
        if(ThisTnode->Child != NULL) {
            // The node is tagged to be "Mowed", so replace it with "ReplaceMeWith", keep following "ReplaceMeWith" until an un"Dangling" "Tnode" is found.
            if((ThisTnode->Child)->Dangling) {
                ThisTnode->Child = TnodeReplaceMeWith(ThisTnode->Child);
                while((ThisTnode->Child)->Dangling) {
                    ThisTnode->Child = TnodeReplaceMeWith(ThisTnode->Child);
                }
            } else {
                TnodeLawnMowerRecurse(ThisTnode->Child);
            }
        }
        if(ThisTnode->Next != NULL) {
            TnodeLawnMowerRecurse(ThisTnode->Next);
        }
    }
}

// This function accepts two identical node branch structures, "EliminateThis" and "ReplaceWith".  It is recursive.
// The boolean "ValidReplacement" determines if the function will check for "Crosslink"s or alter "Protected" and "ReplaceMeWith" states.
// When "ValidReplacement" is ON, it must set "ReplaceMeWith" values for "Protected" nodes under "EliminateThis".
// It will also assign "Protected" status to the corresponding nodes under "ReplaceWith".
// This function returns "false" if a "Crosslink" is found.  It returns "true" if replacement is a GO.
bool TnodeProtectAndReplaceBranchStructure(TnodePtr EliminateThis, TnodePtr ReplaceWith, bool ValidReplacement) {
    if(EliminateThis == NULL || ReplaceWith == NULL) {
        return true;
    }
    if(TnodeProtected(EliminateThis)) {
        if(TnodeDangling(ReplaceWith)) {
            // Though we verify the "Crosslink" condition for confirmation, the two conditions above guarantee the condition below.
            if(EliminateThis == TnodeReplaceMeWith(ReplaceWith)) {
                // In the case of a confirmed "Crosslink", "EliminateThis" will be a "DirectChild".
                // The logic is that "EliminateThis" and "ReplaceWith" have the same structure.
                // Since "ReplaceWith" is "Dangling" with a valid "ReplaceMeWith", it is a "DirectChild".
                // Thus "EliminateThis" must also be a "DirectChild".  Verify this.
                printf("\n   -***- Crosslink found, so exchange branches first, then Dangle the unProtected Tnodes.\n");
                // Resort to drastic measures:  Simply flip the actual nodes in the Trie.
                (ReplaceWith->ParentalUnit)->Child = EliminateThis;
                (EliminateThis->ParentalUnit)->Child = ReplaceWith;
                return false;
            }
        }
        // When "ValidReplacement" is set, if "EliminateThis" is a protected "Tnode", "ReplaceWith" isn't "Dangling".
        // We can "Dangle" "Protected" "Tnodes", as long as we set a proper "ReplaceMeWith" value for them.
        // Since we are now pointing "Tnode"s at "ReplaceWith", protect it.
        if(ValidReplacement) {
            TnodeSetReplaceMeWith(EliminateThis, ReplaceWith);
            TnodeProtect(ReplaceWith);
        }
    }
    if(!TnodeProtectAndReplaceBranchStructure(EliminateThis->Next, ReplaceWith->Next, ValidReplacement)) {
        return false;
    }
    if(TnodeProtectAndReplaceBranchStructure(EliminateThis->Child, ReplaceWith->Child, ValidReplacement)) {
        return true;
    }
    return false;
}

// This function can never be called after a trie has been "Mowed" because this will result in pointers being freed twice.
// A core dump is bad.
void FreeTnodeRecurse(TnodePtr ThisTnode) {
    if(ThisTnode->Child != NULL) {
        FreeTnodeRecurse(ThisTnode->Child);
    }
    if(ThisTnode->Next != NULL) {
        FreeTnodeRecurse(ThisTnode->Next);
    }
    free(ThisTnode);
}