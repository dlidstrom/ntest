// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// othello program

#include "PreCompile.h"
#include "Debug.h"
#include "Book.h"
#include "options.h"
#include "main.h"
#include "QPosition.h"
#include "Player.h"
#include "Games.h"
#include "NodeStats.h"
#include "Evaluator.h"
#include "Test.h"
#include "TreeDebug.h"
#include "MPCStats.h"
#include "Pos2.h"
#include "FastFlip.h"
#include "ODKStream.h"
#include "Cache.h"
#include "Server.h"
#include "Client.h"
#include "ExtractDraws.h"
#include "ExtractLines.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include <ctype.h>
#include <strstream>
#include <iomanip>
#include <string>
#include <numeric>

#ifndef _WIN32
#define __cdecl
#endif

extern bool fPrintMoveSearch;

enum modality
{
   kSpeedTest,
   kCalcMPC,
   kPosValues,
   kGetStartPos,
   kAssignBook,
   kEdmundBook,
   kJoinBooks,
   kAnalyze,
   kAnalyzeEdmund,
   kGame,
   kGameEdmund,
   kGGS,
   kCompare,
   kExtractLines,
   kExpandBook,
   kExtractDraws
} mode;

#define RANDSEED time(0)
//#define RANDSEED 7

const int defaultMatchMinutes=15;
const int defaultMatchDepth=10;

extern string fnOpening;

void Init() {
   setbuf(stdout, 0);
   srand(static_cast<unsigned>(RANDSEED));

   InitFastFlip();
   InitFastFlipPatterns();
   InitConfigToPotMob();
   cout << setprecision(3);
   cerr << setprecision(3);

   extern void InitNewFF();
   InitNewFF();

#ifdef _DEBUG
   int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
   tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
   _CrtSetDbgFlag( tmpFlag );
#endif

   void InitFFBonus();
   InitFFBonus();
   InitForcedOpenings();
}

void Clean() {
   CleanConfigToPotMob();
   CleanFastFlipPatterns();
   CBook::Clean();
   CEvaluator::Clean();
   CleanPlayers();
}

void PrintStuff(bool fPrintStuff) {
   fPrintExtensionInfo=fPrintStuff;
   fPrintBoard=printResult=fPrintStuff;
   fPrintAbort=fPrintStuff;
}

void Test() {
   CBitBoard::Test();
}

int __cdecl main(int argc, char**argv, char**envp) {
   //_CrtSetBreakAlloc(516625);
   time_t end_time;
   int nGames;
   int nSkipValue;
   int success = EXIT_SUCCESS;
   const char* submode;

   cout << "Copyright (c) Chris Welty\nAll Rights Reserved\n\n";

   ////////////////////////////////////////
   // Table Initialization
   ////////////////////////////////////////

   Init();

   Test();

   CComputerDefaults cd1;
   CComputerDefaults cd2;
   ReadParameters(cd1, cd2);
   ParseCommandLine(argc, argv, submode, cd1, nGames, nSkipValue);

   CNodeStats start;
   CNodeStats end;

   start.Read();

   ////////////////////////////////////////
   // Special modes requiring no players
   ////////////////////////////////////////

   if (mode==kSpeedTest) {
      PrintStuff(false);
      TestMoveSpeed(cd1.MinutesOrDepth(), nGames, argv[1]+1);
   }
   else {

      ///////////////////////////////////////////
      // All other modes
      ///////////////////////////////////////////

      fPrintTimeUsed = fPrintMoveSearch = false;

      switch(mode) {
      case kExpandBook:
      {
         if( argc<6 ) {
            cerr << "Usage: " << argv[0] << " b11 sXX bound skip-count [server server-port] | [client server-address server-port]" << std::endl;
            success = EXIT_FAILURE;
         }
         else {
            std::string submode(argv[5]);
            if( submode=="server" ) {
               success = Server(argc, argv, cd1, nGames);
            }
            else if( submode=="client" ) {
               success = Client(argc, argv, cd1);
            }
            else {
               cerr << "Unknown sub-mode: " << submode << std::endl;
               success = EXIT_FAILURE;
            }
         }
      } break;
      case kExtractDraws:
      {
         success = ExtractDraws(argc, argv, submode, cd1, cd2);
      } break;
      case kExtractLines:
      {
         success = ExtractLines(argc, argv, submode, cd1, cd2, nGames);
      } break;
      case kGame:
      case kGameEdmund:
      {
         PrintStuff(true);

         cd1.fEdmundAfter=cd2.fEdmundAfter=(mode==kGameEdmund);
#if defined(_DEBUG)
         CPlayerPtr p0=GetPlayer(submode[0],cd1,cd2, true);
         CPlayerPtr p1=GetPlayer(submode[1],cd1,cd2, true);
#else
         CPlayerPtr p0=GetPlayer(submode[0],cd1,cd2, false);
         CPlayerPtr p1=GetPlayer(submode[1],cd1,cd2, false);
#endif

         if( CPlayerComputerPtr cp1 = std::dynamic_pointer_cast<CPlayerComputer>(p0) ) {
            DrawCount dc = CountDraws(*cp1->book.lock(), std::cout);
         }

         bool fAlternate = strlen(submode)>2 && submode[2]=='*';

         Timer<double> timer;
         while (nGames-->0) {
            bool fSwap = fAlternate && (nGames&1)==1;
            if (fSwap)
               CGame(p1, p0).Play(timer);
            else
               CGame(p0, p1).Play(timer);
         }
         break;
      }
      case kCompare:{
         PrintStuff(false);
         fCompareMode=true;

         cd2.sCalcParams=cd1.sCalcParams;
         cd1.booklevel=cd2.booklevel=CComputerDefaults::kNoBook;

         CPlayerComputerPtr computer1 = CPlayerComputer::Create(cd1, false);
         CPlayerComputerPtr computer2 = CPlayerComputer::Create(cd2, false);
         Compare(computer1, computer2);
         break;
      }
      case kCalcMPC: {
         cd1.booklevel=CComputerDefaults::kNoBook;
         CPlayerComputerPtr computer1 = CPlayerComputer::Create(cd1, false);
         CalcMPCStats(computer1, cd1.MinutesOrDepth());
         break;
      }
      case kPosValues: {
         cd1.booklevel=CComputerDefaults::kNoBook;
         CPlayerComputerPtr computer1 = CPlayerComputer::Create(cd1, false);
         CalcPosValues(computer1, submode[0]=='a');
         break;
      }
      case kEdmundBook: {
         cd1.booklevel=CComputerDefaults::kNegamaxBook;
         cd1.fEdmundAfter = true;
         CPlayerComputerPtr computer1 = CPlayerComputer::Create(cd1, false);
         PrintStuff(true);
         if (CBookPtr bp = computer1->book.lock()) {
            bp->SetComputer(computer1);
            cout << bp->NEdmundNodes() << " edmund nodes detected.\n";
            bp->PlayEdmundGames();
            int nSearches = 0;
            bp->CorrectAll(nSearches);
         }
         else {
            cerr << "ERR: You need a book to use Edmund mode\n";
         }
         break;
      }

      case kJoinBooks: {
         CPlayerComputerPtr computer1 = CPlayerComputer::Create(cd1, false);
         if (CBookPtr bp = computer1->book.lock()) {
            bp->SetComputer(computer1);
            string fn(fnBaseDir);
            fn+="coefficients/join.book";
            bp->JoinBook(fn.c_str());
         }
         else {
            cerr << "ERR: You need a book to use JoinBooks mode\n";
         }
         break;
      }
      case kGGS:
      {
         // this flag adjusts the opening book strategy
         bool exploreEdmunds = false;
         // this flag adjust the learning strategy
         cd1.fEdmundAfter = false;
         if( cd1.sCalcParams=="s20" ) {
            exploreEdmunds = true;
            cd1.fEdmundAfter = true;
         }
         CPlayerComputerPtr computer1 = CPlayerComputer::Create(cd1, !exploreEdmunds);
         CBookPtr bp = computer1->book.lock();
#if !defined(_DEBUG)
         bool correctAll = true;
#else
         bool correctAll = false;
#endif
         if( correctAll ) {
            int nSearches;
            bp->CorrectAll(nSearches);
         }
         DrawCount dc = CountDraws(*bp, std::cout);
         CODKStream gs;
         std::string login(cd1.sCalcParams);
         std::string passw;
         if( cd1.sCalcParams=="s26" ) {
            passw = "qwptupbz";
         }
         else {
            passw = "1KP1BLe";
         }
         if( (!gs.Connect("skatgame.net",5000) || !gs.Connect("localhost",5000))  &&
             gs.Login(login,passw)==0) {
            gs.pComputer=computer1;

            // Get and display the name of the computer.
            gs << "info " << dc.edmunds << " edmund nodes detected.";
            gs << "\\" << dc.draws << " draws. " << dc.privateDraws << " private draws.";
            gs << "\\Computer name: " << GetComputerNameAsString();
            gs << "\n";
            std::string channel = "." + login;
            gs << "tell " << channel << " " << dc.edmunds << " edmund nodes detected.\\";
            gs << dc.draws << " draws. " << dc.privateDraws << " private draws.\\";
            gs << "Computer name: " << GetComputerNameAsString() << "\n";

            gs.Process();
         }
      }
      break;
      case kAnalyze:
      case kAnalyzeEdmund:
      {
         cd1.fEdmundAfter= mode==kAnalyzeEdmund;
         std::cout << "Analyze edmunds is " << (cd1.fEdmundAfter?"on":"off") << std::endl;
         CPlayerComputerPtr computer1 = CPlayerComputer::Create(cd1, false);
         if (!computer1->book.lock()) {
            cerr << "ERR: You need a book to use Analyze mode\n";
         }
         else {
            PrintStuff(true);

            computer1->fAnalyzingDeferred=true;

            COsGame game;
            int n_games = 0;
            Timer<double> timer;
            switch(submode[0])
            {
            case 'g':
            {
               // timer allows synchronization every couple of hours
               fPrintCorrections = false;
               while (cin >> game && timer.elapsed()<3*3600 ) {
                  if( (rand() % nGames)==0 ) {
                     if (!game.mt.fRand && game.mt.bt.nHeight==8 && !game.mt.fAnti) {
                        computer1->AnalyzeGame(game, timer);
                     }
                  }
                  cout << ++n_games << " analyzed" << endl;
               }
            }
            break;
            case 'i':
               while (game.InIOS(cin))
                  if (!game.mt.fRand)
                     computer1->AnalyzeGame(game, timer);
               break;
            case 'l':
               while (game.InLogbook(cin))
                  if (!game.mt.fRand)
                     computer1->AnalyzeGame(game, timer);
               break;
            case '0':
            case '1':
            case '2':
               while (game.InOldNtest(cin, submode[0]-'0'))
                  if (!game.mt.fRand)
                     computer1->AnalyzeGame(game, timer);
               break;
            case 'k': {
               // special, because file must be opened as binary
#ifdef _WIN32
               const char* fn="allinf.oko";
#else
               const char* fn="base/allinf.oko";
#endif

               ifstream ifs(fn, ios::binary);

               PrintStuff(false);

               int nGames;
               for (nGames=0; game.InLogKitty(ifs); nGames++) {
                  if (!game.mt.fRand) {
                     if (nGames%50==0)
                        cout << "\n" << setw(5) << nGames << " ";
                     computer1->AnalyzeGame(game, timer);
                     cout << ".";
                  }
               }
            }
               break;
            default:
               cout << "Unknown file type '" << submode << "' for analyze mode\n";
               cerr << "Unknown file type '" << submode << "' for analyze mode\n";
            }
         }
         break;
      }
      case kAssignBook: {
         cd1.booklevel=CComputerDefaults::kNegamaxBook;
         CPlayerComputerPtr computer = CPlayerComputer::Create(cd1, false);
         int nSearches;
         computer->book.lock()->CorrectAll(nSearches);
         break;
      }
      default:
         cerr << "that mode is not currently supported\n";
         cout << "that mode is not currently supported\n";
         break;
      }

   }

   extern int nStableChecks;
   extern int nProven;

   cout << nProven << " / " << nStableChecks << " = " << (double)nProven*100/(std::max)(1, nStableChecks) << "% stable checks proved WLD\n";
   //PrintBookReadData();
   time(&end_time);
   printf("\n\nRun completed at GMT %s\n",asctime(gmtime(&end_time)));
   end.Read();
   cout << (end-start) << "\n";

   Clean();

   return success;
}

///////////////////////////////////////
// command-line options
///////////////////////////////////////

void ParseCommandLine(int argc,
                      char* argv[],
                      const char*& submode,
                      CComputerDefaults& cd,
                      int& nGames,
                      int& nSkipValue)
{
   // defaults
   submode='\0';
   nGames=1;

   // overrides by command line
   if (argc>1) {
      submode=argv[1];
      switch(*(submode++)) {
      case 'a': mode=kAnalyze; break;
      case 'A': mode=kAnalyzeEdmund; break;
      case 'c': mode=kCompare; break;
      case 'e': mode=kEdmundBook; break;
      case 'g': mode=kGame; break;
      case 'G': mode=kGameEdmund; break;
#ifndef EXTERNAL
      case 'i': mode=kGGS; break;
      case 'j': mode=kJoinBooks; break;
#endif
      case 'm': mode=kCalcMPC; break;
      case 'n': mode=kAssignBook; break;
      case 'p': mode=kPosValues; break;
         //case 's': mode=kGetStartPos; break;
      case 't': mode=kSpeedTest; break;
      case 'x': mode=kExtractLines; break;
      case 'b': mode=kExpandBook; break;
      case 'd': mode=kExtractDraws; break;

      default:
         cerr << "unknown or dangerous mode '" << argv[1] << "'\n";
         cout << "unknown or dangerous mode '" << argv[1] << "'\n";
         _exit(1);
      }
   }

   // argument 2 : time control
   double tMatch=defaultMatchMinutes*60;

   if (argc>2) {
      cd.sCalcParams=argv[2];
   }

   // argument 3 : number of games in a series
   if (argc>3) {
      int nGamesTemp=atol(argv[3]);
      if (nGamesTemp>=0)
         nGames=nGamesTemp;
      else
         fprintf(stderr, "The third argument (number of games) must be greater than or equal to 0!\n");
   }

   // argument 4: opening file name
   if( mode!=kExtractLines && mode!=kExpandBook && argc>4) {
      fnOpening=argv[4];
   }
   if( mode==kExpandBook ) {
      if( argc>4 )
         nSkipValue = atol(argv[4]);
      else
         std::cerr << "The fourth argument must be skip value (int)." << std::endl;
   }
}

void ReadMachineParameters(istream& isBig) {
   string sLine;
   getline(isBig, sLine);
   istrstream is(sLine.c_str());

   // read the following parameters:
   //	int maxCacheMem - size in bytes available for cache (in file in MB,converted here to bytes)
   //	double dGHz - Approx processor speed

   //	first set default values in case we can't read for some reason
   maxCacheMem=10;
   dGHz=0.4;

   is >> maxCacheMem >> dGHz;

   // convert MBytes to bytes in maxCacheMem
   maxCacheMem<<=20;
}

void ReadParameters(CComputerDefaults& cd1, CComputerDefaults& cd2) {
   string fn(fnBaseDir);
   fn+="parameters.txt";
   ifstream fParams(fn.c_str());

   ReadMachineParameters(fParams);
   fParams >> cd1 >> cd2;
}
