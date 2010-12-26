// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// Moves header file

#pragma once

#ifndef _H_MOVES	//MSC sucks
#define _H_MOVES

#include "Utils.h"
#include "BitBoardBlock.h"
#include "NodeStats.h"

#include <vector>

using namespace std;

class CBoard;
class CPosition;

typedef char TMoveCode;
typedef char TMoveList[2*NN+1];

//////////////////////////////////////
// CMove class
//////////////////////////////////////

class CMove {
public:
   CMove();
   CMove(const string& mv);
   CMove(int sq);

   operator string() const;

   int	Row() const;
   int	Col() const;
   int	Mask() const;
   int	Square() const;
   void Out(std::ostream& os) const;
   void Set(int x, int y);
   void Set(u1 square);
   void Set(const char* text);
   bool Valid() const;	// true if square btw 0 and 63
   bool IsPass() const; // true if row<0;

   bool operator==(const CMove& b) const;
   bool operator!=(const CMove& b) const;

   bool operator<(const CMove& right) const;

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      archive & square;
   }

private:
   u1 square;
};
inline std::ostream& operator<<(std::ostream& os, const CMove& move) { move.Out(os); return os; }

inline CMove::CMove() {};
inline CMove::CMove(int sq) { square=sq; };
inline bool CMove::operator==(const CMove& b) const {return square==b.square;}
inline bool CMove::operator!=(const CMove& b) const {return square!=b.square;}
inline bool CMove::operator<(const CMove& right) const {return square<right.square;}
inline int CMove::Row() const { return square>>3;}
inline int CMove::Col() const { return square&7;}
inline int CMove::Mask() const { return 1<<Col();}
inline int CMove::Square() const { return square; }
inline void CMove::Set(u1 asquare) { square=asquare;  }
inline void CMove::Set(int x, int y) { square=x+(y<<3);  }
inline bool CMove::Valid() const {return square<NN;}
inline bool CMove::IsPass() const { return square==(u1)-1; }

//////////////////////////////////////
// CMoveValue and CMVK classes
//////////////////////////////////////

class CMoveValue {
public:
   CMove move;
   CValue value;

   void Out(std::ostream& os) const;
};

inline std::ostream& operator<<(std::ostream& os, const CMoveValue& mv) {mv.Out(os); return os;};
std::ostream& operator<<(std::ostream& os, const vector<CMoveValue>& mvs);

class CMVK : public CMoveValue {
public:
   void FPrint(FILE* fp) const;

   void Clear();
   std::ostream& Out(std::ostream& os) const;

   CHeightInfo hiFull, hiBest;
   bool fBook;		// true if this move comes from the book
   bool fKnown;	// true if the value has been evaluated
   CNodeStats ns;
};

inline std::ostream& operator<<(std::ostream& os, const CMVK& mvk) { return mvk.Out(os);}

// note this operator is the reverse of what you think it should be!!!
inline bool operator<(const CMoveValue& a, const CMoveValue& b) {
   return a.value>b.value;
}

inline bool operator==(const CMoveValue& a, const CMoveValue& b) {
   return a.value==b.value;
}

inline void CMVK::Clear() {
   hiFull.Clear();
   hiBest.Clear();
   fBook=false;
   fKnown=false;
   move.Set((u1)-1);
}

//////////////////////////////////////
// CMoves class
//////////////////////////////////////
class CMoves {
public:
   CMoves() {}
   bool GetNext(CMove& move);				// get next move; return false if none.
   bool IsValid(const CMove& move) const;	// returns true if move is valid. This routine
   //	does not work if GetNext or SetBest has been called.
   int  NMoves() const;				// return the number of available moves, so long as
   // GetNext and SetBest have not been called.
   bool HasBest() const;				// return true if best move is set and unused
   void Delete(const CMove& move);		// remove a move from the move list.
   //	Doesn't work if SetBest() has been called.
   bool HasMoves() const;				// True if there are still moves
   void Set(u8 aBlock);				// set moves to this bitboardblock
   void Set(const i8& aBlock);				// set moves to this bitboardblock
   void SetBest(const CMove& aBestMove); // set the best move

   bool Consistent()const;

   bool operator==(const CMoves& b) const;

   CBitBoardBlock GetRemainingMoves() const { return all;} ;

private:
   CBitBoardBlock all;			// true if have a move (unless the move is in bestMove)
   CMove bestMove;
   enum { kBest, kCorner, kRegular, kCX } moveToCheck;

   void FindMove(u8 availableMoves, CMove& move);

   friend class CBitBoard;
   friend class CQPosition;
   friend class CPosition;
   friend class CPositionG;
};

inline int CMoves::NMoves() const {
   return CountBits(all);
}

inline bool CMoves::HasMoves() const { return (all.u4s[0] | all.u4s[1])!=0; }

class CMoveValueMoves:public CMoveValue {
public:
   CMoves submoves;
};

#endif // _H_Moves
