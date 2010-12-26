// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "TreeDebug.h"

#include <iostream>
#include <iomanip>
using namespace std;

int depth=0;


void TreeDebugStartRound(CHeightInfo hi) {
   cout << setfill('-') << setw(30) << "\n" << setfill(' ');
   cout << "Starting round with height=" << hi.height << ", iPrune=" << hi.iPrune << ", fWLD=" << hi.fWLD << "\n";
}

// before we make a move to value a subtree
// We call this routine after we make the move, so fBlackMove is reversed.
void TreeDebugBefore(int height, CMove& move, bool fBlackMove, CValue alpha, CValue beta, bool fNegascout) {
   cout  << setw(depth+1) << '-' << (fBlackMove?"w ":"b ") << move << "(" << alpha << "," << beta ;
   if (fNegascout) cout << ", negascout";
   cout << "):\n";
   depth++;
}

void TreeDebugAfter(int height, CMove& move, bool fBlackMove, CValue alpha, CValue beta, CValue value, bool fNegascout) {
   depth--;
   cout  << setw(depth+1) << '.' << (fBlackMove?"w ":"b ") << move << "(" << alpha << "," << beta;
   if (fNegascout) cout << ", negascout";
   cout << ") " << nBBFlips << "n: " << value << "\n";
}

void TreeDebugCallSolver(int alpha, int beta, bool fBlackMove, int result) {
   cout << setw(depth+1) << 's';
   cout << "-NovelloSolve(" << -beta << ", " << -alpha << ", " << (fBlackMove?"black":"white") << ") = " << result << "\n";
}

void PrintTreeCutoff(CValue best, CValue beta) {
   cout << setw(depth+1) << '[' << "cutoff " << best << ">=" << beta << "]";
   if (beta==kInfinity)
      cout << "infinity cut off!";
   cout << "\n";
}

void PrintTreeCache(CValue best, CValue beta, int height) {
   printf(" [cache value %hd, beta=%hd, height=%d]",best,beta,height);
}
