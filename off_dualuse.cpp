// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "off.h"
#include "SV.h"
#include "Debug.h"

int lengthToSize(int length) {
   int size;

   if (length==0)
      size=0;
   else {
      for (size=1; length>0; length--)
         size+=size+size;
   }

   return size;
}

void ConfigToTrits(int config, int length, int* trits) {
   int i;

   for (i=0; i<length; i++) {
      trits[i]=config%3;
      config/=3;
   }
}

int NFlipped(int config, int length, int loc, bool fMover, int& left, int& right) {
   int trits[8];
   int iMover=fMover+fMover, iOpp=2-iMover;
   int i;

   left=right=0;
   ConfigToTrits(config, length, trits);
   if (trits[loc]!=1)		// square already filled
      return 1;
   // left
   if (loc>1 && trits[loc-1]==iOpp) {
      for (i=loc-2; i>0 && trits[i]==iOpp; i--);
      if (trits[i]==iMover)
         left=loc-i-1;
   }
   // right
   if (loc<length-2 && trits[loc+1]==iOpp) {
      for (i=loc+2; i<length-1 && trits[i]==iOpp; i++);
      if (trits[i]==iMover)
         right=i-loc-1;
   }
   return 0;
}

int DummyFlipFunction(const CHAB& hab) {
   _ASSERT(0);
   return 0;
}

void InitTable(TFF** table, TFF* tableInit[7][7], int loc, int length, bool fMover) {
   int size=lengthToSize(length);
   int config, nDown, nUp;

   for (config=0; config<size; config++) {
      if (NFlipped(config, length, loc, fMover, nDown, nUp))
         table[config]=DummyFlipFunction;
      else {
         _ASSERT(tableInit[nDown][nUp]!=DummyFlipFunction);
         table[config]=tableInit[nDown][nUp];
      }
   }
}

///////////////////////////////////////////
// incremental update of all patterns
///////////////////////////////////////////

CSVs ffk("FFK", "CHABM", CSVs::kBitboard);

// given an array of square ids, add the pattern and all its reflections into the list
void SetPattern(int nSquares, u1* squares, const char* sPatternName, u4 flags) {
   ffk.push_back(CSV(nSquares, squares, sPatternName, flags));
}

u1 row1[]={0,1,2,3,4,5,6,7};
u1 row2[]={8,9,10,11,12,13,14,15};
u1 row3[]={16,17,18,19,20,21,22,23};
u1 row4[]={24,25,26,27,28,29,30,31};
u1 diag8[]={0,9,18,27,36,45,54,63};
u1 diag7[]={1,10,19,28,37,46,55};
u1 diag6[]={2,11,20,29,38,47};
u1 diag5[]={3,12,21,30,39};
u1 diag4[]={4,13,22,31};
u1 diag3[]={5,14,23};
//u1 triangle[]={0,1,2,3,8,9,10,16,17,24};
//u1 c2x4[]={0,1,2,3,8,9,10,11};
//u1 c2x5[]={0,1,2,3,4,8,9,10,11,12};
u1 triangle[]={000, 011, 030, 020, 010, 021, 012, 001, 002, 003};	// octal, and in a strange order to match J eval
u1 c2x4[]={8,9,10,11,0,1,2,3};	// strange order to match J eval
u1 c2x5[]={8,9,10,11,12,0,1,2,3,4};	// strange order to match J eval

u1 r1xx[]={9,0,1,2,3,4,5,6,7,14};
u1 c3x3[]={0,1,2,8,9,10,16,17,18};

// incremental update patterns
void CreatePatternsJ() {
   SetPattern(sizeof(row1), row1, "row1", CSV::kDir);
   SetPattern(sizeof(row2), row2, "row2", CSV::kDir + CSV::kEval);
   SetPattern(sizeof(row3), row3, "row3", CSV::kDir + CSV::kEval);
   SetPattern(sizeof(row4), row4, "row4", CSV::kDir + CSV::kEval);
   SetPattern(sizeof(diag8), diag8, "diag8", CSV::kDir + CSV::kEval);
   SetPattern(sizeof(diag7), diag7, "diag7", CSV::kDir + CSV::kEval);
   SetPattern(sizeof(diag6), diag6, "diag6", CSV::kDir + CSV::kEval);
   SetPattern(sizeof(diag5), diag5, "diag5", CSV::kDir + CSV::kEval);
   SetPattern(sizeof(diag4), diag4, "diag4", CSV::kDir);
   SetPattern(sizeof(diag3), diag3, "diag3", CSV::kDir);
   SetPattern(sizeof(triangle), triangle, "triangle",CSV::kEval);
   SetPattern(sizeof(c2x4), c2x4, "c2x4", 0);
   SetPattern(sizeof(c2x5), c2x5, "c2x5",CSV::kEval);
   SetPattern(sizeof(r1xx), r1xx, "r1xx",CSV::kEval);
}
