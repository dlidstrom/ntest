// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// game routines

#include "PreCompile.h"
#include "time.h"
#include "TreeDebug.h"
#include "Games.h"
#include "NodeStats.h"
#include "QPosition.h"
#include "options.h"
#include "Cache.h"
#include "Book.h"

#include <math.h>
#include <stdio.h>
#include <io.h>
#include <strstream>
#include <fstream>

using namespace std;

// options
bool fPrintBoard=false;
bool printResult=false;
bool fPrintExtensionInfo=false;

string fnOpening;

//////////////////////////////////////////////////////////
// CGame routines:
//	Initialization
//////////////////////////////////////////////////////////

CGame::CGame(CPlayerPtr plBlack, CPlayerPtr plWhite) {
   InitializePlayers(plBlack, plWhite, tMatch);
}

CGame::CGame(CPlayerPtr plBlack, CPlayerPtr plWhite, double t) {
   InitializePlayers(plBlack, plWhite, t);
}

void CGame::Initialize(char sBoard[NN+1], bool fBlackMove) {
   pos.board.TextSet(sBoard);
   pos.board.iMover=!fBlackMove;
   posStart=pos;
   ml.clear();	// in case there was a fixed opening listed
}

void CGame::InitializePlayers(CPlayerPtr plBlack, CPlayerPtr plWhite, double t) {
   Clear();

   // player pointers
   ppls[0]=plBlack;
   ppls[1]=plWhite;

   if (fnOpening.empty() || Load(fnOpening.c_str())) {
      SetDefaultStart();
   }
   else {
      // clear evals and times from move list so we don't have to look at them in the GGF files
      vector<CSGMoveListItem>::iterator pMli;
      for (pMli=ml.begin(); pMli!=ml.end(); pMli++) {
         pMli->tElapsed=pMli->dEval=0;
      }
      // clear game-over flag
      result.Clear();
   }

   SetInfo();

   // clocks
   posStart.cks[0]=posStart.cks[1]=CSGClock(t);
}

void CGame::SetInfo() {
   // players
   pis[0].sName=ppls[0]->Name();
   pis[1].sName=ppls[1]->Name();

   // sDateTime
   char sTime[30];
   time_t tCurrent=time(NULL);
   strftime(sTime, 30, "%Y-%m-%d %H:%M:%S GMT", gmtime(&tCurrent));
   sDateTime=sTime;

   // other
   sPlace="Local";
}

void CGame::SetDefaultStart() {
   // game type
   COsMatchType amt;
   istrstream is("8");
   is >> amt;

   // positions
   mt=amt;
   pos.board.bt=mt.bt;
   pos.board.Initialize();
   posStart=pos;
}

int CGame::Load(const char* fn) {
   ifstream is(fn);
   if (!is) {
      cerr << "can't read file " << fn << "\n";
      return 1;
   }
   else {
      In(is);
      return !is;
   }
}

//////////////////////////////////////////////////////////
// CGame routines:
//	Playing
//////////////////////////////////////////////////////////

// Play a game. Return net number of discs

int CGame::Play(Timer<double>& timer) {
   CSGMoveListItem mli;
   COsMatch match;
   CPlayer::TCheatcode cheatcode;

   // initialize players
   match.pis[0]=pis[0];
   match.pis[1]=pis[1];
   match.sMatchType=mt;
   match.cRated='R';

   ppls[0]->StartMatch(match);
   if (ppls[1]!=ppls[0])
      ppls[1]->StartMatch(match);

   // TODO: Eliminate this
   pos.board.OutFormatted(cout);

   // get moves from players until the game is over
   while (!GameOver()) {
      if (fPrintBoard || ppls[0]->NeedsBoardDisplayed() || ppls[1]->NeedsBoardDisplayed()) {
         cout << "\n";
         pos.board.OutFormatted(cout);
         cout << "\n";
      }

      // toot
      cheatcode = ppls[!pos.board.iMover]->Update(*this, ppls[pos.board.iMover]->IsHuman()?0:CPlayer::kNoPrint, mli);
      QSSERT(cheatcode==CPlayer::kCheatNone);

      // regular player
      cheatcode = ppls[pos.board.iMover]->Update(*this, CPlayer::kMyMove+CPlayer::kAllowCheats, mli);
      switch(cheatcode) {
      case CPlayer::kCheatNone:
         Update(mli);
         break;
      case CPlayer::kCheatUndo:
         Undo();
         break;
      case CPlayer::kCheatAutoplay:
         ppls[pos.board.iMover]=ppls[!pos.board.iMover];
         break;
      case CPlayer::kCheatSwitch:
      {
         CPlayerPtr ppl;
         ppl=ppls[0];
         ppls[0]=ppls[1];
         ppls[1]=ppl;
      }
      case CPlayer::kCheatContinue:
         break;
      default:
         QSSERT(0);
      }
   }

   result.status = COsResult::kNormalEnd;

   // let players finish up
   ppls[0]->EndGame(*this, timer);
   if (ppls[1]!=ppls[0])
      ppls[1]->EndGame(*this, timer);

   return pos.board.NetBlackSquares();
}

//////////////////////////////////////////////////////////
// CGame routines:
//	Protected helper routines
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// Misc routines that will come in useful sometime
//////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// saved games
//////////////////////////////////////////////////////////////////

CSavedGame::CSavedGame(const char* sNewBlackName, const char* sNewWhiteName, bool fNewComplete,
                       bool fNewPublic, int nNewResult, TMoveList mlNew) {
   strncpy(sBlackName, sNewBlackName, 9);
   strncpy(sWhiteName, sNewWhiteName, 9);
   fComplete=fNewComplete;
   fPublic=fNewPublic;
   nResult=nNewResult;
   posStart.Initialize();
   strcpy(ml, mlNew);
}

bool CSavedGame::FPrint(FILE* fp,int nVersion) {
   switch(nVersion) {
   case 2:
      return FPrint2(fp);
   case 3:
      return FPrint3(fp);
   default:
      QSSERT(0);
      return false;
   }
}

bool CSavedGame::FPrint2(FILE* fp) {
   char sScore[4];

   // make the game into text
   if (fComplete)
      sprintf(sScore, "%+3.2d", nResult);
   else
      sprintf(sScore, "---");
   return fprintf(fp, "%c%-8s %s %-8s %s\n", fPublic?'+':'-', sBlackName, sScore, sWhiteName, ml)>0;
}

bool CSavedGame::FPrint3(FILE* fp) {
   char sScore[4], sBoardText[NN+1], cMover;

   // make the game into text
   posStart.GetSBoard(sBoardText);
   cMover=ValueToText(posStart.BlackMove()?BLACK:WHITE);

   if (fComplete)
      sprintf(sScore, "%+3.2d", nResult);
   else
      sprintf(sScore, "---");
   return fprintf(fp, "%c%-8s %s %-8s %s %c %s\n", fPublic?'+':'-',
                  sBlackName, sScore, sWhiteName, sBoardText,cMover,ml)>0;
}

// scan in a game file. version 0 = my original version, 1 = logistello format, 2 = my new version
//	3 = new version with start pos
bool CSavedGame::FScan(FILE* fp, int nVersion) {
   switch(nVersion) {
   case 0:
      return FScan0(fp);
   case 1:
      return FScan1(fp);
   case 2:
      return FScan2(fp, false);
   case 3:
      return FScan2(fp, true);
   default:
      fprintf(stderr, "Invalid version in CSavedGame::FScan\n");
      QSSERT(0);
      return false;
   }
}

bool CSavedGame::FScan0(FILE* fp) {
   char nBlack, nWhite;

   posStart.Initialize();
   if (3>fscanf(fp,"%c%c%[^\n]s\n", &nBlack, &nWhite, ml))
      return false;
   *sBlackName=*sWhiteName=0;
   fPublic=false;
   fComplete=true;
   nResult=nBlack-nWhite;
   return true;
}

bool CSavedGame::FScan1(FILE* fp) {
   char sLogGame[500], *pcCurMove;
   int nMove, nUnknown, nScanned;

   posStart.Initialize();
   nScanned=fscanf(fp,"%s %d %d\n", sLogGame,&nResult,&nUnknown);
   if (nScanned!=3) {
      QSSERT(nScanned==EOF);
      return false;
   }
   strcpy(sBlackName, "logtest");
   strcpy(sWhiteName, "logtest");
   fPublic=true;
   fComplete=true;

   // turn logistello format into movelist format
   for (nMove=0, pcCurMove=sLogGame+1; *pcCurMove; nMove++, pcCurMove+=3)
      ml[nMove]=(toupper(pcCurMove[0])-'A') + (pcCurMove[1]-'1')*N+' ';
   ml[nMove]=0;

   return true;
}

bool CSavedGame::FScan2(FILE* fp, bool fHasStartPos) {
   char sScore[4], cPublic;
   char sBoardText[NN+1];

   ml[0]=0;

   // players, score, public
   if (4>fscanf(fp, "%c%s %s %s", &cPublic, sBlackName, sScore, sWhiteName))
      return false;
   fPublic = (cPublic=='+');
   fComplete=(strcmp(sScore,"---")!=0);
   if (fComplete)
      nResult=atol(sScore);
   else
      nResult=0;

   // start pos if needed
   if (fHasStartPos) {
      char cMover;
      char sFormat[10];
      sprintf(sFormat," %%%ds",NN,"%s");
      if (1>fscanf(fp, sFormat, sBoardText))
         return false;
      if (1>fscanf(fp," %c",&cMover, ml))
         return false;
      posStart.Initialize(sBoardText, TextToValue(cMover)==BLACK);
   }
   else {
      posStart.Initialize();
   }

   // moves list
   if (1>fscanf(fp, " %[^\n]s\n",ml))
      return false;

   return true;
}

////////////////////////////////////////////////////////////
// Series of games
////////////////////////////////////////////////////////////

void Compare(CPlayerComputerPtr computer1, CPlayerComputerPtr computer2) {
   CBitBoard bb;
   FILE* fp;
   int nWins, nDraws, nLosses;
   int i, nNet;

   nWins=nDraws=nLosses=0;

   fp=fopen("CompareStart.pos","rb");
   QSSERT(fp);
   if (fp) {
      while (fread(&bb,sizeof(bb),1,fp)) {
         char sBoardText[NN+1];
         bb.GetSBoardNonnormalized(sBoardText);
         for (i=0; i<2; i++) {
            // clear caches
            computer1->Clear();
            computer2->Clear();

            CGame game(i?computer2:computer1, i?computer1:computer2);
            game.posStart.board.TextSet(sBoardText);
            game.pos=game.posStart;

            Timer<double> timer;
            nNet=game.Play(timer);
            if (i)
               nNet=-nNet;
            if (nNet>0)
               nWins++;
            else if (nNet<0)
               nLosses++;
            else
               nDraws++;
            cout << nNet << (i?'\n':'\t');
         }
      }
      fclose(fp);

      cout << "computer1 has Wins: " << nWins << "  Draws: " << nDraws << "  Losses: " << nLosses << "\n";
   }
}
