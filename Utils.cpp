// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// source code for utils

#include "PreCompile.h"
#include "Utils.h"
#include "options.h"
#include "Pos2.h"
#include "Debug.h"

#include <stdio.h>
#include <string.h>
#include <iomanip>
#include <sstream>

void str_upper(char *str) {
   for(;*str; str++) {
      *str = toupper(*str);
   }
}


// print a prompt, and repeatedly get lines until a character in answers is pressed on the keyboard
//	answers must be upper-case letters only.
// print the prompt again after every repeatAfter lines are entered, or never if repeatAfter==0.
// print the prompt to file fp (usually STDERR or STDOUT).

char GetAnswer(const char* prompt, const char* answers, int repeatAfter, FILE* fp) {
   char input[500], *finder;
   int ntimes;

   while (1) {
      fprintf(fp, prompt);
      for (ntimes=0; (ntimes<repeatAfter) || !repeatAfter; ntimes++)  {
         scanf("%499s",input);
         str_upper(input);
         if (finder=strpbrk(input, answers))
            return *finder;
      }
   }
}

u2 CountBits(u4 a) {
   return CountBitsInline(a);
}

u2 CountBits(u8 a) {
   return CountBitsInline(a);
}

char ValueToText(int value) {
   switch(value) {
   case WHITE: return 'O';
   case EMPTY: return '-';
   case BLACK: return '*';
   default: QSSERT(0); return '?';
   }
}

int TextToValue(char c) {
   switch(toupper(c)) {
   case 'O':
   case '0':
   case 'W':
      return WHITE;
   case '*':
   case 'X':
   case '+':
   case 'B':
      return BLACK;
   case '.':
   case '_':
   case '-':
      return EMPTY;
   default:
      QSSERT(0);
      return EMPTY;
   }
}

////////////////////////////////////////////////////
// HeightInfo - used in books and iterative value
////////////////////////////////////////////////////

CHeightInfo::CHeightInfo() {
}

CHeightInfo::CHeightInfo(int aheight, int aiPrune, bool afWLD){
   height=aheight;
   iPrune=aiPrune;
   fWLD=afWLD;
   fKnownSolve=false;
}

CHeightInfo::CHeightInfo(int aheight, int aiPrune, bool afWLD, int nEmpty){
   height=aheight;
   iPrune=aiPrune;
   fWLD=afWLD;
   SetNEmpty(nEmpty);
}

void CHeightInfo::Clear() {
   height=iPrune=fWLD=0;
}

const bool fSkipLastFW=true;

bool CHeightInfo::NextRound(int nEmpty, const CSearchInfo& si) {
   int hSolve=nEmpty-hSolverStart;
   int hMargin=hSolve-height;

   // special rules for calcing MPC stats
   if (si.fNeeds & kNeedMPCStats) {
      height++;
      iPrune=(height>13)?4:0;
      return hMargin>=1;
   }

   // currently in midgame?
   if (hMargin>0) {
      do {
         height++;
         hMargin=hSolve-height;
      }
      while (false);

      // start endgame?
      if (hMargin==0) {
         height=hSolve;
         iPrune=si.iPruneEndgame;
         fWLD=true;
      }
      // stay in midgame
      else {
         fWLD=false;
      }
   }

   // currently in endgame
   else {
      if (fWLD && !(iPrune==1 && fSkipLastFW))
         fWLD=false;
      else {
         iPrune--;
         fWLD=true;
      }
   }
   // save data for printouts
   if (si.fNeeds&kNeedRandSearch)
      fWLD=false;
   SetNEmpty(nEmpty);
   return iPrune>=0;
}

bool CHeightInfo::operator==(const CHeightInfo& b) const {
   return Order()==b.Order();
}

bool CHeightInfo::operator<(const CHeightInfo& b) const {
   return Order()<b.Order();
}

bool CHeightInfo::operator>(const CHeightInfo& b) const {
   return Order()>b.Order();
}

bool CHeightInfo::operator<=(const CHeightInfo& b) const {
   return Order()<=b.Order();
}

bool CHeightInfo::operator>=(const CHeightInfo& b) const {
   return Order()>=b.Order();
}

bool CHeightInfo::operator!=(const CHeightInfo& b) const {
   return Order()!=b.Order();
}

i4 CHeightInfo::Order() const {
   return (height<<8) - (iPrune<<1) - fWLD;
}

bool CHeightInfo::WLDSolved(int nEmpty) const {
   if (height<nEmpty-hSolverStart)
      return false;
   if (iPrune)
      return false;
   return true;
}

bool CHeightInfo::ExactSolved(int nEmpty) const {
   if (height<nEmpty-hSolverStart)
      return false;
   if (iPrune)
      return false;
   if (fWLD)
      return false;
   return true;
}

void CHeightInfo::SetNEmpty(int nEmpty) {
   int hSolve=nEmpty-hSolverStart;
   fKnownSolve=height>=hSolve;
}

CHeightInfo CHeightInfo::operator-(int n) const {
   CHeightInfo result;
   result=*this;
   result.height-=n;
   return result;
}

CHeightInfo CHeightInfo::operator+(int n) const {
   CHeightInfo result;
   result=*this;
   result.height+=n;
   return result;
}

int nSolvePct[6]={100, 97, 91, 70, 60, 50};

void CHeightInfo::Out(ostream& os) const {
   if (fKnownSolve)
      os << std::setw(3) << nSolvePct[iPrune] << '%' << (fWLD?'W':' ');
   else
      os << std::setw(3) << height <<":"<<iPrune;
}

ostream& operator<<(ostream& os, const CHeightInfo& hi) {
   hi.Out(os);
   return os;
}

int Sign(int n) {
   if (n<0)
      return -1;
   else
      return n>0;
}

std::string format_time(int seconds)
{
   std::ostringstream stream;
   if( seconds > 3600 ) {
      int hour = (seconds+1800)/3600; // round up
      stream
         << hour
         << (hour>1?" hours":" hour")
         ;
   }
   else if( seconds > 120 ) {
      int min = (seconds+30)/60; // round up
      stream
         << min
         << (min>1?" minutes":" minute")
         ;
   }
   else {
      stream
         << seconds
         << (seconds>1?" seconds":" second")
         ;
   }

   return stream.str();
}

std::string GetComputerNameAsString()
{
   std::string computerName;

   const int INFO_BUFFER_SIZE = 32767;
   TCHAR infoBuf[INFO_BUFFER_SIZE];
   DWORD bufCharCount = INFO_BUFFER_SIZE;
   if( GetComputerName( infoBuf, &bufCharCount ) )
      computerName = infoBuf;
   else
      computerName = "UNKNOWN";

   return computerName;
}
