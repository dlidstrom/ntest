// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// board header file

#pragma once

#ifndef _H_BitBoard // MSC sucks
#define _H_BitBoard

#include "BitBoardBlock.h"
class CMoves;

class CBitBoard {
public:
   CBitBoardBlock empty, mover;

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      archive & empty;
      archive & mover;
   }

   // Set to starting position
   void Initialize();
   void Initialize(const char* boardText, bool fBlackMove=true);
   void SetRow(const char* sRow, int row);

   // flipping
   void FlipHorizontal();
   void FlipVertical();
   void FlipDiagonal();
   CBitBoard Symmetry(int sym) const;
   CBitBoard MinimalReflection() const;

   void InvertColors();

   // impossible board
   void Impossible();

   // I/O
   void Print(bool fBlackMove) const;// Print the board in human-readable format
   void FPrint(FILE* fp, bool blackMove) const;// Print the board in human-readable format
   bool Write(FILE* fp) const;	// print in bitstream format
   bool Read(FILE* fp);		// read in bitstream format; return true if successful
   char* GetSBoardNonnormalized(char sBoard[NN+1]) const;		// turning it back into a string
   char* GetSBoardNormalized(char sBoard[NN+1], bool fBlackMove) const;		// turning it back into a string

   // statistics
   u4 Hash() const;
   int NEmpty() const;
   void NDiscs(bool fBlackMove, int& nBlack, int& nWhite, int& nEmpty) const;
   int TerminalValue() const;

   // moving
   bool CalcMoves(CMoves& moves) const;
   int CalcMobility(u4& nMovesPlayer, u4& nMovesOpponent) const;
#ifdef OLD
   int CalcMobility(u4& nMovesPlayer, u4& nMovesOpponent, CMoves& moves) const;
#endif
   bool operator==(const CBitBoard& b) const;
   bool operator!=(const CBitBoard& b) const;

   static void Test();

protected:
   void FPrintHeader(FILE* fp) const;
};

bool operator<(const CBitBoard& a, const CBitBoard& b);

std::size_t hash_value(const CBitBoard& b);

inline void CBitBoard::Impossible() {
   mover[0]=mover[1]=empty[0]=empty[1]=(u4)-1;
}

inline bool CBitBoard::operator==(const CBitBoard& b) const { return mover==b.mover && empty==b.empty;};
inline bool CBitBoard::operator!=(const CBitBoard& b) const { return mover!=b.mover || empty!=b.empty;};
inline void CBitBoard::FlipVertical() { empty.FlipVertical(); mover.FlipVertical();}
inline void CBitBoard::FlipHorizontal() { empty.FlipHorizontal(); mover.FlipHorizontal();}
inline void CBitBoard::FlipDiagonal() { empty.FlipDiagonal(); mover.FlipDiagonal();}
inline void CBitBoard::InvertColors() { mover^=~empty; }
#endif // defined _H_BitBoard
