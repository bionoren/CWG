//
//  arraydawg.h
//  wwfmax
//
//  Created by Bion Oren on 11/10/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#ifndef wwfmax_arraydawg_h
#define wwfmax_arraydawg_h

#include "tnode.h"

#define BIT_COUNT_FOR_CHILD_INDEX 17

// This program will create "5" binary-data files for use, and "1" text-data file for inspection.
#define TRADITIONAL_DAWG_DATA "Traditional_Dawg_For_Word-List.dat"
#define TRADITIONAL_DAWG_TEXT_DATA "Traditional_Dawg_Text_For_Word-List.txt"

#define DIRECT_GRAPH_DATA_PART_ONE "Part_One_Direct_Graph_For_Word-List.dat"
#define DIRECT_GRAPH_DATA_PART_TWO "Part_Two_Direct_Graph_For_Word-List.dat"
#define FINAL_NODES_DATA "Final_Nodes_For_Word-List.dat"

// Next and Child become indices.
struct arraydnode{
    int Next;
    int Child;
    char Letter;
    char EndOfWordFlag;
    char Level;
};

typedef struct arraydnode ArrayDnode;
typedef ArrayDnode* ArrayDnodePtr;

struct arraydawg {
    int NumberOfStrings;
    ArrayDnodePtr DawgArray;
    int First;
    char MinStringLength;
    char MaxStringLength;
};

typedef struct arraydawg ArrayDawg;
typedef ArrayDawg* ArrayDawgPtr;

void ConvertIntNodeToBinaryString(int TheNode, char *BinaryNode);
void ConvertChildListIntToBinaryString(int TheChildList, char *BinaryChildList);

void ConvertIntNodeToBinaryString(int TheNode, char *BinaryNode);
void ArrayDnodeInit(ArrayDnodePtr ThisArrayDnode, char Chap, int Nextt, int Childd, char EndingFlag, char Breadth);
void ArrayDnodeTnodeTranspose(ArrayDnodePtr ThisArrayDnode, TnodePtr ThisTnode);
int ArrayDnodeNumberOfChildrenPlusString(ArrayDnodePtr DoggieDog, int Index, char* FillThisString);
ArrayDawgPtr ArrayDawgInit(char **Dictionary, int *SegmentLenghts, int MaxStringLength);

#endif