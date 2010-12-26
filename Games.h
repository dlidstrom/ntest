// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// game header routines

#pragma once

#include "Player.h"
#include "Moves.h"
#include "NodeStats.h"
#include "GDK/OsObjects.h"

#include <vector>
#include <stdio.h>

// options
extern bool fPrintBoard, printResult, printSeriesResult, printSeriesProgress, fPrintExtensionInfo;
extern char* saveGameFile;

void Compare(CPlayerComputerPtr computer1, CPlayerComputerPtr computer2);

class CGame : public COsGame {
public:
   CGame(CPlayerPtr plBlack, CPlayerPtr plWhite);
   CGame(CPlayerPtr plBlack, CPlayerPtr plWhite, double t);

   void InitializePlayers(CPlayerPtr plBlack, CPlayerPtr plWhite, double t);
   void Initialize(char sBoard[NN+1], bool fBlackMove);

   int Play(Timer<double>& timer);

   int Load(const char* fn);
   void SetInfo();
   void SetDefaultStart();

   //void SetStartPos(const CBitBoard& bb);

protected:
   CPlayerPtr ppls[2];
};

// Saved Games
class CSavedGame {
public:
   char sBlackName[9], sWhiteName[9];
   bool fComplete, fPublic;
   int nResult;
   CQPosition posStart;
   TMoveList ml;

   CSavedGame();
   CSavedGame(const char* sNewBlackName, const char* sNewWhiteName, bool fNewComplete, bool fNewPublic, int nNewResult, TMoveList mlNew);
   bool FPrint(FILE* fp, int nVersion=3);
   bool FScan(FILE* fp, int nVersion=3);

protected:
   bool FScan0(FILE* fp);
   bool FScan1(FILE* fp);
   bool FScan2(FILE* fp, bool fStartPos);

   bool FPrint2(FILE* fp);
   bool FPrint3(FILE* fp);
};

inline CSavedGame::CSavedGame() {};
