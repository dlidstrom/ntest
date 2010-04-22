// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// source file for options

#include "PreCompile.h"
#include "Utils.h"
#include "options.h"

int hSolverStart=8;

int treeNEmpty;

#if _WIN32
const char* fnBaseDir = "";
#else
const char* fnBaseDir = "./";
#endif

// timing paramters
double tMatch;	// total time available for a match
double dGHz;	//	double dGHz - Approx processor speed

// maximum memory for cache
int maxCacheMem=25<<20; // can be up to 90<<20 on 128MB NT machine
double tSetStale=0.17;	// time to SetStale() in seconds... so we don't lose on time

// opponent's move?
bool fTooting=false;
bool fMyMove=false;
TMoveSignal moveSignalOpponent=kSignalStdin;

// book-do solved nodes count as minimal nodes?
bool fSolvedAreMinimal=true;

// search parameters
std::tr1::weak_ptr<CBook> book;
CCache* cache=0;
CEvaluator* evaluate=0;
CMPCStats* mpcs=0;
int hBookRead=0;

bool fPrintAbort=true;

#ifdef _DEBUG
bool fPrintTree=false;
#endif
bool fPrintTimeUsed=false;
bool fPrintWLD=false;
bool fPrintMoveSearch=false;
bool fCompareMode=false;

void SetMatchTime(double aMatchTime) {
   if (aMatchTime<=0)
      tMatch=10+maxCacheMem*2E-9/dGHz*60;
   else
      tMatch=aMatchTime;
}

//Captured positions
FILE* cpFile;
int nCapturedPositions=0;

int hNegascout=6;
