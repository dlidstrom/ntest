// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// moves source file

#include "PreCompile.h"
#include "Debug.h"
#include "Moves.h"
#include "GDK/OsObjects.h"
#include "BitBoard.h"
#include "options.h"

#include <algorithm>
#include <iomanip>

//////////////////////////////////////
// CMove class
//////////////////////////////////////

CMove::CMove(const string& mv) {
   Set(mv.c_str());
}

void CMove::Out(ostream& os) const {
   if (Row()>=0 && Row()<N)
      os << (char)(Col()+'A') << (char)(Row()+'1');
   else if (IsPass())
      os << "pa";
   else
      os << "??";
}

CMove::operator string() const {
   ostrstream os;
   Out(os);
   os << '\0';
   string sResult(os.str());
   os.rdbuf()->freeze(0);
   return sResult;
}

void CMove::Set(const char* text) {
   char items[2];

   if (strlen(text)<2) {
      QSSERT(0);
      Set(-2);
      return;
   }

   items[0]=tolower(text[0]);
   items[1]=tolower(text[1]);

   // Pass? then set to -1
   if (!strncmp(items,"pa",2)) {
      Set(-1);
   }
   else {
      CSGSquare sq(text);

      // valid square?
      if (sq.x>=0 && sq.x<N && sq.y>=0 && sq.y<N) {
         Set(sq.x, sq.y);
      }

      // any other value? set to -2.
      else {
         Set(-2);
         QSSERT(0);
      }
   }
}

//////////////////////////////////////
// CMoveValue class
//////////////////////////////////////

void CMoveValue::Out(ostream& os) const {
   os << move << ":" << value;
}

ostream& operator<<(ostream& os, const vector<CMoveValue>& mvs) {
   vector<CMoveValue>::const_iterator i;
   for (i=mvs.begin(); i!=mvs.end(); i++) {
      if (i!=mvs.begin())
         os << "\t";
      os << *i;
   }
   return os;
}

//////////////////////////////////////
// CMVK class
//////////////////////////////////////

void CMVK::FPrint(FILE* fp) const {
   fprintf(fp, "%s",string(move).c_str());
   if (fKnown)
      fprintf(fp, " [%hd]", value);
   else
      fprintf(fp, " [?]");
   fprintf(fp, " (%d:%d ply", hiFull.height, hiFull.iPrune);
   if (hiFull.fWLD)
      fprintf(fp, " WLD");
   if (hiFull!=hiBest) {
      fprintf(fp, ", best to %d:%d ply", hiBest.height, hiBest.iPrune);
      if (hiBest.fWLD)
         fprintf(fp, " WLD");
   }
   if (fBook)
      fprintf(fp, ", book");
   fprintf(fp, ")");
}

ostream& CMVK::Out(ostream& os) const {
   os << move << " [" ;
   os.width(5);
   if (fKnown)
      os << value;
   else
      os << "?";
   os << "] (" << hiFull;
   if (hiFull!=hiBest)
      os<<", best to " << hiBest;
   if (fBook)
      os<<", book";
   os << ") ";

   ns.OutShort(os);
   return os;
}
//////////////////////////////////////
// CMoves class
//////////////////////////////////////

void CMoves::Set(u8 aBlock) {
   all=aBlock;
   moveToCheck=kCorner;
}

void CMoves::Set(const i8& aBlock) {
   all.i8s=aBlock;
   moveToCheck=kCorner;
}

bool CMoves::IsValid(const CMove& move) const {
   return (move.Mask() & all.u1s[move.Row()])?true:false;
}

bool CMoves::Consistent() const {
   switch(moveToCheck) {
   case kBest:
   case kCorner:
   case kRegular:
   case kCX:
      return true;
   default:
      return false;
   }
}


bool CMoves::operator==(const CMoves& b) const {
   return all==b.all;
}

void CMoves::SetBest(const CMove& aBestMove) {
   bestMove=aBestMove;

   // if row<0, both rows should be negative. return
   if (bestMove.Row()<0)
      return;

   // check best move
   moveToCheck=kBest;
}

bool CMoves::HasBest() const {
   return moveToCheck==kBest;
}

// CMoves::GetNext
// get the next valid move from the list of moves
// delete the returned move from the list
// return true if there is an ungenerated valid move

bool CMoves::GetNext(CMove& move) {
   u8 availableMoves;

   // all case statements fall through
   switch(moveToCheck) {
   case kBest:		// the best move from the transposition table
      if (bestMove.Valid()) {
         move=bestMove;
         moveToCheck=kCorner;
         QSSERT(move.Row()<8);
         break;
      }
   case kCorner:	// check corners before other squares
      availableMoves[0]=all[0] & 0x81;
      availableMoves[1]=all[1] & 0x81000000;
      if (availableMoves[0] | availableMoves[1]) {
         FindMove(availableMoves, move);
         QSSERT(move.Row()<8);
         break;
      }
      moveToCheck=kRegular;
   case kRegular:	// check regular squares before C&X-squares
      availableMoves[0]=all[0] & 0xFFFF3C3C;
      availableMoves[1]=all[1] & 0x3C3CFFFF;
      if (availableMoves[0] | availableMoves[1]) {
         FindMove(availableMoves, move);
         QSSERT(move.Row()<8);
         break;
      }
      moveToCheck=kCX;
   case kCX:		// check C and X squares
      availableMoves=all;	// no mask needed since we should have only C and X squares now
      if (availableMoves[0] | availableMoves[1]) {
         FindMove(availableMoves, move);
         QSSERT(move.Row()<8);
         break;
      }
      return false;
   default:
      QSSERT(0);
      return false;
   }
   all.u1s[move.Row()]^=move.Mask();
   return true;
}

void CMoves::FindMove(u8 availableMoves, CMove& move) {
   u4 mask4;
   u1 square;

   QSSERT(availableMoves[0] | availableMoves[1]);

   // find a row with stuff in it
   if (availableMoves[0]) {
      square=0;
      mask4=availableMoves[0];
   }
   else {
      square=32;
      mask4=availableMoves[1];
   }

   if ((mask4&((1<<16)-1))==0) {
      square+=16;
      mask4>>=16;
   }
   if ((mask4&((1<<8)-1))==0) {
      square+=8;
      mask4>>=8;
   }
   if ((mask4&((1<<4)-1))==0) {
      square+=4;
      mask4>>=4;
   }
   if ((mask4&((1<<2)-1))==0) {
      square+=2;
      mask4>>=2;
   }
   if ((mask4&((1<<1)-1))==0) {
      square+=1;
      mask4>>=1;
   }

   move.Set(square);
}

void CMoves::Delete(const CMove& move) {
   QSSERT(all.u1s[move.Row()]&move.Mask());
   all.u1s[move.Row()]&=~move.Mask();
}
