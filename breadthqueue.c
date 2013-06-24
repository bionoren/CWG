//
//  breadthqueue.c
//  wwfmax
//
//  Created by Bion Oren on 11/10/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include "breadthqueue.h"
#include "dawg.h"

void BreadthQueueNodeSetNext(BreadthQueueNodePtr ThisBreadthQueueNode, BreadthQueueNodePtr Nexit) {
    ThisBreadthQueueNode->Next = Nexit;
}

BreadthQueueNodePtr BreadthQueueNodeNext(BreadthQueueNodePtr ThisBreadthQueueNode) {
    return ThisBreadthQueueNode->Next;
}

TnodePtr BreadthQueueNodeElement(BreadthQueueNodePtr ThisBreadthQueueNode) {
    return ThisBreadthQueueNode->Element;
}

BreadthQueueNodePtr BreadthQueueNodeInit(TnodePtr NewElement) {
	BreadthQueueNodePtr Result = (BreadthQueueNode *)malloc(sizeof(BreadthQueueNode));
	Result->Element = NewElement;
	Result->Next = NULL;
	return Result;
}

BreadthQueuePtr BreadthQueueInit(void) {
	BreadthQueuePtr Result = (BreadthQueue *)malloc(sizeof(BreadthQueue));
	Result->Front = NULL;
	Result->Back = NULL;
	Result->Size = 0;
    return Result;
}

void BreadthQueuePush(BreadthQueuePtr ThisBreadthQueue, TnodePtr NewElemental) {
	BreadthQueueNodePtr Noob = BreadthQueueNodeInit(NewElemental);
	if((ThisBreadthQueue->Back) != NULL) {
        BreadthQueueNodeSetNext(ThisBreadthQueue->Back, Noob);
    } else {
        ThisBreadthQueue->Front = Noob;
    }
	ThisBreadthQueue->Back = Noob;
	ThisBreadthQueue->Size += 1;
}

TnodePtr BreadthQueuePop(BreadthQueuePtr ThisBreadthQueue) {
	if(ThisBreadthQueue->Size == 0) {
        return NULL;
    }
	if(ThisBreadthQueue->Size == 1) {
		ThisBreadthQueue->Back = NULL;
		ThisBreadthQueue->Size = 0;
		TnodePtr Result = (ThisBreadthQueue->Front)->Element;
		free(ThisBreadthQueue->Front);
		ThisBreadthQueue->Front = NULL;
		return Result;
	}
	TnodePtr Result = (ThisBreadthQueue->Front)->Element;
	BreadthQueueNodePtr Holder = ThisBreadthQueue->Front;
	ThisBreadthQueue->Front = (ThisBreadthQueue->Front)->Next;
	free(Holder);
	ThisBreadthQueue->Size -= 1;
	return Result;
}


// For the "Tnode" "Dangling" process, arrange the "Tnodes" in the "Holder" array, with breadth-first traversal order.
void BreadthQueuePopulateReductionArray(BreadthQueuePtr ThisBreadthQueue, TnodePtr Root, TnodePtr **Holder) {
	int Taker[MAX];
	memset(Taker, 0, MAX*sizeof(int));
	TnodePtr Current = Root;
	// Push the first row onto the queue.
	while(Current != NULL) {
		BreadthQueuePush(ThisBreadthQueue, Current);
		Current = Current->Next;
	}
	// Initiate the pop followed by push all children loop.
	while((ThisBreadthQueue->Size) != 0) {
		Current = BreadthQueuePop(ThisBreadthQueue);
		int CurrentMaxChildDepth = Current->MaxChildDepth;
		Holder[CurrentMaxChildDepth][Taker[CurrentMaxChildDepth]] = Current;
		Taker[CurrentMaxChildDepth] += 1;
		Current = TnodeChild(Current);
		while(Current != NULL) {
			BreadthQueuePush(ThisBreadthQueue, Current);
			Current = TnodeNext(Current);
		}
	}
}


// It is of absolutely critical importance that only "DirectChild" nodes are pushed onto the queue as child nodes.  This will not always be the case.
// In a DAWG a child pointer may point to an internal node in a longer list.  Check for this.
int BreadthQueueUseToIndex(BreadthQueuePtr ThisBreadthQueue, TnodePtr Root) {
	int IndexNow = 0;
	TnodePtr Current = Root;
	// Push the first row onto the queue.
	while(Current != NULL) {
		BreadthQueuePush(ThisBreadthQueue, Current);
		Current = Current->Next;
	}
	// Pop each element off of the queue and only push its children if has not been "Dangled" yet.  Assign index if one has not been given to it yet.
	while((ThisBreadthQueue->Size) != 0) {
		Current = BreadthQueuePop(ThisBreadthQueue);
		// A traversal of the Trie will never land on "Dangling" "Tnodes", but it will try to visit certain "Tnodes" many times.
		if(TnodeArrayIndex(Current) == 0) {
			IndexNow += 1;
			TnodeSetArrayIndex(Current, IndexNow);
			Current = TnodeChild(Current);
			if(Current != NULL) {
                // The graph will lead to intermediate positions, but we cannot start numbering "Tnodes" from the middle of a list.
                if(TnodeDirectChild(Current) && TnodeArrayIndex(Current) == 0) {
                    while(Current != NULL) {
                        BreadthQueuePush(ThisBreadthQueue, Current);
                        Current = Current->Next;
                    }
                }
			}
		}
	}
	return IndexNow;
}