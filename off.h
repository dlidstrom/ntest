// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#pragma once

#include "Debug.h"

class CHAB {
public:
   int nEmpty;
   int alpha;
   int beta;
   int nDiscDiff;
};

class CHABM;
typedef int TFHABM(const CHABM& habm);

class CHABM {
public:
   int height;
   int alpha;
   int beta;
   int nEmpty;
   int iPrune;
   int iff;
   bool fNegascout;
   TFHABM* func;

   CHABM() {};
   CHABM(int aheight, int aalpha, int abeta, int anEmpty, int aiPrune, int aiff, bool afNegascout);
   void DecreasePass(const CHABM& hab2);
   void Decrease(const CHABM& hab2);
   void DecreasePass(const CHABM& hab2, int vSearchAlpha, int vSearchBeta);
   void Decrease(const CHABM& hab2, int vSearchAlpha, int vSearchBeta);
   void Pass(const CHABM& hab2);

   bool OK() const { return alpha<beta && height>=0 && height<=nEmpty; }
};

inline CHABM::CHABM(int aheight, int aalpha, int abeta, int anEmpty, int aiPrune, int aiff, bool afNegascout) :
   height(aheight), alpha(aalpha), beta(abeta), nEmpty(anEmpty), iPrune(aiPrune), iff(aiff), fNegascout(afNegascout) {
}

inline void CHABM::DecreasePass(const CHABM& hab2) {
   height=hab2.height-1;
   nEmpty=hab2.nEmpty-1;
   iPrune=hab2.iPrune;
   alpha=hab2.alpha;
   beta=hab2.beta;
   iff=0;
   fNegascout=false;
   _ASSERT(OK());
}

inline void CHABM::Decrease(const CHABM& hab2) {
   height=hab2.height-1;
   nEmpty=hab2.nEmpty-1;
   iPrune=hab2.iPrune;
   alpha=-hab2.beta;
   beta=-hab2.alpha;
   iff=0;
   fNegascout=false;
   _ASSERT(OK());
}

inline void CHABM::DecreasePass(const CHABM& hab2, int vSearchAlpha, int vSearchBeta) {
   height=hab2.height-1;
   nEmpty=hab2.nEmpty-1;
   iPrune=hab2.iPrune;
   alpha=vSearchAlpha;
   beta=vSearchBeta;
   iff=0;
   fNegascout=false;
   _ASSERT(OK());
}

inline void CHABM::Decrease(const CHABM& hab2, int vSearchAlpha, int vSearchBeta) {
   height=hab2.height-1;
   nEmpty=hab2.nEmpty-1;
   iPrune=hab2.iPrune;
   alpha=-vSearchBeta;
   beta=-vSearchAlpha;
   iff=0;
   fNegascout=false;
   _ASSERT(OK());
}

inline void CHABM::Pass(const CHABM& hab2) {
   height=hab2.height;
   nEmpty=hab2.nEmpty;
   iPrune=hab2.iPrune;
   alpha=-hab2.beta;
   beta=-hab2.alpha;
   iff=hab2.iff;
   fNegascout=hab2.fNegascout;
}

void InitEmpties();
int AB_BlackToMove(const CHAB& hab);
int AB_WhiteToMove(const CHAB& hab);

// internal use
typedef unsigned long u4;
typedef int TFF(const CHAB& hab);
typedef int TFFM(const CHABM& habm);
typedef int TFnEval(u4 MP, u4 MO);
typedef int TCF();

int BadFF(const CHAB& hab);
int BadFFM(const CHABM& habm);
int BadCF();
void ClearFfs(TFF *ffs[7][7]);
void SetFfs(TFF *ffs[7][7], TFF* fftable[], int length, int loc, bool fMover);
void ClearFFMs(TFFM *ffms[7][7]);
void SetFFMs(TFFM *ffs[7][7], TFFM* fftable[], int length, int loc, bool fMover);
int lengthToSize(int length);
void ConfigToTrits(int config, int length, int* trits);
int NFlipped(int config, int length, int loc, bool fMover, int& left, int& right);
