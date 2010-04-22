// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#pragma once
#define GENERATED_AB 1

#include "off.h"

#define PARITY_USAGE 1

#if PARITY_USAGE
extern u4 holeParity;
#endif

///////////////////////////////////////////////////////////////////////////
// helper classes
///////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// CEmpty
// an empty square. Empties are stored in a doubly-linked list
//	using a fixed best-to-worst order taken from endgame.c
//	EmHead->next is the first element in the list
////////////////////////////////////////////////////////////////

class CEmpty;

class CEmpty {
public:
   CEmpty* prev;
   CEmpty* next;
   int square;
   TFF* AB_White, *AB_Black;
   TCF* Count_White, *Count_Black, *Terminal_White, *Terminal_Black;
#if PARITY_USAGE
   u4 holeMask;
#endif

   static void Initialize(const char *sBoard);

   void Add();
   void Remove();

   void AddParity();
   void RemoveParity();

   static void Remove(int square);
   static void Add(int square);
   static void RemoveParity(int square);
   static void AddParity(int square);
   static CEmpty* SquareToEmpty(int square);
   static int SquareToParity(int square);	// returns 1<<quadrant if true

protected:
   static int orderToSquare[NN];	// best-to-worst order
   static CEmpty* squareToEmpty[NN];
};

inline CEmpty* CEmpty::SquareToEmpty(int square) {
   return squareToEmpty[square];
}

inline int CEmpty::SquareToParity(int square) {
   return SquareToEmpty(square)->holeMask & holeParity;
}

extern CEmpty emptyHead, empties[NN];
inline void CEmpty::Remove(int square) {squareToEmpty[square]->Remove();};
inline void CEmpty::Add(int square) {squareToEmpty[square]->Add();};
inline void CEmpty::RemoveParity(int square) {squareToEmpty[square]->RemoveParity();};
inline void CEmpty::AddParity(int square) {squareToEmpty[square]->AddParity();};

//if CIRCULAR_EMPTIES is defined, the empty list is circular with the last node pointing back to the head.
//	This saves us 2 if statements per node
#define CIRCULAR_EMPTIES 1

#if CIRCULAR_EMPTIES
CEmpty* const emEOL=&emptyHead;
#else
CEmpty* const emEOL=0;
#endif

////////////////////////////////////////////////////////////////
// CEmptyValue - stores a pointer to an empty and a value
//	used in move sorting
////////////////////////////////////////////////////////////////

class CEmptyValue {
public:
   CEmpty* pem;
   CValue value;
};

// note reversed sense of '<' so that the sort ends with the highest values
//	first
inline bool operator<(const CEmptyValue& a, const CEmptyValue&b) {
   return a.value>b.value;
}
