// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// Quick position header file

// Position and Moves and Move class header file

#pragma once

#include "Utils.h"
#include "Moves.h"
#include "Evaluator.h"
#include "BitBoard.h"

#include <vector>

class COsGame;
class CEvaluator;
class COsBoard;

//////////////////////////////////////
// CQPosition class
//////////////////////////////////////

class CQPosition {
public:
   CQPosition();
   CQPosition(const CBitBoard& aBoard, bool blackMove);
   CQPosition(const char* cstrBoard, bool blackMove);
   CQPosition(const COsGame& game, std::size_t iMove);
   CQPosition(const COsBoard& board);

   // Initialization
   void Initialize();
   void Initialize(const char* cstrBoard, bool blackMove);
   void Initialize(const CBitBoard& aNormalizedBoard, bool blackMove);
   bool SetBlackMove(bool fNewBlackMove);
   bool SetRow(const char* sRow, int row);

   // Moving
   bool CalcMoves(CMoves& moves) const;
   void MakeMove(CMove move);
   void Pass();	// flip the board and change the color of the mover
   int CalcMovesAndPass(CMoves& moves); // calc moves, then pass if a pass is needed

   // Other stuff
   bool PlayMove(TMoveCode mc, int passCode=1);		// update the board with the given move. Return false if can't move
   bool PlayMoves(const TMoveCode* moveCodes, int nMoves, int passCode=1);	// update the board with the given moves.
   // return false if a move is invalid or game over before we run
   // out of moves

   // turning it back into a string
   char* GetSBoard(char sBoard[NN+1]);

   // Printing
   void Print(bool fBoardOnly=false) const;
   void FPrint(FILE* fp, bool fBoardOnly=false) const;

   // Values of interest
   bool ObviouslyDone() const;		// return true if wipeoout or no empty squares
   bool BlackMove() const;			// return true if it's black's move
   int NEmpty() const;				// return number of empty squares
   int	NetSquares() const;			// return net number of squares for black
   CValue TerminalValue() const;	// value at end of game
   bool IsTerminal();				// true if game is over
   const CBitBoard& BitBoard() const;		// return a bitboard representation of the position, normalized with black to move
   void NDiscs(int& nBlack, int& nWhite, int& nEmpty) const;
   bool IsSuccessor(const CQPosition& posSuccessor) const;

   // other evaluation
   u2 FrontierLength(bool playerToMove) const;		// return number of vacant squares next to opponent's squares
   u2 Mobility(bool playerToMove) const;				// number of valid moves
   int CalcMobility(u4& nMovesPlayer, u4& nMovesOpponent) const;	// calc mobs & return pass

   // evaluation function
   static CEvaluator* evaluator;

   bool operator==(const CQPosition& pos2) const;

protected:
   CBitBoard board; // normalized such that black is to move
   int nEmpty, nBlack;
   bool fBlackMove;

   void FlipBoard();				// switch white to black and black to white. Do NOT change the color of the mover.
   int NBlack() const;				// return number of black squares
};

inline CQPosition::CQPosition() {};
inline bool CQPosition::BlackMove() const { return fBlackMove; }
inline int  CQPosition::NEmpty() const { return nEmpty; }
inline const CBitBoard& CQPosition::BitBoard() const { return board;};
inline void CQPosition::NDiscs(int& nBlack, int& nWhite, int& nEmpty) const {
   board.NDiscs(fBlackMove, nBlack, nWhite, nEmpty);
}
inline bool CQPosition::CalcMoves(CMoves& moves) const { return board.CalcMoves(moves); };
inline bool CQPosition::operator==(const CQPosition& pos2) const { return board==pos2.board; };
