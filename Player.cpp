// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// Player source code

#include "PreCompile.h"
#include "Debug.h"
#include "Moves.h"
#include "Cache.h"
#include "Player.h"
#include "TreeDebug.h"
#include "QPosition.h"
#include "NodeStats.h"
#include "Ticks.h"
#include "Games.h"
#include "MPCStats.h"
#include "Pos2.h"
#include "Book.h"

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <strstream>
#include <fstream>
#include <iomanip>
#include <algorithm>

const bool fBookWLDOnly=true;

//////////////////////////////////////////////////
// CPlayer
//////////////////////////////////////////////////

CPlayer::CPlayer() {
   sName[0]=0;
   fnSaveGame=0;
}

CPlayer::~CPlayer() {
   if (fnSaveGame)
      delete[] fnSaveGame;
}

// game playing

void CPlayer::StartMatch(const COsMatch& match) {
}

CPlayer::TCheatcode CPlayer::Update(COsGame& game, int flags, CSGMoveListItem& mli) {
   TCheatcode cheatcode;

   // time the call to GetMove()
   i8 ticksPerSecond=GetTicksPerSecond();
   i8 start=GetTicks();

   cheatcode = GetMove(game, flags, mli);

   i8 end=GetTicks();
   mli.tElapsed = (end-start)/(double)ticksPerSecond;

   return cheatcode;
}

// display

void CPlayer::SetName(const char* newName) {
   strncpy(sName, newName, 31);
   sName[31]=0;
}

void CPlayer::SetName(const char* newName, int number) {
   sprintf(sName, "%.20s%d",newName, number);
}

const char* CPlayer::Name() const {
   return sName;
}

bool CPlayer::NeedsBoardDisplayed() const {
   return false;
}

//////////////////////////////////////////////////
// CPlayerHuman
//////////////////////////////////////////////////

CPlayerHuman::CPlayerHuman() {
   printf("Please enter your name: ");
   scanf("%31s",sName);
}

CPlayerHuman::CPlayerHuman(const char* sAName) {
   strncpy(sName,sAName,32);
   sName[31]=0;
}

bool CPlayerHuman::Save(const COsGame& game) {
   string fn;
   cin >> fn;

   ofstream os(fn.c_str());

   bool fResult = (os << game)!=0;
   cout << (fResult?"Saved ":"Failed to save ") << fn << "\n";

   return fResult;
}

bool CPlayerHuman::Load(COsGame& game) {
   string fn;
   cin >> fn;

   CGame* pGame=dynamic_cast<CGame*>(&game);
   if (pGame) {
      int err=pGame->Load(fn.c_str());
      cout << (err?"Failed to load ":"Loaded ") << fn << "\n";
      if (!err)
         pGame->SetInfo();

      return !err;
   }
   else {
      cerr << "Internal error at " << __FILE__ << " line " << __LINE__ << "\n";
      QSSERT(0);
      return false;
   }
}

CPlayer::TCheatcode CPlayerHuman::GetMove(COsGame& game, int flags, CSGMoveListItem& mli) {
   CMoves moves;
   string sInput;
   CSGSquare sq;

   while (flags&kMyMove) {

      // input and convert to lowercase
      cout << "Please enter next move: ";
      cin >> sInput;
      for(std::size_t i=0; i<sInput.length(); i++) {
         sInput[i]=tolower(sInput[i]);
      }

      // make a move?
      if (sInput.length()>=2 && isalpha(sInput[0]) && isdigit(sInput[1])) {
         if (game.pos.board.IsMoveLegal(sInput)) {
            mli.mv=CSGSquare(sInput);
            break;
         }
         else
            cout << "'" << sInput << "' is not a valid move.\n";
      }
      // pass?
      else if(!strncmp(sInput.c_str(),"pa",2)) {
         if (!game.pos.board.HasLegalMove()) {
            mli.mv="PA";
            break;
         }
         else
            cout << "You have a valid move, you can't pass.\n";
      }

      // cheat code ?
      else {
         if (sInput=="save") {
            Save(game);
         }
         else if (flags&kAllowCheats) {
            if (sInput=="load") {
               Load(game);
               //return kCheatContinue;
            }
            else if (sInput=="undo") {
               return kCheatUndo;
            }
            else if (sInput=="ap") {
               return kCheatAutoplay;
            }
            else if (sInput=="switch") {
               return kCheatSwitch;
            }
            else if (sInput=="play") {
               string sLine;
               getline(cin, sLine);
               istrstream is(sLine.c_str());
               CSGMoveListItem mli;
               while ((is >> mli)) {
                  if (game.pos.board.IsMoveLegal(mli.mv))
                     game.Update(mli);
                  else {
                     cout << "Play sequence terminated at illegal move " << mli.mv << "\n";
                     break;
                  }
               }
               return kCheatContinue;
            }
            else if (sInput=="?"|| sInput=="help") {
               cout << "Current cheat codes:\n";
               cout << " save filename: save the current position as filename\n";
               cout << " load filename: load the current position as filename\n";
               cout << " ap: have opponent autoplay the remainder of the game\n";
               cout << " switch: switch sides in game\n";
               cout << " play <move>*: force a sequence of moves to be played\n";
               cout << " undo: undo your last move\n";
               cout << "\n";
            }
         }
         else
            cout << "Your move must consist of a letter followed by a number, e.g. A8\n";
      }
   }

   mli.dEval=0;
   return kCheatNone;
}

// information

bool CPlayerHuman::NeedsBoardDisplayed() const {
   return true;
}

bool CPlayerHuman::IsHuman() const {
   return true;
}

TMoveSignal CPlayerHuman::MoveSignal() const {
   return kSignalStdin;
}

void CPlayerHuman::EndGame(const COsGame &, Timer<double> &) {
}

////////////////////////////////////////
// CComputerDefaults
////////////////////////////////////////

CComputerDefaults::CComputerDefaults()
   : cEval('J'),
     cCoeffSet('A'),
     sCalcParams("s12"),
     iPruneMidgame(4),
     iPruneEndgame(5),
     fEdmundAfter(false),
     booklevel(kNegamaxBook)
{
   vContempts[0]=kMaxBonus;
   vContempts[1]=-kMaxBonus;
   nRandShifts[0]=nRandShifts[1]=0;
   fsPrint=5;
   fsPrintOpponent=1;
}

int CComputerDefaults::MinutesOrDepth() const {
   if (sCalcParams.empty())
      return 0;
   else
      return atol(sCalcParams.c_str()+1);
}

CValue CComputerDefaults::FloatToContempt(double fContempt) {
   CValue v=fContempt*kStoneValue;
   if (v>kMaxBonus)
      v=kMaxBonus;
   else if (v<-kMaxBonus)
      v=-kMaxBonus;

   return -v;
}

istream& CComputerDefaults::In(istream& isBig) {
   string sLine;
   getline(isBig, sLine);
   istrstream is(sLine.c_str());

   double fContempt[2];
   string sCalcParamsTemp;
   bool fUseBook, fCNABook;

   is >> cEval >> cCoeffSet;
   if (is >> sCalcParamsTemp)
      sCalcParams=sCalcParamsTemp;
   if (is >> fContempt[0])
      vContempts[0]=FloatToContempt(fContempt[0]);
   if (is >> fContempt[1])
      vContempts[1]=FloatToContempt(fContempt[1]);
   is >> nRandShifts[0] >> nRandShifts[1];
   is >> fUseBook >> fCNABook;
   is >> fsPrint >> fsPrintOpponent;

   if (!fUseBook)
      booklevel=kNoBook;
   else if (fCNABook)
      booklevel=kNegamaxBook;
   else
      booklevel=kBook;

   return isBig;
}

////////////////////////////////////////
// CPlayerComputer
////////////////////////////////////////

void CPlayerComputer::Switch()
{
   swap(search_pcp, book_pcp);
}

CPlayerComputer::CPlayerComputer(const CComputerDefaults& acd, bool online, int movesToAdd)
   : mOnline(online),
     movesToAdd(movesToAdd)
{
   cd=acd;

   if( cd.sCalcParams=="s26" )
      // s26 uses this one:
      search_pcp = CCalcParams::NewFromString("m15", true);
   else
      // s20 uses this one:
      search_pcp = CCalcParams::NewFromString(cd.sCalcParams);

   book_pcp = CCalcParams::NewFromString(cd.sCalcParams);

   caches[0]=caches[1]=NULL;
   fHasCachedPos[0]=fHasCachedPos[1]=false;
   book=(cd.booklevel!=CComputerDefaults::kNoBook) ? CBook::FindBook(cd.cEval, cd.cCoeffSet, book_pcp) : CBookPtr();
   eval=CEvaluator::FindEvaluator(cd.cEval, cd.cCoeffSet);
   mpcs=CMPCStats::GetMPCStats(cd.cEval, cd.cCoeffSet, Max(cd.iPruneMidgame, cd.iPruneEndgame));
   fAnalyzingDeferred=false;

   // get saved-game file name
   ostrstream os;
   os << fnBaseDir << "results/" << cd.cEval << cd.cCoeffSet << '_' << *book_pcp << ".ggf" << '\0';
   fnSaveGame=new char[os.pcount()];
   if (fnSaveGame)
      strcpy(fnSaveGame,os.str());
   os.rdbuf()->freeze(0);

   toot=false;

   // calculate computer's name
   ostrstream osName(sName, sizeof(sName));
   search_pcp->Name(osName);
   osName << '\0';

   // Set up MPC widths
   if (mpcs) {
      int iPruneMax=cd.iPruneMidgame;
      if (cd.iPruneEndgame>cd.iPruneMidgame)
         iPruneMax=cd.iPruneEndgame;
   }
   else
      cd.iPruneMidgame=cd.iPruneEndgame=0;


   //SetupBook(cd.booklevel==CComputerDefaults::kNegamaxBook);
}

CPlayerComputerPtr CPlayerComputer::Create(const CComputerDefaults& acd, bool online, int movesToAdd)
{
   CPlayerComputerPtr ptr(new CPlayerComputer(acd, online, movesToAdd));
   assert(ptr.use_count()==1);
   assert(ptr->book.use_count()==1);
   ptr->SetupBook(acd.booklevel==CComputerDefaults::kNegamaxBook);
   assert(ptr.use_count()==2);
   assert(ptr->book.use_count()==1);
   return ptr;
}

CPlayerComputer::~CPlayerComputer() {
   int i;
   for (i=0; i<2; i++)
      if (caches[i])
         delete caches[i];
}

u4 CPlayerComputer::LogCacheSize(CCalcParamsPtr pcp, int aPrune) {
   u2 lgCacheSize;

   // basic suggestion from the calc params
   lgCacheSize=pcp->LogCacheSize(aPrune);

   // constrain lgCacheSize to be reasonable, due in part to Pentium handling of
   //	shift operators with right operand >= 32
   if (lgCacheSize<2)
      lgCacheSize=2;
   else if (lgCacheSize>30)
      lgCacheSize=30;

   // constrain lgCacheSize to fit within available RAM (from params.txt)
   if (maxCacheMem>sizeof(CCacheData)) {
      while (sizeof(CCacheData) > maxCacheMem>>lgCacheSize)
         lgCacheSize--;
   }

   // lgCacheSize must be at least 2, due to the way cache operates
   if (lgCacheSize<2)
      lgCacheSize=2;

   return lgCacheSize;
}

#define TOURNAMENT 1
bool CPlayerComputer::DEFER_ANALYSIS = false;

void CPlayerComputer::GetBookVariability(const COsGame& game, CSearchInfo& si) {
   int iMover=game.pos.board.iMover;
   double rMe, rOpp, rDelta;

   // if TOOT the book variation doesn't matter,
   //	so assume it's our move
   rMe=game.pis[iMover].dRating;
   if( game.pis[!iMover].sName==game.pis[iMover].sName )// selfplay game
      rOpp = rMe*9/10;
   else
      rOpp=game.pis[!iMover].dRating;
   rDelta=rMe-rOpp;

   si.rs=0;

#if TOURNAMENT
   //si.vContempt=kStoneValue*.5;
   si.vContempt = 0;
   si.rs = -1;
#else
   // if player is better than we are
   if (rDelta<0 ) {
      si.vContempt=kStoneValue*rDelta/100;
      if (rDelta<-200)
         si.rs=-1;
   }

   // if player is worse
   else {
      si.vContempt=kStoneValue*rDelta/300;
      int rRandMax=(rMe<2200?rMe:2200);

      // increase rand shift?
      if (rRandMax-rOpp>0) {
         si.rs=(rRandMax-rOpp)/250;
         if (si.rs>2)
            si.rs=2;
      }
   }
#endif
   si.rs=0;
}

CCache* CPlayerComputer::GetCache(int iCache) {
   if (caches[iCache]==NULL)
      caches[iCache]=new CCache(1<<LogCacheSize(search_pcp, cd.iPruneMidgame && cd.iPruneEndgame));

   if (caches[iCache]==NULL) {
      cerr << "out of memory allocating cache " << iCache << " for computer " << Name() << "\n";
      _ASSERT(0);
      exit(-1);
   }

   return caches[iCache];
}

CPlayer::TCheatcode CPlayerComputer::GetMove(COsGame& game, int flags, CSGMoveListItem& mli) {
   CMVK chosen;
   chosen.Clear();
   double tRemaining;

   tRemaining=game.pos.cks[game.pos.board.iMover].tCurrent;

   CQPosition qpos(game.pos.board);
   u4 fNeeds=kNeedMove;
   if (game.mt.fRand)
      fNeeds|=kNeedRandSearch;
   CSearchInfo si(0,0,0,0, fNeeds, tRemaining, CIdGame(game.Idg()).NIdmg()==1);
   GetBookVariability(game, si);

   //game.pos.board.OutFormatted(cout);
   if (flags&kMyMove) {
      search_pcp->fInBook=fInBook;
      GetChosen(qpos, si, chosen, !game.mt.fRand);
      fInBook=chosen.fBook;

      if (chosen.fKnown)
         mli.dEval=chosen.value/(double)kStoneValue;
      else
         mli.dEval=0;

      mli.mv=chosen.move;
   }
   else {
      si.fsPrint=cd.fsPrintOpponent;

      if (si.PrintBookLevel() && !(flags&kNoPrint)) {
         CMoves moves;
         CMVK mvk;
         qpos.CalcMoves(moves);
         CBookPtr bp = book.lock();
         if (bp && bp->GetRandomMove(qpos, si, mvk)) {
            cout << "----- (Book suggestion from " << Name() << ") -----\n\n";
         }
      }
   }

   return kCheatNone;
}

void CPlayerComputer::GetChosen(const CQPosition& pos, const CSearchInfo& asi, CMVK& mvk, bool fUseBook) {
   int hSolve;
   CSearchInfo si=asi;

   hSolve = pos.NEmpty()-hSolverStart;

   SetParameters(pos, fUseBook, si.iCache);
   QSSERT((mpcs==NULL) || cd.iPruneMidgame<=::mpcs->NPrunes());

   // set search info
   si.iPruneMidgame=cd.iPruneMidgame;
   si.iPruneEndgame=cd.iPruneEndgame;
   si.rs += DefaultRandomness() + cd.nRandShifts[!pos.BlackMove()];
   si.vContempt+=cd.vContempts[!pos.BlackMove()];
   si.fsPrint=cd.fsPrint;

   Initialize(pos.BitBoard(),pos.BlackMove());
   TimedMVK(*search_pcp, si, mvk, mOnline);

   // Print the move
   if (si.PrintMove()) {
      cout << (fTooting?"Predict: ":"=== ");
      cout << setw(2) << pos.NEmpty() << "e: " << mvk;
      cout << (fTooting?"":" ===") << "\n";

      stream.str("");
      stream << (fTooting?"Predict: ":"=== ");
      stream << setw(2) << pos.NEmpty() << "e: " << mvk;
      stream << (fTooting?"":" ===");
   }
}

void CPlayerComputer::Clear() {
   int i;
   for (i=0; i<2; i++)
      if (caches[i])
         caches[i]->Clear();
   solved=false;
}

void CPlayerComputer::StartMatch(const COsMatch& match) {
}

void CPlayerComputer::EndGame(const COsGame& game, Timer<double>& timer) {

   // save the game to the saved game file
   if (fnSaveGame) {
      ofstream os(fnSaveGame,ios::app);
      os << game << "\n";
   }

   // if not full strength, add all games to book.
   // if full strength, add non-rand games to book.
   if (!game.mt.fRand) {
      // if deferAnalysis is set, save game to file to be analyzed later
      //	typically during tournament or when online
      if (FDeferAnalysis()) {
         ofstream os(Name(),ios::app);
         os << game << "\n";
      }
      else if (CBookPtr bp = book.lock()) { // not deferred
         AnalyzeGame(game, timer);
         // Don't worry about transpositions, takes too long.
         //	Will get recalced next time we start the program anyway
         //CNABook();
      }

      // save the book every so often, even if
      //  we're deferring the analysis (because solved nodes
      //	will be saved in the book)
      if (CBookPtr bp = book.lock())
         bp->Mirror();
   }
}

void CPlayerComputer::SetupBook(bool fCNABook) {
   int nSearches=0;
   CBoni boni;

   Clear();
   hasBacktracked=false;
   fInBook=true;

   if (CBookPtr bp = book.lock()) {
      bp->SetBoni(boni);
      SetParameters(true, 0);
      bp->SetComputer(shared_from_this());
      //if (fCNABook)
      //       book->CorrectAll(nSearches);
   }
}

extern bool fPrintCorrections;

void CPlayerComputer::AnalyzeGame(const COsGame& game, Timer<double>& timer)
{
   if (CBookPtr bp = book.lock()) {
      ::book=bp;
      ::cache=GetCache(0);
      ::cache->Prepare();
      Backtrack(game, timer);
      if( fPrintCorrections )
         PrintAnalysis(game);
      bp->Mirror();
   }
}

void CPlayerComputer::Backtrack(const COsGame& game, Timer<double>& timer) {
   int nSearches=0;
   extern bool fPrintCorrections;

   if(CBookPtr bp = book.lock()) {
      bp->SetComputer(shared_from_this());
      bp->CorrectGame(game, game.sPlace!="Local", nSearches, cd.fEdmundAfter, timer, movesToAdd);
      if (fPrintCorrections)
         cout << "Done correcting game. " << nSearches << " searches done.\n";
   }
}

// preconditions:
//	There is a book
//	There is a cache
//	The book has boni
//  The game has been backtracked
void CPlayerComputer::PrintAnalysis(const COsGame& game) {
   int iMove, nMoves;
   CQPosition pos(game, 0);
   CMoves moves;
   CMove move;
   double totalTime=0;
   CMVK chosen;
   CNodeStats start, end;
   CBookData *bd;

   start.Read();
   nMoves=game.ml.size();

   cout << "----------- Analyzing Game -----------\n";
   for (iMove=0; iMove<nMoves; iMove++) {
      CMove mv(game.ml[iMove].mv);
      if (pos.NEmpty()<=hSolverStart)
         break;
      Initialize(pos.BitBoard(),pos.BlackMove());
      cout << "--------- move " << iMove+1 << "--------------\n";
      //Print();
      CBookPtr bp = book.lock();
      if (pos.NEmpty()<60 && pos.CalcMoves(moves) && bp) {
         bd=bp->FindAnyReflection(pos.BitBoard());
         if (!bd) {
            cout << "Position not in book\n";
            QSSERT(0);
         }
         else if (bd->IsLeaf()) {
            cout << *bd << "\n";
         }
         else {
            CSearchInfo si;
            si.rs=0;
            si.vContempt=0;
            si.fsPrint=cd.fsPrint & ~CSearchInfo::kPrintBookRandomInfo;

            if (!bp->GetRandomMove(pos, si, chosen)) {
               QSSERT(0);
            }
         }
      }
      cout << game.pis[!pos.BlackMove()].sName << " played: " << mv << "\n";
      pos.MakeMove(mv);
   }

   // more debug info
   end.Read();
   printf("Analysis complete in %.3lg seconds\n",(end-start).Seconds());
}

void CPlayerComputer::SetParameters(const CQPosition& pos, bool fUseBook, int iCache) {
   // Set up parameters
   ::book = fUseBook ? book.lock() : CBookPtr();
   ::cache=GetCache(iCache);

   // we may want to copy the other cache, if the position is a predecessor
   posCached[iCache]=pos;
   fHasCachedPos[iCache]=true;
   if (caches[!iCache] && fHasCachedPos[!iCache] && posCached[!iCache].IsSuccessor(pos)) {
      caches[iCache]->CopyData(*(caches[!iCache]));
      cout << "Cache copy!\n";
   }

   ::cache->Prepare();
   //::iPruneMidgame = iPruneMidgame;
   //::iPruneEndgame = iPruneEndgame;
   ::mpcs = mpcs;
   eval->Setup();
}

void CPlayerComputer::SetParameters(bool fUseBook, int iCache) {
   // Set up parameters
   ::book = fUseBook ? book.lock() : CBookPtr();
   ::cache=GetCache(iCache);

   ::cache->Prepare();
   //::iPruneMidgame = iPruneMidgame;
   //::iPruneEndgame = iPruneEndgame;
   ::mpcs = mpcs;
   eval->Setup();
}

TMoveSignal CPlayerComputer::MoveSignal() const {
   return kSignalNone;
}

bool CPlayerComputer::IsHuman() const {
   return false;
}

bool CPlayerComputer::FDeferAnalysis() const {
   // return pcp->Strength()>16 && !fAnalyzingDeferred;
   return DEFER_ANALYSIS;
}

// default randomness for computer, dependent upon stone value
int CPlayerComputer::DefaultRandomness() {
   int result, sv;

   for (result=-4, sv=kStoneValue; sv; result++, sv>>=1);
   return result;
}

CPlayerPtr pPlH, pPl1, pPl2;

CPlayerPtr GetPlayer(char c,
                     const CComputerDefaults& cd1,
                     const CComputerDefaults& cd2,
                     bool online,
                     int movesToAdd/*=60*/)
{
   switch (c) {
   case 'h':
      if (!pPlH)
         pPlH.reset(new CPlayerHuman);
      return pPlH;
   case '1':
      if (!pPl1)
         pPl1 = CPlayerComputer::Create(cd1, online, movesToAdd);
      return pPl1;
   case '2':
      if (!pPl2)
         pPl2 = CPlayerComputer::Create(cd2, online, movesToAdd);
      return pPl2;
   default:
      cerr << "Unknown player type " << c << "\n";
      _ASSERT(0);
      exit(-1);
      return CPlayerPtr();
   }
}

void CleanPlayers()
{
   pPlH.reset();
   pPl1.reset();
   pPl2.reset();
}
