// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// CQPosition source code

#include "PreCompile.h"
#include "QPosition.h"
#include "Evaluator.h"
#include "options.h"
#include "TreeDebug.h"
#include "Cache.h"
#include "Book.h"
#include "NodeStats.h"
#include "Debug.h"
#include <algorithm>
#include "Pos2.h"

using namespace std;

#include "GDK/OsObjects.h"

////////////////////////////////////////////////
// CQPosition class
////////////////////////////////////////////////

CEvaluator* CQPosition::evaluator;

CQPosition::CQPosition(const CBitBoard& aBoard, bool blackMove) {
   Initialize(aBoard, blackMove);
}

CQPosition::CQPosition(const char* cstrBoard, bool blackMove) {
   Initialize(cstrBoard, blackMove);
}

CQPosition::CQPosition(const COsGame& game, std::size_t iMove) {
   char sBoard[NN+1];
   int iMover;

   game.posStart.board.TextGet(sBoard, iMover);
   Initialize(sBoard, !iMover);
   for (std::size_t i=0; i<iMove && i<game.ml.size(); i++) {
      CMove mv(game.ml[i].mv);
      MakeMove(mv);
   }
}

CQPosition::CQPosition(const COsBoard& osboard) {
   char sBoard[NN+1];
   int iMover;

   osboard.TextGet(sBoard, iMover);
   Initialize(sBoard, !iMover);
}

void CQPosition::Initialize() {
   board.empty.u4s[0]=0xE7FFFFFF;
   board.mover.u4s[0]=0x10000000;
   board.empty.u4s[1]=0xFFFFFFE7;
   board.mover.u4s[1]=0x00000008;
   nEmpty=60;
   nBlack=2;
   fBlackMove=true;
}

void CQPosition::Initialize(const char* cstrBoard, bool blackMove) {
   board.Initialize(cstrBoard);
   if (!blackMove)
      board.InvertColors();
   fBlackMove=blackMove;
   nEmpty=CountBits(board.empty);
   nBlack=CountBits(board.mover);
}

// Initialize a position with a bitboard. aBoard is assumed to be
//	NORMALIZED such that the player to move is black.
void CQPosition::Initialize(const CBitBoard& aBoard, bool blackMove) {
   board.empty=aBoard.empty;
   board.mover=aBoard.mover;
   fBlackMove=blackMove;
   nEmpty=CountBits(board.empty);
   nBlack=CountBits(board.mover);
}

// return the output board string
char* CQPosition::GetSBoard(char sBoard[NN+1]) {
   return board.GetSBoardNormalized(sBoard, fBlackMove);
}

bool CQPosition::SetBlackMove(bool fNewBlackMove) {
   bool fResult=(fNewBlackMove!=fBlackMove);
   if (fResult) {
      board.InvertColors();
      fBlackMove=fNewBlackMove;
   }
   return fResult;
}

// set a row for cheat purposes (or for rand games)
bool CQPosition::SetRow(const char* sRow, int row) {
   if (strlen(sRow)<N) {
      printf("Please enter a complete row\n");
      return false;
   }
   else {
      char sBoard[NN+1];

      GetSBoard(sBoard);
      strncpy(sBoard+row*N,sRow,N);
      Initialize(sBoard, fBlackMove);

      return true;
   }
}

/////////////////////////////////////////
// Moving routines
/////////////////////////////////////////

// CalcMoves - return true if a move is available
// internal info:
//	a - have seen a black square and no empty square since
//	b - have seen a white square since the black square and no empty square since
//	c - color of this row
//	e - empty of this row

// simultaneously calculate the mobility of board.black and white
// ignore X-squares when the corner is unoccupied, unless fCountXMobs is true
// return the pass code: 0 = black has a move, 1=black has no move but white does, 2=no moves
int CQPosition::CalcMobility(u4& nMovesPlayer, u4& nMovesOpponent) const {
   int nrow, srow, pass;
   u4 ae, aw, a, be, bw, b, c, e;
   u2 moverRowMoves, opponentRowMoves, c2, e2;
   CBitBoardBlock opponentMoves, moverMoves;
   u2 moverPattern, opponentPattern;

   // east and west flips
   for (nrow=0; nrow<8; nrow++) {
      QSSERT(rowMovesTableSize==6561);
      moverPattern=Base2ToBase3(board.mover.u1s[nrow], board.empty.u1s[nrow]);
      opponentPattern=Base2ToBase3(board.mover.u1s[nrow]^~board.empty.u1s[nrow], board.empty.u1s[nrow]);
      opponentMoves.u1s[nrow]=rowAllMovesTable[opponentPattern];
      moverMoves.u1s[nrow]=rowAllMovesTable[moverPattern];
   }

   // other moves
   // high byte is N moves, low byte is S moves.
   // high word is board.mover, low word is opponent
   a=b=ae=be=aw=bw=0;
   for (nrow=0, srow=7; srow>=0; nrow++, srow--) {
      e2 = (board.empty.u1s[nrow]<<8)|board.empty.u1s[srow];
      c2 = (board.mover.u1s[nrow]<<8)|board.mover.u1s[srow];
      c = ((u4)c2<<16) | (c2^(u2)(~e2));
      e = ((u4)e2<<16) | e2;
      moverRowMoves = ((be|b|bw)&e)>>16;
      opponentRowMoves = ((be|b|bw)&e)&0xFFFF;
      opponentMoves.u1s[nrow]|=opponentRowMoves>>8;
      opponentMoves.u1s[srow]|=opponentRowMoves&0xFF;
      moverMoves.u1s[nrow]|=moverRowMoves>>8;
      moverMoves.u1s[srow]|=moverRowMoves&0xFF;

      ae=(~e)&(ae|c);
      be=ae&~c;
      ae=(ae>>1)&0x7F7F7F7F;
      be=(be>>1)&0x7F7F7F7F;

      aw=(~e)&(aw|c);
      bw=aw&~c;
      aw=(aw<<1)&0xFEFEFEFE;
      bw=(bw<<1)&0xFEFEFEFE;

      a=(~e)&(a|c);
      b=a&~c;
   }

   // calc pass code
   if (moverMoves.u4s[0] || moverMoves.u4s[1])
      pass=0;
   else if (opponentMoves.u4s[0] || opponentMoves.u4s[1])
      pass=1;
   else
      pass=2;

   // add 'em up
   nMovesPlayer=CountBits(moverMoves);
   nMovesOpponent=CountBits(opponentMoves);

   return pass;
}

// Set down a piece and flip opposing pieces. Change board.blacks.
void CQPosition::MakeMove(CMove move) {
   extern CBitBoard bb;

#if _DEBUG
   CMoves movesTest;
   bool fHasMove=CalcMoves(movesTest);
   bool fOK = fHasMove?movesTest.IsValid(move):move.IsPass();
   if (!fOK) {
      cout << "--- move " << move << " is invalid ---\n";
      Print();
      cout.flush();
      QSSERT(0);
   }
#endif

   // use Pos2 nowadays
   if (move.IsPass())
      Pass();
   else {
      ::Initialize(board,fBlackMove);
      ::MakeMoveBB(move.Square());
      board=bb;
      fBlackMove=fBlackMove_;

      int nWhite;
      board.NDiscs(fBlackMove, nBlack, nWhite, nEmpty);
   }
}

// update the board with the given moves.
// return false if a move is invalid or game over before we run
// out of moves
//	passCode = 0 => never pass
//	passCode = 1 => pass if needed, then make moves
//	passCode = 2 => make moves, then pass if needed
//	passCode = 3 => pass if needed, then make moves, then pass if needed
bool CQPosition::PlayMoves(const TMoveList moveCodes, int nMoves, int passCode) {
   while (nMoves--) {
      if (!PlayMove(*moveCodes++, passCode))
         return false;
   }
   return true;
}

// update the board with the given move. Return false if game was
//	over or move is invalid
//	passCode = 0 => never pass
//	passCode = 1 => pass if needed, then make moves
//	passCode = 2 => make moves, then pass if needed
//	passCode = 3 => pass if needed, then make moves, then pass if needed
bool CQPosition::PlayMove(TMoveCode mc, int passCode) {
   CMoves moves;
   CMove move;
   int pass, square;

   square=mc-' ';

   // Check mc to see if it's a valid square
   // sometimes a square of -1 is a pass:
   if (square==-1 && !passCode) {
      pass = CalcMovesAndPass(moves);
      return pass?1:0;
   }
   else if (square>=NN || square<0) {
      QSSERT(0);
      return false;
   }

   move.Set(square);

   // pass before the move if needed (or return false, depending on passCode)
   if (passCode & 1) {
      pass = CalcMovesAndPass(moves);
      if (pass==2) {
         QSSERT(0);
         return false;
      }
   }
   else if (!CalcMoves(moves)) {
      QSSERT(0);
      return false;
   }

   // check move validity
   if(!moves.IsValid(move)) {
      return false;
   }

   // make the move
   MakeMove(move);

   // pass after the move if needed
   if ((passCode&2) && !CalcMoves(moves))
      Pass();

   // all done
   return true;
}

// Values of interest

bool CQPosition::ObviouslyDone() const {
   int nWhite = 64-nEmpty-nBlack;
   return !(nEmpty && nBlack && nWhite);
}

// return net number of squares for board.black
int	CQPosition::NetSquares() const {
   int score;

   score = (nBlack<<1)+nEmpty-64;
   if (!fBlackMove)
      score=-score;
   return score;
}

// value at end of game
CValue CQPosition::TerminalValue() const {
   int nWhite=64-nBlack-nEmpty;
   int net=nBlack-nWhite;
   if (net<0)
      net-=nEmpty;
   else if (net>0)
      net+=nEmpty;

   return net*kStoneValue;
}

bool CQPosition::IsTerminal() {
   CMoves moves;
   bool result;

   if (CalcMoves(moves))
      result=false;
   else {
      Pass();
      result=!CalcMoves(moves);
      Pass();
   }
   return result;
}

const u4 northTwoRowsMask=0xFFFF0000;
const u4 southTwoRowsMask=0x0000FFFF;
const u4 eastTwoRowsMask =0x3F3F3F3F;
const u4 westTwoRowsMask =0xFCFCFCFC;

u2 CQPosition::FrontierLength(bool playerToMove) const {
   u8 mySquares, directionFrontier, frontierSquares;

   mySquares=board.mover;
   if (!playerToMove)
      mySquares^=~board.empty;

   frontierSquares[0]=frontierSquares[1]=0;

   // board.empty squares south of my squares
   directionFrontier=mySquares<<8;
   directionFrontier[0]&=northTwoRowsMask;	// except squares on top two rows
   frontierSquares|=directionFrontier;

   // board.empty squares north of my squares
   directionFrontier=mySquares>>8;
   directionFrontier[1]&=southTwoRowsMask;	// except squares on south two rows
   frontierSquares|=directionFrontier;

   // board.empty squares east of my squares
   directionFrontier=mySquares<<1;
   directionFrontier[0]&=westTwoRowsMask;
   directionFrontier[1]&=westTwoRowsMask;
   frontierSquares|=directionFrontier;

   // board.empty squares west of my squares
   directionFrontier=mySquares>>1;
   directionFrontier[0]&=eastTwoRowsMask;
   directionFrontier[1]&=eastTwoRowsMask;
   frontierSquares|=directionFrontier;

   // board.empty squares southeast of my squares
   directionFrontier=mySquares<<9;
   directionFrontier[0]&=westTwoRowsMask&northTwoRowsMask;
   directionFrontier[1]&=westTwoRowsMask;
   frontierSquares|=directionFrontier;

   // board.empty squares southwest of my squares
   directionFrontier=mySquares<<7;
   directionFrontier[0]&=eastTwoRowsMask&northTwoRowsMask;
   directionFrontier[1]&=eastTwoRowsMask;
   frontierSquares|=directionFrontier;

   // board.empty squares northeast of my squares
   directionFrontier=mySquares>>7;
   directionFrontier[0]&=westTwoRowsMask;
   directionFrontier[1]&=westTwoRowsMask&southTwoRowsMask;
   frontierSquares|=directionFrontier;

   // board.empty squares northwest of my squares
   directionFrontier=mySquares>>9;
   directionFrontier[0]&=eastTwoRowsMask;
   directionFrontier[1]&=eastTwoRowsMask&southTwoRowsMask;
   frontierSquares|=directionFrontier;

   // count squares
   return CountBits(frontierSquares&board.empty);
}

u2 CQPosition::Mobility(bool playerToMove) const {
   CMoves moves;

   if (playerToMove) {
      CalcMoves(moves);
   }
   else {
      CQPosition next(*this);
      next.Pass();
      next.CalcMoves(moves);
   }
   return moves.NMoves();
};

void CQPosition::FlipBoard() {
   board.InvertColors();
   nBlack=64-nBlack-nEmpty;
};

void	CQPosition::Pass() {
   FlipBoard();
   fBlackMove=!fBlackMove;
};

// CalcMovesAndPass - calc moves. decide if the mover needs to pass, and pass if he does
// inputs:
//	none
// outputs:
//	moves - the moves available
//	pass -	0 if the next player can move
//			1 if the next player must pass but game is not over
//			2 if both players must pass and game is over
//	If pass>0 the position is left with one move passed
//		(and you must call UndoPass before UndoMove)
int CQPosition::CalcMovesAndPass(CMoves& moves) {
   if (CalcMoves(moves))
      return 0;
   else {
      Pass();
      if (CalcMoves(moves))
         return 1;
      else
         return 2;
   }
}

void CQPosition::Print(bool fBoardOnly) const {
   FPrint(stdout, fBoardOnly);
}

void CQPosition::FPrint(FILE* fp, bool fBoardOnly) const {
   BitBoard().FPrint(fp, fBlackMove);
   fprintf(fp,"%s to move\n",fBlackMove?"Black":"White");
}

bool CQPosition::IsSuccessor(const CQPosition& posSuccessor) const {
   CMoves moves;
   CMove move;
   CQPosition pos2(*this);
   pos2.CalcMoves(moves);
   while (moves.GetNext(move)) {
      pos2=*this;
      pos2.MakeMove(move);
      if (pos2==posSuccessor) {
         return true;
      }
   }
   return false;
}
