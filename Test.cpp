// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// test source file

#include "PreCompile.h"
#include "Test.h"
#include "Moves.h"
#include "NodeStats.h"
#include "Evaluator.h"
#include "Player.h"
#include "QPosition.h"
#include <math.h>
#include "MPCStats.h"
#include "options.h"
#include "Cache.h"
#include "Pos2All.h"
#include "Games.h"
#include "Timing.h"
#include <fstream>
#include <string>
#include <iomanip>
#include "off.h"

extern int iff;

//////////////////////////////////////////
// Test parameters
//////////////////////////////////////////

#define NEW_GAMES 1

void SetupStuff(CMPCStats*& mpcs, CEvaluator*& eval, int& iPruneMidgame, int& iPruneEndgame) {
   const int nLevels=0;

   iPruneEndgame=iPruneMidgame=nLevels;
   if (true) {	// J4 eval
      string fn(fnBaseDir);
      fn+="coefficients/mpcJ4_11.txt";
      mpcs = new CMPCStats(fn.c_str(), 4);
      eval = CEvaluator::FindEvaluator('J','4');
   }
}

// nGamesMax is the maximum number of different games the computer will study for its statistics
// more games will be more accurate but will take longer. There are 100 games in the file
//	so nGames can take on any value up to 100. Often just the first 12 games are used though.
const int nGamesMax=100;
CSavedGame sgTest[nGamesMax];


bool LoadTestGames() {
   FILE* fp;
   int i;

   string fn(fnBaseDir);
   fn+="/results/test_games.txt";
   fp=fopen(fn.c_str(),"r");
   if (!fp){
      fprintf(stderr,"Can't open test_games file\n");
      printf("Can't open test_games file\n");
      return false;
   }
   else {
      for (i=0; i<nGamesMax && sgTest[i].FScan(fp,2); i++);
      fclose(fp);
      return true;
   }
}

const int nReps=1000;

template<class T>
void TestCalcMovesSpeed(T& pos) {
   int i, moveNumber, game, nMoves;
   bool blackToMove;
   CMoves moves;
   CNodeStats start, end;
   double seconds;

   nMoves=0;
   start.Read();

   for (game=0; game<nGamesMax; game++) {
      pos.Initialize();
      blackToMove=true;
      for (moveNumber=0; moveNumber<60; moveNumber++) {
         if (!pos.PlayMove(games[game][moveNumber+nMoveListOffset],blackToMove))
            break;
         for (i=0; i<nReps; i++) {
            pos.CalcMoves(moves);
         }
         nMoves+=nReps;
      }
   }

   end.Read();
   seconds=(end-start).Seconds();
   printf("%.3lg seconds for %ld CalcMove()s - %.3lg us/moveCalc\n",seconds, nMoves, seconds/nMoves*1e6);
}

template<class T>
void TestCalcMoveUndo(T& pos) {
   int i, moveNumber, game, nCalcMoves, nMoves;
   bool blackToMove;
   CMoves moves;
   CNodeStats start, end;
   double seconds;
   T next;
   CMove move;

   nCalcMoves=nMoves=0;
   start.Read();

   for (game=0; game<nGamesMax; game++) {
      pos.Initialize();
      blackToMove=true;
      for (moveNumber=0; moveNumber<60; moveNumber++) {
         if (!pos.PlayMove(games[game][moveNumber+nMoveListOffset],blackToMove))
            break;
         for (i=0; i<nReps; i++) {
            pos.CalcMoves(moves);
            move.Set(-1);
            while (moves.GetNext(move)) {
               next=pos;
               next.MakeMove(move, moves);
               next.UndoMove();
            }
         }
         nCalcMoves+=nReps;
         pos.CalcMoves(moves);
         nMoves+=nReps*moves.NMoves();
      }
   }

   end.Read();
   seconds=(end-start).Seconds();
   printf("%.3lg seconds for %ld CalcMove()s and %ld Get and MakeMoves\n",seconds, nCalcMoves, nMoves);
}

void TestDiagonalSpeed() {
   int i, moveNumber, game, nDiags;
   bool blackToMove;
   CNodeStats start, end;
   double seconds;
   CQPosition pos;
   CBitBoard bitBoard;

   nDiags=0;
   start.Read();

   for (game=0; game<nGamesMax; game++) {
      pos.Initialize();
      blackToMove=true;
      for (moveNumber=0; moveNumber<60; moveNumber++) {
#if NEW_GAMES
         if (!pos.PlayMove(sgTest[game].ml[moveNumber],blackToMove))
#else
            if (!pos.PlayMove(games[game][moveNumber+nMoveListOffset],blackToMove))
#endif
               break;
         bitBoard=pos.BitBoard();
         for (i=0; i<nReps; i++) {
            bitBoard.FlipDiagonal();
            bitBoard.FlipDiagonal();
         }
         nDiags+=nReps<<1;
      }
   }

   end.Read();
   seconds=(end-start).Seconds();
   printf("%.3lg seconds for %ld diagonal flips - %#.3lg us/flip\n",seconds, nDiags, seconds/nDiags*1e6);
}

int roundsign(int n) {
   if (n<-5)
      return -1;
   else
      return n>5;
}

// flags for TestMidgameSpeed
const int kPrintTestHeader=1;
const int kPrintScrzebra=2;
const int kPrintValues=4;
const int kOnlyFinalRound=8;

void TestMidgameSpeed(int nEmpty, CHeightInfo hi, int nGames, int flags) {
   int game;
   double tRun, tTotal, geoMean;
   CNodeStats start, end, start1, end1;
   CQPosition pos;
   CCalcParamsFixedHeightPtr pcp(new CCalcParamsFixedHeight(hi));
   CCalcParamsPtr pcpOld;
   CMVK mvk;
   int nMoves;
   int nResult, nCorrect;
   CComputerDefaults cd;

   // setup computer
   cd.booklevel=CComputerDefaults::kNoBook;
   CPlayerComputerPtr computer = CPlayerComputer::Create(cd, false);
   pcpOld=computer->search_pcp;
   computer->search_pcp=pcp;

   // initialize stats
   fPrintTimeUsed=false;
   start.Read();
   geoMean=tTotal=0;
   nCorrect=0;

   // print header info
   if (flags&kPrintTestHeader) {
      cout << "Testing midgame from " << nEmpty << " empties\n";
      cout << hi << "\n";
   }
   else {
      cout << hi << "\t" << nEmpty << "\t" << nGames << "\n";
   }

   // repeatedly test a game
   for (game=0; game<nGames; game++) {

      // setup board
      pos.Initialize();
      computer->Clear();
      nMoves=60-nEmpty;

      if (!pos.PlayMoves(sgTest[game].ml,nMoves,3))
         continue;

      if (flags&kPrintScrzebra) {
         // print for scrzebra
         char sBoard[NN+1], *pc;
         pos.GetSBoard(sBoard);
         for (pc=sBoard; *pc; pc++) {
            switch(TextToValue(*pc)) {
            case BLACK:
               *pc='X';
               break;
            case WHITE:
               *pc='O';
               break;
            default:
               *pc='-';
               break;
            }
         }
         cout << sBoard << " " << (fBlackMove_?'X':'O') << " % Ntest position " << nEmpty << "." << game << "\n";
         continue;
      }

      // get initial time
      start1.Read();

      // calc move and value
      CSearchInfo si(0,0,0,0,kNeedMove+kNeedValue,1e6, 0);

      computer->GetChosen(pos, si, mvk);

      // calc timing
      end1.Read();
      tRun=(end1-start1).Seconds();
      geoMean+=log(tRun);
      tTotal+=tRun;

      // check value for WLD
      nResult=sgTest[game].nResult;
      if (!pos.BlackMove())
         nResult=-nResult;
      if (roundsign(mvk.value)==Sign(nResult))
         nCorrect++;

      // print info
      if (flags&kPrintValues) printf("%d\t",mvk.value);
   }

   // print results
   end.Read();
   tRun=(end-start).Seconds();
   if (flags&kPrintTestHeader) {
      cout << "Run complete in " << tRun << "s; tTotal = " << tTotal << "; tAverage = " << tTotal/nGames;
      cout << "\n";

      cout << end-start << "\n";
   }
   else {
      cout << nCorrect << "\t" << tTotal/nGames << "\n";
   }

   computer->search_pcp=pcpOld;
}

void TestMidgameSpeeds(int nEmpty, CHeightInfo hiMax, int nGames, int flags) {
   extern int iffMidgame;

   fPrintMoveSearch=false;
   iffMidgame=4;
   CSearchInfo si(4,5,0,0,0,0,0);
   CHeightInfo hi(1,0,0);

   if (flags&kOnlyFinalRound)
      hi=hiMax;

   while (hi<=hiMax) {
      TestMidgameSpeed(nEmpty, hi, nGames, flags);
      hi.NextRound(nEmpty, si);
   }
}

void TestLogStuff(int hMax) {
   CHeightInfo hi(hMax,0,false);
   CCalcParamsFixedHeightPtr cp(new CCalcParamsFixedHeight(hi));
   CCalcParamsPtr pcpOld;
   CComputerDefaults cd;
   cd.booklevel=CComputerDefaults::kNoBook;
   CPlayerComputerPtr computer = CPlayerComputer::Create(cd, false);
   pcpOld=computer->search_pcp;
   computer->search_pcp = cp;
   CMVK mvk;


   CQPosition qpos("..................*O**....**O*....*OOO....OOOO......O...........", false);

   CSearchInfo si(0,0,0,0,kNeedMove+kNeedValue,1e6,0);

   computer->GetChosen(qpos, si, mvk);
   computer->search_pcp=pcpOld;
}

void TestEvaluator() {
   // setup computer
   CEvaluator* peval = CEvaluator::FindEvaluator('J','A');

   // repeatedly test a game
   int game=0;
   int nMoves, nEmpty, i;
   u4 nMovesPlayer, nMovesOpponent;
   CValue v;
   u2 ids[nPatternsJ];

   for (nEmpty=0; nEmpty<60; nEmpty++) {
      if (nEmpty%10 > 1)
         continue;
      nMoves=60-nEmpty;

      // setup board
      Initialize();

      if (!PlayMoves(sgTest[game].ml,nMoves,3)) {
         cout << "Error\n";
         QSSERT(0);
         continue;
      }

      // print board
      cout << "..........................\n";
      Print();

      // evaluate
      CalcMobility(nMovesPlayer, nMovesOpponent);
      cout << "--------- Eval ----------\n";
      v= peval->EvalMobs(nMovesPlayer, nMovesOpponent);
      cout << "\nValue = " << v << "\n";

      // get ids
      GetIDsJ(ids);
      cout << "========= GetIDsJ =============\n";
      for (i=0; i<nPatternsJ; i++)
         cout << setw(2) << i << ": " << ids[i] << "\n";

   }

}

void FFOTest() {
   double tRun, tTotal;
   CNodeStats start, end, start1, end1;
   CQPosition pos;
   CHeightInfo hi(0,0,false);
   CCalcParams* pcp=new CCalcParamsFixedNEmpty(hi);
   CMVK mvk;
   char buf[NN+1], c;
   string s;
   CComputerDefaults cd;

   std::ifstream ifs("ffotest.scr");

   // setup computer
   CPlayerComputerPtr computer = CPlayerComputer::Create(cd, false);

   // initialize stats
   fPrintTimeUsed=false;
   start.Read();
   tTotal=0;

   // print header info
   cout << "Testing FFO WLD solving\n";

   while (ifs>>buf>>c) {
      // comment
      getline(ifs,s);
      cout << "\n" << s << "\n";

      // setup board
      pos.Initialize(buf,TextToValue(c)==BLACK);
      //pos.Print();
      computer->Clear();

      // get initial time
      start1.Read();

      // calc move and value
      CSearchInfo si(0,0,0,0,kNeedMove+kNeedValue,1e6,0);

      computer->GetChosen(pos, si, mvk);

      // calc timing
      end1.Read();
      tRun=(end1-start1).Seconds();
      tTotal+=tRun;

      // print info
      cout << tRun << "s\t" << mvk << "\n";
   }

   // print results
   end.Read();
   tRun=(end-start).Seconds();
   cout << "Run complete in " << tRun << "s\n";

   /*
     u4 nNodes=inodes+nSNodes+nKFlips+nBBFlips;
     printf("%.3lgMn (solverflips) + %.3lgMn (kflips) + %.3lgMn (BBflips) + %.3lgMn (PosG) = %.3lgMn (Total), %.3lgMn/s\n",
     (double)nSNodes*1e-6,(double)nKFlips*1e-6,(double)nBBFlips*1e-6,
     (double)(inodes)*1e-6,(double)nNodes*1e-6,(double)nNodes*1E-6/tRun);
   */

}

struct is_char
{
   char c;
   is_char(char c)
      : c(c)
   {}
   bool operator()(char s) const
   {
      return c==s;
   }
};

struct TSolverPosition {
   std::string board;
   int nResultNoEmpties;
};

TSolverPosition bds[] = {
   // should be 0
   {"--b-----b-bwww--bbbbwbb-bwbwwb--bwwbwbb--bwbwbbb-wbbbww-bwbbbb--b", 0},
   // solved as -38 on GGS
   {"..bbbbb...bwwww...wwbwwb..wbbww...bwwbww.bbbbwww..wwwwww..w.bbb.", 42},
   {"..bbbbb...bwwww...wwbwwb..wbbww...bwbbww.bbbbbww..wwwwbw..w.bbbb",-42},
   {"..bbbbb...bwwww...wwbwwb..wbbww...bwbbww.bwbbbww.wwwwwbw..w.bbbb", 42},
   {"..bbbbb...bwwww...wwbwwb..wbbww...bbbbww.bbbbbww.bwwwwbwb.w.bbbb",-42},
   {"..bbbbb...bwwww...wwbwwb..wbbww.w.bbbbww.wbbbbww.bwwwwbwb.w.bbbb", 42},
   {"..bbbbb...bwwww...wwbwwb..wbbww.w.bbbbww.bbbbbwwbbwwwwbwb.w.bbbb",-42},
   {"..bbbbb...bwwww...wwbwwb..wbbww.wwwwwwww.bwbbbwwbbwwwwbwb.w.bbbb", 42},
   {"..bbbbbb..bwwwb...wwbbwb..wbbww.wwwwwwww.bwbbbwwbbwwwwbwb.w.bbbb",-42},
   {"..bbbbbb.wwwwwb...wwbbwb..wbbww.wwwwwwww.bwbbbwwbbwwwwbwb.w.bbbb", 42},
   {".bbbbbbb.wbwwwb...wbbbwb..wbbww.wwwwwwww.bwbbbwwbbwwwwbwb.w.bbbb",-42},
   // solved as -2 on GGS
   {"...bbb.....bbw...bbbwbww.bbwbwwwbbwbwbwwbwbbwwwwwwwwbwwwbbbbbbbb",0}, // 12 ply
   {"...bbb.....bbbb..bbbwbbw.bbwbwbwbbwbwbbwbwbbwwbwwwwwbwbwbbbbbbbb",0}, // 11 ply
// some trivial positions:
   {"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww..",62},
   {"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwb.",64},
   {"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwbw..",64},
   {"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwbww.",56},
   {"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwbw...",63},
   {"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwbwbw....",49},
   {"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbwbw...",-64},
   {"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbwb...",-63},
   {"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbwb..",-64},
   {"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbwb.",-58},
   {"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbwbbbbbbbwbbbbbwb.",-54},
   {"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwbwwwwwwwbwwwwbbb.",64},

// one added by me:
   {"wwwwwwwbwwbbbww.wbwbwbw.wbbwbbbbwbwbbwbwwwbbbbwww.bbbbw..bbbbbbw",-10},

// Harder positions
   {"..wwwww.b.wwbw..bbwbbwwwbwbbbwwwbbwbbwwwbbbwbwww..bbwbw....bbbbw",-12},
   {"wbbw.b...wwbwww.wwwwbwwwwwwbwbbbwwwwbwb.wwwwwbbb..wwbbb..wwwww..",-6},
   {"..w.bb.bw.wbbbbbwwwwbbbbwbwwwwbbwbbbwwbwwwbbbbbww.wbbbbw......bw",28},
   {"bwwwwww..bwwwww..bbbwwww.bbbbbww.bbbbwww.bbbbbww..bbbbw...bbbbbw",-8},
   {"b.wbw....bwwww.bbbbbbbbbbbbbbbbbbbwbbbbbbbbwwbbbb.wwww....w.bbbb",-4},
   {"..bwb..b.bbwwwwwbbbwbbwbbwwbbbwbbwwwbwbbb.wwwwbbb.wwwb.b...wbbb.",-2},
   {"...bwwbb..bbbwwb.bbbbbwwbbwbbbb.bbbwbbbb.bbbwwbb..bbbbbb..bbbbwb",12},
   {"..bbbbwb..bbbwbb.bbwwbbbwwwwbbbb.wwbbbbb.wwbbwbb.bbwww...bwwwww.",-12},
   {"..b.....wwbb.b..wwwbbbbbbwwwbbwwbbwbbwwwbbbwwwwwbwbbwb..bwwwwwww",4},
   {"b.w.b...bb.bbw.wbbbbbbwwbbbbbwwwbwwwwwwwbwbwwww.bbwwww.b..wwwww.",-22},
   {"b..b....b.bbbb.bbbwwwwbbbbbbwbbbbwbwbbbbbwbwbwbbbwwwww..bwwwww..",-12},
   {"wbbb.ww.wbbbbw.bwbbwwb.bwbbwwbwbwwbwwww.wwbwwwwwwbwwww....ww..w.",8},
   {"bbbbbbb...wwww.bbbbbbwwb.bwbwwwbbbbwwwwb.bbbwwwb..wwbww...wwwww.",4},
   {"b.w.b...bb.bbw.wbbbbbbwwbbbbbwwwbwwwwwwwbwbwwww.bbwwww.b..wwwww.",-22},
   {"bbbbbbb...wwww.bbbbbbwwb.bwbwwwbbbbwwwwb.bbbwwwb..wwbww...wwwww.",4},
   {"b..b..w.bbbbbw.bbwbwwwbbbwbbwbbbbwbwbbbbbwbbbwbbbbbbww..b.b.ww..",-12},
   {"b..b....b.bbbb.bbbwwwwbbbbbbwbbbbwbwbbbbbwbwbwbbbwwwww..bwwwww..",-12},
   {"...bbbb...bbbb.wbbbbbww.wbwbwww.wbbbbwwwbbbbbbww.bbbbbbw.wwwwww.",20},
   {"..bbb.....bbbw.wwwwwbwwwbwwbbbbwwwbwwbbw.bwwbwbwb.wwbbbw..wwwwbw",-8},
   {"w..bbbb.bbbbbbb.wbwbwwbbwwbbwbbbwwbbwbbbw.wwwwbb..wwww.b..wwww..",8},
   {"..bbbbb.wwbbbbb..wwbbbbwbbwwwbbwbbwwwbbwbbbbbwbww.wwwwww.....w.w",4},
   {".bbb....w.bbbb.wwbbbbbbbwbbwbbbbwbwbwwbbwwbbbbwbw.bbbwww...bww.w",2},
   {"..wwwww.bbwwww.bbbbwwwbbbwbbwbwbbwbbwbbbbbwbbbbbb.wwww......www.",-12},
   {"w..bbb..bbbbbb..wbwbwbbbwbbwwbbbwbwbwbbbwbbwwwbb.bwwww.b..wwww..",8},
   {".bbbbbbb..wwbwb.wwwwwbww.wbwbwwb.bwbbwbb.wbbbbbb...bbbbb..wwwwww",0},
   {"..bbbbbw..bbbbww..bbbwbw..bbwbbw.wbwwbbwwwbbwwwwwwbbbb.bbbbbbb..",6},
   {".wwwww...wwbbbb.wwwwbbbbwwwwbwb.wwwwwww.wbwwwwwwbbbwww..wwww..w.",-6},
   {"b.w.b...bb.bbb..bbbwwwwwbbbbbwwwbwbbwwbwbbwwbwwwbwwwwwwww....wbw",-2},
   {"...bbbb...bbbb.wbbbbbww.wbwbwww.wbbbbwwwbbbbbbww.bbbbbbw.wwwwww.",20},
   {".wbbbb.w..bbbb.w.wbwwwbw.bbwwwww.bbwwwbw.bwbwbww.bbbbww.wbwwwww.",14},
   {"..wbbw.w..wwwwwbwwwwwwbbwwwbwbwbwwbwbwwbwbbbwbbbw.bbbbbb..bb....",8},
   {"..bbbbb..bbbbbbbbbbwbbbbwbbbwbbbbbbbbwbb.bbbwww..bbbbwww....bbw.",12},
   {"..w..bww...wbbbb..wwbbbwbbbbbbwwbbbbwbwwbwbbwwwwbbwwwww.bbbbb.w.",-12},
   {".wwwww..b.wbbw.w.bbwwbwwwwbbwwb.wwwbwwbbwwbwbwbb..bbwbb...bbbbbb",14},
   {"....w.....wwww.wwwwwbbbbwwwbwbbbwwwbwwbbwwbwwbwb.bbbbbbw.wwwwwww",-10},
   {".ww.b.....wwbb..wbbwwbbbwbbbwwbbwbbbbbwbwbbbbwbb.bbbwb.b.bbbbbbb",12},
   {"..wbbbbww.bbwwwwwbbbwwwwwbbwwbwwwwwwwwwww.wbbwwww.w..www...bbb..",0},
   {".wbbbb.w..bbbb.w.wbwwwbw.bbwwwww.bbwwwbw.bwbwbww.bbbbww.wbwwwww.",14},
   {"..bbbb..w.bww...wwbbww..wbbwbwwwwbbbwwwwwbbbbwbw.bbwbbbw.bbbbbbw",-8},
   {".bb.w....bbbww.bwbwbbwbbwwbwwbbbwwbwwwb.wwwbbwww.wwwwwww..bwwww.",-8},
   {".....w....wwwwwwbbwbbbwb.bwwwwwbbbbwwbwbbbwbbwbbbbbbbbbbwbbbbb..",2},
   {"w..bww..bwwbbbbbbbbwbbbbbbbbwbbb..bwwwb...wwbwww.wwwwwww.wwwwww.",-2},
   {"wwwwwbw...wwwww.bbwbbbbb.bbwwbbbbbbbbbbb.bwwwbbbb.wwww....wwwww.",18},
   {"..w.wb..wwbwww..wwbbwbw.wbbwbbw.wbbbwbwwwwbwwwbwwbbbbbbwb.bb..bw",8},
   {"..wbbw..wwwbbbbbwwbbbbbbwwwbbbwbwwwbwwww.wbbwb.w.bbbbw...b.wwww.",-6},
   {"..wbbb.....b.bwwwwbbwbwwwwbbwwww.wbwwbww.wwbbwwb.wwwwwwb.wwwwwww",-10},
   {"b.wwwww.bbwwwwb.bwbwwbbb.bwbbwbb.wbwbbbbw.bwbbbw...wwbbw..bw.wbw",-2},
   {"..bwww....bbbb...wbwbb.wwwbwwwwwwwbbwbwwwwbbbwbwwwwbbbbw..wbbbbw",-6},
   {"bbbbbb...bwbbbbbbbbwwbbwbbwbbbww.bwbbbw..bwbwbw...wwbwb...wwwwwb",2},
   {"w.w.....w.wwwb..wwwwbwwbbwbbbwwbbwbwwbwbbbwwbbbbbwwbww.bwwwwww..",2},
   {"....b.wb..bbbbbb..bbbbbb..bbbbw.wwbbbwwwwbbbbwwwbbbwwwwwbwwwwwww",0},
   {".bbbbbb.b.bbbb.bbbwwwwbbbbwwbwwbbbwbwbwb.wbwwwbbw.wwww.b...wbw..",10},
   {".wwwwww...wbbw..w.bwwbw..bbbwbwwbbbwbbwwbbwbbbbw.wbbbbbw..wbbbbw",-6},
   {"..wbbw...wwwwwbbbbwbwwwbbbbwbbwbbbbbbbwbbbbbbwbb..bbbb.b..bbbb..",-8},
   {"..wbbw..wwwbbbbbwwbbbbbbwwwbbbwbwwwbwwww.wbbwb.w.bbbbw...b.wwww.",-6},
   {"b..www...bwwwwwwbwbbbbwwbwbbbwbwbbbbbbbwbbbbbbbwb..bbbbw...w..bw",4},
   {".b.www....bbbb...wwbbb.wwwwwbwwwwwwbwbwwwwbbbwbwwwwbbbbw..wbbbbw",-6},
   {".bbbbb...wwwwbbbwwwbwbb.bwbwwbbwbbwbwbbwbbbbbbww...bbbww....wbbw",18},
   {"..bbbb.b..bwbbb.bbbbbbbbbbbbbbbbbbbbwbbbwbbbbbbb..bbbb...wwwwww.",12},
   {".wbw.....bbbwb..bwbwbw...wwwwwwbwwwwbwbbwwwwwbbbbwbwbbbb.bwwwwwb",0},
   {".wwwww....wbbw..bbbbbww.bbbwbww.bbbwwbwwbbbbwbww.wbbbbbw..bbbbbw",2},
   {"wwwbbbbwbbbbbbbwwbwbwwww.wbwwwbwwwwwwbwwb.bbbwww...bbww......bw.",-2},
   {".b.wwww...bbbw...wwbwb.wwwwwbbb.wwwbwbbbwwbbbwbbwwwbbbbb..wbbbbw",-6},
   {"..b.b.w.w.bbbb.b.bbbwbbbbbwbbwbbbwwwbwwbbwwwwbwbb.wwwwbb..wwww.b",0},
   {".wwwww...bbbbw.bbwbwwwbbbwwbwbbbbwbwbbb.wwwbwb.w.wwbbw...wwwwww.",-4},
   {"..bbbw...bbbbb..bwwbbbbbbbwwbbbbbwbbwwwwbbwbwwwwb.bwbww..b.wwww.",6},
   {"..bbbbb.w.bbwb..wbbwbww.bbbbwww.bwwwwww.bwwwwwwwb.wwwww..bwwwwww",-6},
   {"..w.wb....wwwb.wbbwwwbwwbwwwbwwwbbbwwbwwwwwbwwbw..wwbbbb..wwwwww",-14},
   {"..bbbbbbw.bbbb.bwwbwbwwbwbwbwwwbwwbbwbwbwwwwwwbb...w.bwb....bbww",-22},
   {"...bw....bbwwb.bwwbwwbbbwwwbwbbb.wwbbbbbbwwbbbwb..wwwwwb.bwwwbbb",-8},
   {"..wbwwwbwbbbbwwbwbbbwbwbwwwwbwwbwbwbwbwbwwbwwwbbw..bww.b.....w..",-10},
   {"..bbw..w..bwwwbbwwbbwwwbwbbwwwwwwbwwwwwwwwwwwww.wbwww...wwwwww..",18},
   {"..wwwww..bbbww.wbbbwwwwwbbbwwwbwbbwbwwwwbwbbbwwwbbbbbb..b.b...b.",14},
   {".bbbbbb...bbwb..wwbwbwwbwwwbwwwwwwwwbbwb.wwwbbw..wwbwww...bwwwwb",-4},
   {".bbb..wwb.bwbwbbwbwbwwbbwbbwwwbwwbwwbw..wwwwwww.wbwww...wwwwww..",18},
   {"..wwww....wwwwb.bbwwwbbwbwwwbwbwwwwwwwbbbbbwbwbw.bbbww..bwwwww..",6},
   {".bbbbbbb.bbw.wb.wbbwwbwbwbbbbw.wwbwbbww.wbbwwb..wbbbbbb..wwwww..",-1},
   {"wbbbwb...bbbbw..wbwwwwwbwwbwwwbbwwwwwbwbwwwbwwbb..wbww.b..wwww..",2},
   {"..wbb...bbwbbb..bbbwwwbwbbwbbbbbbbbwbwbb.bbbwbbbwwbwbbb..wwbbw..",-2},
   {"bbbbbbb..bwwwb.w.wbbbbbw.wwbbwbw.wwwwbbw..wwbwbw...wbbbw.wwwwwww",2},
   {"b.w..w.b.bwwwwww.bbwbbwwbbbbbwbwwbwwwwww..bbbbbw..bbbbbw..bbbbbw",-2},
   {".bbbbb..b.bbbbw.bbwbbwb.bwwwbbbbbwwbwbbwbwbbbbb.bbbwww..bb...www",-2},
   {"wwwwwww..wbbbw.w.wwbbbbw.wbwbbbwwwwwbwwwb.wwbwbw..wwbbbw...wb.bw",12},
   {".bbbwww..bbwwwwwwbwwwwwwwwbbbwwwwbwbbbwwwwbbwbbbw.bbb.....bbw...",-12},
   {"wwwwwww..wbbbw.w.wwbbbbw.wbwbbbwwwwwbwwwb.wwbwbw..wwbbbw...wb.bw",12},
   {"..w.wb..w.wwww...wbbwwbb.bwbbbwbwbbbbwwbwwwbbbbbw.bwbbbb.bbbbbwb",-2},
   {"..ww....w.www...wwwbbwbbwwbbbbwbwbwwbbwwbbwwwbbbb.bwwwbb.bbbbwwb",2},
   {".wwwww..b.wbbw..bwbwbbw.bbbwbwwwbbbbwwwwbbbbww.b.bbbbbw...wwwwww",-2},
   {"...wbb..bb.wbbb.bbbbwwwwbbbbbwwwbbwbwwww.wbwbwwww.bbbb...wwwwwww",4},
   {"..wwww.bbbwwwwb.bbbwwbwwwbbbbwbbbbbbbww.bbbwbw.w.bbw.w...bbb.www",-12},
   {"w.wbbbbw.wbbbbb.bbwbbwb..bwwbwbwbbbwbbbwwbbbbbbw..bbbbb....wwww.",6},
   {"..bb.b....bbbb.bwwwwwwbb.bwbwbbbbbbbbbbbbbbbbbwbb.wwwwwb..bwwwwb",6},
   {"..wwwb..b.wwbb..bbbbwbbbbbbwbbbbbbwbwwbb.bwbwbwww.bbbbw...wbbbbw",6},
   {"bbbbbw..bwwb.w..bbbwww..bbwwwww.bwwbwbw.bwbbwwbww.bbwbb..wwbbbbw",-25},
   {"..bbbbb.w.bbbb.bwbbbbbbbwbbbbbwbwbwbwww.b.wwwww..bwwww...bbbbbbb",-25},
   {"w.wbbbbw.wbbbbb.bbwbbwb..bwwbwbwbbbwbbbwwbbbbbbw..bbbbb....wwww.",6},
   {"..wbbbbw..bwwwwwwbwbwbwwbbbwwwbww.wbwbww.wbwbwww..bbwww..bbbbw..",2},
   {"..bbbb....bbbb..wwwwwbwbwwwwwwbbwwwbwwbbwwbwbwb.wbbbbbbw...bbbbw",33},
   {".bbbbbb.wwwwbw..wwwbwbw.wbwwwwb.wbbbbbbbwwwwwww.w.wbww....wbwwww",-20},
   {".wwwwww...bbbb..b.bbwbbbwwbbwbb.bwbbwbb.bwwbbbb.wwwwbwb..bbbbbbb",-4}
};

const int nEndgames = sizeof(bds)/sizeof(bds[0]);

#define PRINT_DETAILS 1	// 0 = 1-line output per position, 1=print board and other info
#if PRINT_DETAILS
#define PD(x) x
#else
#define PD(x)
#endif

void TestEndgameAccuracy(){
   int i;
   CNodeStats start,end, start1,end1,delta1;
   CValue value;

   // create computer
   CComputerDefaults cd;
   cd.booklevel=CComputerDefaults::kNoBook;
   CPlayerComputerPtr computer = CPlayerComputer::Create(cd, false);

   fPrintTimeUsed=false;
   fPrintMoveSearch=false;
   start.Read();

   for (i=0; i<nEndgames; i++) {
      bool fBlackMove = std::count_if(bds[i].board.begin(), bds[i].board.end(), is_char('.'))%2==0;

      Initialize(bds[i].board.c_str(), fBlackMove);
      computer->Clear();
      PD(Print());

      start1.Read();

      CMVK mvk;
      CSearchInfo si;
      si.fNeeds=kNeedValue+kNeedMove;
      si.tRemaining=1e6;
      si.fsPrint=0;
      si.iCache=0;
      computer->GetChosen(CQPosition(bb,fBlackMove_),si,mvk);
      value=mvk.value/kStoneValue;
      end1.Read();
      delta1=end1-start1;

      if (value==bds[i].nResultNoEmpties)
         cout << "    ";
      else {
         QSSERT(0);
         cout << "ERR ";
      }
      cout << setw(2) << i;
      cout << ": Value: " << setw(3) << value;
      cout << ". Nodes: " << setw(6) << delta1.Nodes();
      cout << ". Board: " << bds[i].board << "\n";
      PD(cout << end1-start1 << "\n");
      PD(printf("-------------------------------------------\n"));
   }

   end.Read();
   cout << end-start << "\n";
}

extern bool fPrintMoveSearch;

void TestMoveSpeed(int hSolveFrom, int nGames, char* sMode) {
   CHeightInfo hi(hSolveFrom-hSolverStart, 0,true);
   //if (!LoadTestGames())
   //   return;
   if (false) {
      fPrintMoveSearch=true;
      //FFOTest();
      // if the mode contains an e it is endgame only
      bool fEndgameOnly=sMode && strchr(sMode,'e');
      //for (int i=10; i<=30; i++)
      //	TestMidgameSpeed(i,nGames,fEndgameOnly);
      TestMidgameSpeed(hSolveFrom, CHeightInfo(hSolveFrom,0,true), nGames, (sMode && strchr(sMode,'e'))?kOnlyFinalRound:0);
   }
   else if (false)
      TestEvaluator();
   else if (true) {
      TestEndgameAccuracy();
   }
   else {
      const int hEndgame=20;
      TestMidgameSpeed(hEndgame, CHeightInfo(hEndgame-hSolverStart,0,true), 12, kPrintTestHeader);
      TestMidgameSpeed(35, CHeightInfo(20,4,false), 12, kPrintTestHeader);
   }
}
