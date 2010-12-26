// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "Timing.h"
#include "options.h"
#include <math.h>
#include "Debug.h"
#include <iomanip>
#include <strstream>

// base class. most subclasses don't set abort time, so we won't need to override this

bool CCalcParams::RoundOK(const CHeightInfo& hi, int nEmpty, double tElapsed, double tRemaining) const {
   return hi<=MinHeight(nEmpty);
}

void CCalcParams::SetAbortTime(const CNodeStats& nsStart, int nEmpty, double tRemaining) const {
   ::SetAbortTime(1e6);
}

int CCalcParams::LogCacheSize(int aPrune) const {
   return 60;	// ridiculously large cache size will be cut down to max cache size
}

void CCalcParams::Name(std::ostream& os) const {
   Out(os);
}

CHeightInfo CCalcParams::MinHeight(int nEmpty) const {
   return CHeightInfo(1, 4, false, nEmpty);
}

CCalcParamsPtr CCalcParams::NewFromString(const std::string& sCalcParams, bool search) {
   char c;
   int nMinutesOrDepth, nEmptyMinSolve;
   CCalcParamsPtr pcp;
   std::istrstream is(sCalcParams.c_str());

   if (is >> c) {
      nMinutesOrDepth=15;
      is >> nMinutesOrDepth;

      switch(c) {
      case 'f': pcp.reset(new CCalcParamsFixedHeight(CHeightInfo(nMinutesOrDepth,4,false))); break;
      case 's': pcp.reset(new CCalcParamsStandard(nMinutesOrDepth, search)); break;
      case 't': pcp.reset(new CCalcParamsTurbo(nMinutesOrDepth)); break;
      case 'm': pcp.reset(new CCalcParamsMatchTime()); tMatch=60*nMinutesOrDepth; break;
      case 'a':
         nEmptyMinSolve=0;
         is >> c >> nEmptyMinSolve;
         pcp.reset(new CCalcParamsAverageTime(nMinutesOrDepth,nEmptyMinSolve));
         break;
      }
   }
   if (pcp==NULL) {
      std::cerr << "Unable to parse time control parameter " << sCalcParams.c_str() << "\n";
      exit(-2);
   }

   return pcp;
}

//////////////////////////////////////
// CCalcParamsFixedHeight
//////////////////////////////////////

CCalcParamsFixedHeight::CCalcParamsFixedHeight(CHeightInfo hi) {
   hiMax=hi;
}

void CCalcParamsFixedHeight::SetHeight(CHeightInfo hi) {
   hiMax=hi;
}

int CCalcParamsFixedHeight::LogCacheSize(int aPrune) const {
   if (aPrune)
      return 17+(hiMax.height-12)/2;	// appropriate for MPC searcher
   else
      return 7+(hiMax.height*3)/2;	// appropriate for full-width searcher
}

void CCalcParamsFixedHeight::Out(std::ostream& os) const {
   os << "f" << hiMax.height;
}

CHeightInfo CCalcParamsFixedHeight::MinHeight(int nEmpty) const {
   return hiMax;
}

int CCalcParamsFixedHeight::Strength() const {
   return hiMax.height;
}

int CCalcParamsFixedHeight::PerfectSolve() const
{
   return 0;
}

//////////////////////////////////////
// CCalcParamsFixedNEmpty
//////////////////////////////////////

CCalcParamsFixedNEmpty::CCalcParamsFixedNEmpty(CHeightInfo hi) {
   hiMax=hi;
}

void CCalcParamsFixedNEmpty::Out(std::ostream& os) const {
   os << "e" << hiMax.height;
}

CHeightInfo CCalcParamsFixedNEmpty::MinHeight(int nEmpty) const {
   int hSolve=nEmpty-hSolverStart;
   return hiMax+hSolve;
}

int CCalcParamsFixedNEmpty::Strength() const {
   return 22;
}

int CCalcParamsFixedNEmpty::PerfectSolve() const
{
   return 0;
}

//////////////////////////////////////
// CCalcParamsStandard
//////////////////////////////////////

CCalcParamsStandard::CCalcParamsStandard(int ahMidgame, bool search) {
   hMidgame=ahMidgame;
   CalcHeights(search);
}

CHeightInfo CCalcParamsStandard::MinHeight(int nEmpty) const {
   int i;

   for (i=0; i<=5 && nEmpty>hWLD[i]; i++);
   switch(i) {
   case 6:
      return CHeightInfo(hMidgame, 4, false, nEmpty);
   case 0:
      return CHeightInfo(nEmpty-hSolverStart, 0, nEmpty>hWLD[i]-4, nEmpty);
   default:
      return CHeightInfo(nEmpty-hSolverStart, i, true, nEmpty);
   }
}

void CCalcParamsStandard::CalcHeights(bool search) {
   int i;
   // cIncrease = ln(time to depth D+1/time to depth D)
   const double cIncrease[6]= { 0.738, 0.601, 0.549, 0.495, 0.438, 0.400 };
   const double cIncreaseMidgame = 0.500;
   const int add = (search ? -2 : 0);

   // times are roughly equal at depth 8
   for (i=0; i<=5; i++)
      hWLD[i]=15+(hMidgame-6)*cIncreaseMidgame/cIncrease[i] + add;

   // increase solve height since we won't have to calculate a deviation
   // check that heights are still in order
   hWLD[0]++;
   for (i=1; i<=5; i++) {
      if (hWLD[i]<hWLD[0])
         hWLD[i]=hWLD[0];
   }

   // above ~26 empties, time to reach probable solve levels 5 and 4 are dominated
   //	by the midgame search, so they take about the same amount of time. Therefore
   //	set heights equal
   if (hWLD[5]>26) {
      if (hWLD[4]>=26)
         hWLD[5]=hWLD[4];
      else
         hWLD[5]=26;
   }

   extern int nSolvePct[6];

   std::cout << "---- Heights ----\n";
   std::cout << "midgame : " << hMidgame << "\n";
   for (i=0; i<=5; i++)
      std::cout << std::setw(3) << nSolvePct[i] << "% WLD: " << hWLD[i] << "\n";
   std::cout << "\n";
}

int CCalcParamsStandard::LogCacheSize(int aPrune) const {
   if (aPrune)
      return 17+(hMidgame-12)/2;	// appropriate for MPC searcher
   else
      return 7+(hMidgame*3)/2;	// appropriate for full-width searcher
}

void CCalcParamsStandard::Out(std::ostream& os) const {
   os << "s" << hMidgame;
}

void CCalcParamsStandard::Name(std::ostream& os) const {
   switch(hMidgame) {
   case  2: os << "pamphlet"; break;
   case  3: os << "n2"; break;
   case  8: os << "leaflet"; break;
   case 12: os << "booklet"; break;
   default: Out(os);
   }
}

int CCalcParamsStandard::Strength() const {
   return hMidgame;
}

int CCalcParamsStandard::PerfectSolve() const
{
   return this->hWLD[0];
}

//////////////////////////////////////
// CCalcParamsTurbo
//////////////////////////////////////

CCalcParamsTurbo::CCalcParamsTurbo(int ahMidgame) : CCalcParamsStandard(ahMidgame+4) {
   hMidgame=ahMidgame;
}

void CCalcParamsTurbo::Out(std::ostream& os) const {
   os << "t" << hMidgame;
}

void CCalcParamsTurbo::Name(std::ostream& os) const {
   switch(hMidgame) {
   case 12: os << "booklet+"; break;
   default: Out(os);
   }
}

int CCalcParamsTurbo::Strength() const {
   return hMidgame+4;
}

//////////////////////////////////////
// CCalcParamsAverageTime
//////////////////////////////////////

CCalcParamsAverageTime::CCalcParamsAverageTime(double atAverage) {
   tAverage=atAverage;
   nEmptyMinSolve=0;
}

CCalcParamsAverageTime::CCalcParamsAverageTime(double atAverage, int anEmptyMinSolve) {
   tAverage=atAverage;
   nEmptyMinSolve=anEmptyMinSolve;
}

void CCalcParamsAverageTime::SetAbortTime(const CNodeStats& nsStart, int nEmpty, double tRemaining) const {
   ::SetAbortTime(TTypical(nEmpty)*1.5);
}

bool CCalcParamsAverageTime::RoundOK(const CHeightInfo& hi, int nEmpty, double tElapsed, double tRemaining) const {
   if (nEmpty<=nEmptyMinSolve && !hi.ExactSolved(nEmpty)) {
      ::ResetAbortTime(1e6);
      return true;
   }
   ::ResetAbortTime(TTypical(nEmpty)*1.5);
   return (tElapsed<=TTypical(nEmpty)*0.5);
}

double CCalcParamsAverageTime::TTypical(int nEmpty) const {
   return fInBook?tAverage+tAverage:tAverage;
}

int CCalcParamsAverageTime::LogCacheSize(int aPrune) const {
   return log(dNPS*dGHz*tAverage*0.5)/log(2.0);
}

void CCalcParamsAverageTime::Out(std::ostream& os) const {
   os << "a" << tAverage;
   if (nEmptyMinSolve)
      os << ";" << nEmptyMinSolve;
}

int CCalcParamsAverageTime::Strength() const {
   return LogCacheSize(4)*2-20;
}

int CCalcParamsAverageTime::PerfectSolve() const
{
   return 0;
}

//////////////////////////////////////
// CCalcParamsMatchTime
//////////////////////////////////////

void CCalcParamsMatchTime::SetAbortTime(const CNodeStats& nsStart, int nEmpty, double tRemaining) const {
   ::SetAbortTime(Min(TTypical(nEmpty, tRemaining)*2.0,tRemaining*0.5));
}

bool CCalcParamsMatchTime::RoundOK(const CHeightInfo& hi, int nEmpty, double tElapsed, double tRemaining) const {
   double tt = TTypical(nEmpty, tRemaining);
   double tStop=tt*0.5;
   double tStopSpecial=tt*.3;

   if (hi.WLDSolved(nEmpty)) {
      if (tElapsed>tStopSpecial && tElapsed<=tStop && false)
         std::cout << "special stop criterion for WLD round\n";
      return tElapsed<=tStopSpecial;
   }
   else
      return tElapsed<=tStop;
}

double CCalcParamsMatchTime::TTypical(int nEmpty, double tRemaining) const {
   double t;

   // subtract buffer for clearing cache
   tRemaining-=nEmpty*tSetStale;

   // adjust remaining time if we are running out
   if(tRemaining<=6)
      tRemaining=2;
   else if (tRemaining<=10)
      tRemaining=3;
   else if (tRemaining<=20)
      tRemaining=10;
   
   if (nEmpty>=26)
      t=tRemaining*2/(nEmpty-22);
   else
      t=tRemaining*0.5;

   return fInBook?t+t:t;
}

void CCalcParamsMatchTime::Out(std::ostream& os) const {
   os << "m";
}

void CCalcParamsMatchTime::Name(std::ostream& os) const {
   os << "ntest";
}

int CCalcParamsMatchTime::Strength() const {
   return 20;
}

int CCalcParamsMatchTime::PerfectSolve() const
{
   return 0;
}
