// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "Pos2All.h"
#include "off.h"

///////////////////////////////////////////////////////////////////////////////
// CEmptyList
///////////////////////////////////////////////////////////////////////////////

CEmpty emptyHead, empties[64];

// fixed preference ordering of squares for endgame solver.
//	best to worst order. Carped from endgame.c. Note constants are octal.
int CEmpty::orderToSquare[NN]= {
   /*A1*/      000, 007, 070, 077,
   /*C1*/      002, 005, 020, 027, 050, 057, 072, 075,
   /*C3*/      022, 025, 052, 055,
   /*D1*/      003, 004, 030, 037, 040, 047, 073, 074,
   /*D3*/      023, 024, 032, 035, 042, 045, 053, 054,
   /*D2*/      013, 014, 031, 036, 041, 046, 063, 064,
   /*C2*/      012, 015, 021, 026, 051, 056, 062, 065,
   /*B1*/      001, 006, 010, 017, 060, 067, 071, 076,
   /*B2*/      011, 016, 061, 066,
   /*D4*/      033, 034, 043, 044,
};

CEmpty* CEmpty::squareToEmpty[NN];

void CEmpty::Initialize(const char *sBoard) {
   CEmpty* emLast=&emptyHead, *pem;
   int order, square, value;
#if PARITY_USAGE
   int quarter;
#endif

#if PARITY_USAGE
   holeParity=0;
#endif

   // initialize the list in order from best to worst
   for(order=0, pem=empties;order<NN; order++, pem++) {
      square=orderToSquare[order];
      value=TextToValue(sBoard[square]);
      if (value==EMPTY) {
         emLast->next=pem;
         pem->prev=emLast;
         pem->square=square;
         squareToEmpty[square]=pem;
         QSSERT(squareToEmpty[square]->square==square);
         InitEmpties();
#if PARITY_USAGE
         quarter=(Row(square)<=N/2);
         quarter=quarter+quarter+(Col(square)<=N/2);
         pem->holeMask=1<<quarter;
         holeParity^=1<<quarter;
#endif
         emLast=pem;
      }
   }
   emLast->next=emEOL;
   emptyHead.prev=emLast;
   //QSSERT(emptyHead.next!=NULL);
}
