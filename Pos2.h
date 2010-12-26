// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#pragma once

#ifndef H_POS2
#define H_POS2

#include "Utils.h"
#include "Moves.h"
#include "Timing.h"
#include "NodeStats.h"
#include "Evaluator.h"
#include "Fwd.h"

#include <boost/unordered_map.hpp>

#include <stdio.h>
#include <iosfwd>

class CBook;

class CSearchInfo {
public:
   CSearchInfo();
   CSearchInfo(int iPruneMidgame, int iPruneEndgame, int rs, int vContempt, u4 fNeeds, double tRemaining, int iCache);

   u4 PrintBookLevel() const { return fsPrint&kPrintBookLevel; };
   u4 PrintBookRandomInfo() const { return fsPrint&kPrintBookRandomInfo; };
   u4 PrintMove() const  { return fsPrint&kPrintMove; };
   u4 PrintSearchStats() const  { return fsPrint&kPrintSearchStats; };

   int iPruneMidgame, iPruneEndgame;
   int rs;	// rand shift
   int vContempt;
   u4 fNeeds;
   double tRemaining;
   int iCache;
   u4 fsPrint;	// print flags for book
   enum { kPrintBookLevel=3, kPrintBookRandomInfo=4, kPrintMove=8, kPrintSearchStats=16 };	// flags for fsPrint
};

inline CSearchInfo::CSearchInfo() : fsPrint(kPrintSearchStats) {}

inline CSearchInfo::CSearchInfo(int aiPruneMidgame,
                                int aiPruneEndgame,
                                int ars,
                                int avContempt,
                                u4 afNeeds,
                                double atRemaining,
                                int aiCache)
   : fsPrint(kPrintSearchStats)
{
   iPruneMidgame=aiPruneMidgame;
   iPruneEndgame=aiPruneEndgame;
   rs=ars;
   vContempt=avContempt;
   fNeeds=afNeeds;
   tRemaining=atRemaining;
   iCache=aiCache;
}

class CBitBoard;
class CEmptyValue;
class CEmpty;
class CUndoInfo;

// global variables
extern int nEmpty_,nDiscDiff_;
extern bool fBlackMove_;

// set up a given position. Initialize the global variables nEmpty_, nDiscDiff_, fBlackMove_
// these are updated by most of the 'customer' routines here but are passed as parameters to most internal routines
//	for performance reasons
void Initialize();
void Initialize(const char* sBoard, bool fBlackMove);
void Initialize(const CBitBoard& bb, bool fBlackMove);

// play a move or series of moves. pass if no move according to fPass:0 never, 1 before move, 2 after move, 3 both times
// return true if all moves were legal.
// update global variables
// These routines are NOT OPTIMIZED FOR SPEED
int  PassIfBB();

int ValidEmptyMove(bool fBlackMove, int square);
bool PlayMove(char mv, int fPass);
bool PlayMoves(TMoveList mvs, int nMoves, int fPass);

// print the position
void Print();
void Print(bool fBlackMove);
void FPrint(FILE* logfile);
void FPrint(FILE* logfile, bool fBlackMove);
void PrintConfigs();

// get the board text
char* GetText(char* sBoard);
i1* GetItems(i1* itemsBoard);

// value a position
int NovelloSolve(const CBitBoard& bb, int alpha, int beta, bool fBlackMove);
int NovelloSolve(int alpha, int beta, bool fBlackMove);

// test functions
void Test();
void Test1();
void TestIU();
void TestMoveSpeed();

// number of nodes
extern double nKFlips, nBBFlips,nSNodes;

// Moving
void MakeMoveBB(int square);

void PassBase();
void PassBB();

void UndoPassBase();
void UndoPassBB();

int CalcMovesAndPassBase(CMoves& moves); // calc moves, then pass if a pass is needed
int CalcMovesAndPassBB(CMoves& moves); // calc moves, then pass if a pass is needed

// evaluation
void GetIDsJ(u2 ids[nPatternsJ]);
void GetIDsL(u2 ids[nPatternsL]);
CValue ValueJMobs(const std::vector<TCoeff> pcoeffs[2] , u4 nMovesPlayer, u4 nMovesOpponent);
CValue ValueLMobs(const TCoeff* pcoeffs[2] , u4 nMovesPlayer, u4 nMovesOpponent);

// evaluation control schemes
void TimedMVK(const CCalcParams& cp, const CSearchInfo& si, CMVK& mvk, bool online);
void IterativeValue(CMoves moves, const CCalcParams& cp, const CSearchInfo& si, CMVK& mvk);

//IterativeValue support routines
double IterativeRound(int height, CValue alpha, CValue beta, const CMoves& moves, int iPrune,
                      bool fNeedMove, bool fSubset, const CNodeStats& start, CMVK& mvk);
void SetBookHeights(int height);
void InitializeCache(bool fNeedMove, bool fSubset);

// fixed-height evaluators
void ValueBookCacheOrTree(int height, CValue alpha, CValue beta, CMoves& moves, int iPrune, CMoveValue& best);
void ValueCacheOrTree(int height, CValue alpha, CValue beta, CMoves& moves, int iPrune, CMoveValue& best);
void ValueTree(int height, CValue alpha, CValue beta, CMoves& moves, int& iFastestFirst, int iPrune, CMoveValue& best);
CValue ChildValue(int height, CValue alpha, CValue beta, int iPrune);
CValue StaticValue(int iff);
bool MPCCheck(int height, CValue alpha, CValue beta, const CMoves& moves, int& iPrune, CMoveValue& best);
void ValueTreeNoSort(int height, int hChild, CValue alpha, CValue beta, CMoves& moves, int iPrune, CMoveValue& best);
bool ValueMove(int height, int hChild, CValue alpha, CValue beta, CMove& move, CMoves& moves, int iPrune,
               bool fNegascout, CMoveValue& best);

// position capture
void SetRandomCapture();

// forced openings
void InitForcedOpenings();

struct DrawCount
{
   DrawCount()
      : draws(0),
        privateDraws(0),
        edmunds(0)
   {}

   DrawCount(std::size_t draws, std::size_t privateDraws, std::size_t edmunds)
      : draws(draws),
        privateDraws(privateDraws),
        edmunds(edmunds)
   { }

   const std::size_t draws;
   const std::size_t privateDraws;
   const std::size_t edmunds;
};

typedef boost::unordered_map<CBitBoard, DrawCount> Draws;

class CGame;

DrawCount CountDraws(const CBook& book,
                     std::ostream& out);
DrawCount GetDrawCount(CBitBoard board);
void ExtractLines(const CBook& book,
                  VariationCollection& variations,
                  int bound,
                  std::size_t maxSize=std::numeric_limits<std::size_t>::max());
void ExpandLine(const std::vector<CMove>& moves);
VariationCollection ExtractDraws(const CBook& book);

#endif // #ifdef H_POS2
