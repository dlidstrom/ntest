// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// options for the compilation

#pragma once

#ifndef _H_OPTIONS
#define _H_OPTIONS

#include <stdio.h>
#include "Fwd.h"

// if COUNTNODES is 1, we count a node every time we flip a disc
//	if it is 2, we also count a node when nEmpty==1 and we merely
//	count the number of discs that would be flipped (if at least one
//	would be flipped).
const int COUNTNODES=2;
#define COUNT_FLIP { if (COUNTNODES) nSNodesSinceCheck++; }
#define COUNT_COUNT { if (COUNTNODES>2) nSNodesSinceCheck++; }

extern int hSolverStart;	// number of empty squares when we start solver
#ifdef _DEBUG
extern bool fPrintTree;
#else
const bool fPrintTree=false;
#endif

extern bool fPrintTimeUsed;
extern bool fPrintWLD;
extern bool fPrintMPCStats;
extern bool fPrintMoveSearch;
extern bool fCompareMode;
extern int treeNEmpty;
extern std::weak_ptr<CBook> book;

// search parameters
extern CCache* cache;
extern CEvaluator* evaluate;
extern CMPCStats* mpcs;

extern bool fSolvedAreMinimal;

extern double tMatch;	// total time available for a match
extern double dGHz;	// processor speed
const double dNPS=400000;	// approx midgame nodes per second on a 1GHz machine

extern bool fPrintAbort;	// print abort message when aborting search?

// maximum amount of memory to allocate to cache table.
//	should be a bit less than the total RAM on the computer.
extern int maxCacheMem;
extern double tSetStale;

// is it my move? Am I thinking on opponent's time?
extern bool fMyMove, fTooting;
typedef enum {kSignalNone, kSignalStdin, kSignalGGS} TMoveSignal;
extern TMoveSignal moveSignalOpponent;

void SetMatchTime(double atMatch);

// captured positions
extern FILE* cpFile;
extern int nCapturedPositions;

// directory
extern const char* fnBaseDir;

// search params
extern int hNegascout;

#endif // _H_OPTIONS