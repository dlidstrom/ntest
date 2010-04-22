// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#pragma once


#define INLINE_ABIFPARITY 0

extern int hSolveNoParity;

#include "FastFlip.h"

#define CHECK_VALUES 0

#include "Pos2Internal.h"
#include "Pos2Helpers.h"
#if CHECK_VALUES
#include "endgame.h"
#endif

#include "Debug.h"

/////////////////////////////////
// inline functions
////////////////////////////////

inline int imax(int a, int b) { return a>b?a:b; }

#if CHECK_VALUES
static char sBoardCheck[NN+1];
#endif

// find out if a square is valid. quicker than calling count?
// returns 0 if not a valid move, 1 if a valid move
inline int ValidMove(bool fBlackMove, int square) {
   CEmpty* pem=CEmpty::SquareToEmpty(square);
   if(fBlackMove)
      return pem->Count_Black();
   else
      return pem->Count_White();
}

inline void PassBase() {
   fBlackMove_=!fBlackMove_;
   nDiscDiff_=-nDiscDiff_;
}

inline void UndoPassBase() {
   fBlackMove_=!fBlackMove_;
   nDiscDiff_=-nDiscDiff_;
}

inline void PassBB() {
   fBlackMove_=!fBlackMove_;
   nDiscDiff_=-nDiscDiff_;
   bb.InvertColors();
}

inline void UndoPassBB() {
   fBlackMove_=!fBlackMove_;
   nDiscDiff_=-nDiscDiff_;
   bb.InvertColors();
}
