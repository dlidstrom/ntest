// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

//////////////////////////////////////////////////////
// CNodeStats class
//////////////////////////////////////////////////////

#pragma once

#ifndef _H_NODESTATS
#define _H_NODESTATS

#include <iostream>

typedef unsigned long u4;
#if _WIN32
typedef __int64 i8;
#else
typedef long long i8;
#endif

extern u4 nEvalsQuick, nSNodesQuick;
extern double nEvals, nSNodes, nINodes, nKFlips, nBBFlips;

class CNodeStats {
public:
   double nINodes, nSNodes, nKFlips, nBBFlips, nEvals;

   i8 time;

   void Read();
   void Out(std::ostream& os) const;
   void OutShort(std::ostream& os) const;

   CNodeStats operator-(const CNodeStats& b) const;
   double Seconds() const;
   double Nodes() const;
};

inline std::ostream& operator<<(std::ostream& os, const CNodeStats& ns) { ns.Out(os); return os; }

// thinking on opponent's time
void WipeNodeStats();
extern bool abortRound;
void SetAbortTime(double seconds);
void ResetAbortTime(double seconds);
bool CheckAbortTime();
bool CheckForOpponentMove();

#endif // _H_NODESTATS
