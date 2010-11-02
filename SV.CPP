// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "SV.h"
#include "Debug.h"
#include <iostream>
#include <iomanip>
#include "Utils.h"
#include "Patterns.h"
#include "Evaluator.h"
using namespace std;

const int maxSize=16;	// no configs bigger than maxSize trits allowed

static int idirToDir[4]={1,7,8,9};
static char* colors[2]={"Black_","White_"};

bool SquareMayBeEmpty(int x, int y) {
   return (x<3 || x>4 || y<3 || y>4);
}

CSV::CSV(const CSV& b) {
   Copy(b);
}

CSV::CSV(int nSquares, const u1* squares, const char* name, u4 aflags)
   :sName(name), flags(aflags), size(nSquares), configToId(NULL) {
   int i, value;

   ClearSquares();
   _ASSERT(size<maxSize);

   // values
   for (i=0, value=1; i<nSquares; i++) {
      svs[squares[i]]=value;
      value+=value+value;
   }
   nConfigs=value;
   _ASSERT(value<=6561*27);

   CalcReflections();
   CalcDirs();
   CreateConfigToIdTable();
}

void CSV::Copy(const CSV& b) {
   int sq;

   for (sq=0; sq<64; sq++)
      svs[sq]=b.svs[sq];
   sName=b.sName;
   dirs=b.dirs;
   flags=b.flags;
   ius=b.ius;
   rs=b.rs;
   nConfigs=b.nConfigs;
   nIds=b.nIds;
   rs=b.rs;
   size=b.size;
   sName=b.sName;
   configToId=NULL;
   CreateConfigToIdTable();

}

CSV::~CSV() {
   if (configToId)
      delete[] configToId;
}

void CSV::ClearSquares() {
   int i;
   for (i=0; i<64; i++)
      svs[i]=0;
}

// Similar returns true if the CSVs have nonzero entries for exactly the same squares
bool CSV::Similar(int r1, int r2) const {
   int x,y;
   for (x=0; x<8; x++) {
      for (y=0; y<8; y++) {
         int sq1=SquareR(x,y,r1);
         int sq2=SquareR(x,y,r2);
         bool fBlank1=!svs[sq1];
         bool fBlank2=!svs[sq2];
         if (fBlank1!=fBlank2)
            return false;
      }
   }
   return true;
}

void CSV::CalcReflections() {
   std::size_t i;
   for (std::size_t r=0; r<8; r++) {
      for (i=0; i<rs.size(); i++) {
         if (Similar(r,rs[i])) {
            if (ProperOrder(r,rs[i]))
               rs[i]=r;
            break;
         }
      }
      if (i==rs.size())
         rs.push_back(r);
   }

   // number of reflections should be a power of 2 since the reflections group has 8 elements
   i=rs.size();
   _ASSERT((i&i-1)==0);
}

void CSV::CalcDirIr(int ir) {
   int x, y, s,s0;

   bool fHasS=false;
   for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
         if (ValueI(x,y,ir)) {
            s=Square(x,y);
            if (fHasS) {
               dirs[ir]= s-s0;
               return;
            }
            else {
               s0=s;
               fHasS=true;
            }
         }
      }
   }
}

void CSV::CalcDirs() {
   int ir;

   dirs.resize(rs.size(),0);

   if (flags&kDir) {
      for (ir=0; ir<rs.size(); ir++)
         CalcDirIr(ir);
   }
}

// return TRUE if reflection1 is smaller than reflection2.
//	The main use of this function is to determine which of two similar reflections
//	to keep (for directional flips, we want the squares listed in increasing order).
bool CSV::ProperOrder(int r1, int r2) const {
   int x,y,v1,v2;
   for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
         v1=svs[SquareR(x,y,r1)];
         v2=svs[SquareR(x,y,r2)];
         _ASSERT(!v1==!v2);
         if (v1<v2)
            return true;
         else if (v1>v2)
            return false;
      }
   }
   return false;
}

bool CSV::operator<(const CSV& b) const {
   int i;
   for (i=0; i<64; i++) {
      if (svs[i]<b.svs[i])
         return true;
      else if (svs[i]<b.svs[i])
         return false;
   }
   return false;
}

bool CSV::operator==(const CSV& b) const {
   int i;
   for (i=0; i<64; i++) {
      if (svs[i]!=b.svs[i])
         return false;
   }
   return true;
}

// Return square id for reflection. r=0..7.
//	Reflection indices are not guaranteed to be the same as bitboard reflection indices.
int CSV::SquareR(int x, int y, int r) {
   bool fSwapXY=r&1;
   bool fReverseX=(r&2)!=0;
   bool fReverseY=(r&4)!=0;

   if (fReverseX)
      x=7-x;
   if (fReverseY)
      y=7-y;
   if (fSwapXY)
      return Square(y,x);
   else
      return Square(x,y);
}

int CSV::SquareR(int sq, int r) {
   return SquareR(X(sq), Y(sq), r);
}

void CSV::Out(ostream& os, int ir) const {
   os << sName << "\n";
   int x,y;
   for (y=0; y<=7; y++) {
      os << char('A'+y)<< ' ';
      for (x=0; x<=7; x++) {
         os << setw(6) << ValueI(x,y,ir);
      }
      os << ' ' << char('A'+y)<< '\n';
   }
   os << '\n';
}

void CSV::Out(ostream& os) const {
   Out(os, 0);
}

// mini bitboard class for determining bitboard updates
class CBB {
public:
   u4 bits[2];

   void Clear() { bits[0]=bits[1]=0; }
   void SetBit(int i);
};

void CBB::SetBit(int i) {
   _ASSERT(i>=0 && i<64);
   bits[i>>5]|=1<<(i&31);
}

/////////////////////////////////////////////////////////////
// Incremental update determination routines
//
// usage:
//	Call ClearIUs() once to reset values to 0
//	Call IncrementIUs() repeatedly to calculate the IU value
//	Call PrintIUs() to print the generated code
/////////////////////////////////////////////////////////////

void CSV::ClearIUs() {
   ius.clear();
   ius.resize(rs.size());
}

void CSV::IncrementIUs(int x, int y, int increment) {
   int i;

   for (i=0; i<rs.size(); i++) {
      int s=SquareI(x,y,i);
      ius[i]+=increment*svs[s];
   }
}

// incrementally update only if the move is in the
//	proper direction. This is used when updating patterns
//	due to a stone placement - the directional update routine
//	only updates the pattern in its direction. If fNonDir is
//	true, we update all nondirectional patterns as well; it should
//	be true in precisely one of the directional flip routines.
// svOK is the directional CSV that it is OK to print, with reflection index iOK.
void CSV::IncrementIUsDir(int x, int y, int increment, const CSV* svOK, int iOK, bool fNonDir) {
   // directional patterns
   if (flags&kDir) {
      if (this==svOK)
         ius[iOK]+=increment*ValueI(x,y,iOK);
   }
   else {
      if (fNonDir)
         IncrementIUs(x,y,increment);
   }
}

void CSV::PrintIUs(bool fPlus) {
   int i, iu;
   bool fPrintPlus;

   for (i=0; i<rs.size(); i++) {
      if (ius[i]) {
         iu=ius[i];
         if (iu<0) {
            iu=-iu;
            fPrintPlus=!fPlus;
         }
         else {
            fPrintPlus=fPlus;
         }
         cout << sName << '_' << i << (fPrintPlus?"+=":"-=") << iu << ";\t";
      }
   }
}

void CSV::PrintClear() {
   int i;

   for (i=0; i<rs.size(); i++) {
      cout << sName << '_' << i << "=0;\t";
   }
}

void CSV::PrintVariableDefinitions() const {
   for (int i=0; i<rs.size(); i++) {
      cout << "int " << sName << '_' << i << ";\t";
   }
   cout << "\n";
}

void CSV::PrintCoefficientTables() const {
   if (flags&kEval) {
      for (int window=0; window<10; window++) {
         for (int subwindow=0; subwindow<2; subwindow++) {
            for (int iColor=0; iColor<2; iColor++) {
               cout << "int " << CoeffTableName(window, subwindow, iColor!=0) << "[" << nConfigs << "];\t";
            }
         }
      }
      cout << "\n";
   }
}

void CSV::PrintEvalSection(int window, int subwindow, int iColor) const {
   int ir;

   if (flags & kEval) {
      cout << "\n\t\t";
      for (ir=0; ir<rs.size(); ir++) {
         cout << " + ";
         cout << CoeffTableName(window,subwindow,iColor) << "[" << sName << "_" << ir << "]";
      }
   }
}

// find the direction (1,7,8,9) of the reflection, or return 0 if no direction
int CSV::Dir(int ir) const {
   return dirs[ir];
}

// find the CSV corresponding to the direction following the given index
// inputs:
//	x, y - location of square
//	idir - direction index (0..3)
// outputs:
//	ir - reflection index for the CSV for the new idir, undefined if no more directions
// returns
//	true if found the dir
bool CSV::FindDirCSV(int x, int y, int idir, int& ir) const {
   int dir=idirToDir[idir];

   for (ir=0; ir<rs.size(); ir++) {
      if (ValueI(x,y,ir) && Dir(ir)==dir)
         return true;
   }
   return false;
}

// find the maximum number of disks flippable in a given direction
void CSV::GetMaxFlips(int x0, int y0, int ir, int& nDownMax, int& nUpMax) const {
   int x,y;

   _ASSERT(flags&kDir);
   _ASSERT(ValueI(x0,y0,ir));

   nDownMax=nUpMax=0;

   for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
         if (ValueI(x,y,ir)) {
            if (y<y0 || (y==y0 && x<x0))
               nDownMax++;
            else if (y>y0 || (y==y0&& x>x0))
               nUpMax++;
            else
               _ASSERT(y==y0 && x==x0);
         }
      }
   }

   // can't flip the end disk
   if (nDownMax)
      nDownMax--;
   if (nUpMax)
      nUpMax--;
}

void CSV::GetLengthAndLoc(int x0, int y0, int ir, int& length, int& loc) const {
   int x,y;

   _ASSERT(ValueI(x0,y0,ir));
   _ASSERT(flags&kDir);

   length=0;

   for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
         if (ValueI(x,y,ir)) {
            if (x==x0 && y==y0)
               loc=length;
            length++;
         }
      }
   }
}

string CSV::CoeffTableName(int window, int subwindow, int iColor) const {
   ostrstream os;
   os << "coeffs_" << sName << "_" << colors[iColor] << window << subwindow << '\0';
   string result(os.str());
   os.rdbuf()->freeze(0);

   return result;
}

int Dot(int* trits, int* rvalues, int size) {
   int i;

   int result=0;
   for (i=0; i<size; i++) {
      result+=trits[i]*rvalues[i];
   }

   return result;
}

extern void ConfigToTrits(int config, int length, int* trits);

bool CSV::ContainsSubconfig(const CSV* pSub, int ir) const {
   int x,y;

   for (x=0; x<8; x++) {
      for (y=0; y<8; y++) {
         if (pSub->ValueI(x,y,ir) && !Value(x,y))
            return false;
      }
   }
   return true;
}

// fills in the itritToSquare table. Caller is responsible for allocating memory for itritToSquare.
//	It has size equal to the number of filled squares in the pattern.
void CSV::GetItritToSquareTable(int* itritToSquare) const {
   int itrit, sq;
   for (sq=0; sq<64; sq++) {
      int value=svs[sq];
      if (value!=0) {
         itrit=0;
         while (value/=3)
            itrit++;
         _ASSERT(itrit<=maxSize);
         itritToSquare[itrit]=sq;
      }
   }
}

void CSV::GetItritToValueTable(const int* itritToSquare, int nTrits, int r, int* itritToValue) const {
   int itrit;

   for (itrit=0; itrit<nTrits; itrit++)
      itritToValue[itrit]=ValueR(itritToSquare[itrit],r);
}

int* CSV::CreateConfigToSubconfigTable(const CSV* pSub) const {
   int ir,config,itrits[maxSize];
   int *configToSubconfig = new int[nConfigs];

   if (configToSubconfig==0) {
      cerr << "Out of memory in CSV::CreateConfigToSubconfigTable\n";
      cout << "Out of memory in CSV::CreateConfigToSubconfigTable\n";
      _exit(-1);
   }

   // find the reflection that we use to fit sub in this pattern
   int nr=pSub->rs.size();
   for (ir=0; ir<nr; ir++) {
      if (ContainsSubconfig(pSub,ir))
         break;
   }
   _ASSERT(ir<nr);	// fails if no reflection of subconfig is contained in this pattern

   // get itritToValue table.
   int itritToSquare[maxSize], itritToValue[maxSize];
   GetItritToSquareTable(itritToSquare);
   pSub->GetItritToValueTable(itritToSquare, Size(), ir, itritToValue);

   // fill in table
   for (config=0; config<nConfigs; config++) {
      ConfigToTrits(config, size, itrits);
      configToSubconfig[config]=Dot(itrits, itritToValue, size);
   }

   return configToSubconfig;
}

// creates and returns a table mapping configs to ids. CALLER is responsible for deleting the table.
void CSV::CreateConfigToIdTable() {
   if (configToId) {
      _ASSERT(0);
      return;
   }

   vector<int> sims;
   int r,ir;
   configToId = new int[nConfigs];

   _ASSERT(configToId);
   _ASSERT(maxSize>=size);

   // find similar configs
   for (r=0; r<8; r++) {
      if (Similar(0,r))
         sims.push_back(r);
   }
   // must be 2^n elements
   _ASSERT((sims.size()&(sims.size()-1))==0);

   // find reorder tables
   // first step, rvalues table
   int itritToSquare[maxSize];
   int rvalues[8][maxSize];	// [reflection index][trit index]
   GetItritToSquareTable(itritToSquare);
   for (ir=0; ir<sims.size(); ir++)
      GetItritToValueTable(itritToSquare, size, sims[ir], rvalues[ir]);

   // second step, find rconfigs for each config
   int config, rconfig;
   bool fMinimal;
   int trits[maxSize];

   nIds=0;
   for (config=0; config<nConfigs; config++) {
      ConfigToTrits(config, size, trits);
      fMinimal=true;
      for (ir=0; ir<sims.size(); ir++) {
         rconfig = Dot(trits, rvalues[ir], size);
         if (rconfig<config) {
            fMinimal=false;
            break;
         }
      }
      if (fMinimal)
         configToId[config]=nIds++;
      else
         configToId[config]=configToId[rconfig];
   }

   if (size==8 && (flags&kDir)) {
      for (config=0; config<nConfigs; config++) {
         _ASSERT(configToId[config]==mapsJ[R1J].ConfigToID(config));
      }
   }
}

// add coefficients from a file, and pot mob coefficients, to the coeffs.
//	if fClear = 0, clear the coeffs first
//	if fReadCoeffs = true (the default), read coeffs from the file, otherwise just use pot mobs.

bool fDebugCoeffLoad=false;

void CSV::CoeffLoad(FILE* fp, int* coeffs[2], bool fClear, bool fReadCoeffs) const {
   if (fClear)
      CoeffClear(coeffs);

   int config, coeff, nPMB, nPMW;

   float* idToRaw = new float[nIds];

   if (idToRaw==0) {
      cerr << "Out of memory in CSV::CoeffAdd\n";
      cout << "Out of memory in CSV::CoeffAdd\n";
      _exit(-1);
   }

   if (fReadCoeffs && nIds!=fread(idToRaw, sizeof(float), nIds, fp)) {
      cerr << "Can't read coefficients in CSV::CoeffAdd\n";
      cout << "Can't read coefficients in CSV::CoeffAdd\n";
      _exit(-2);
   }

   nPMB=nPMW=0;
   for (config=0; config<nConfigs; config++) {
      if (fReadCoeffs) {
         coeff = int(idToRaw[configToId[config]]*kStoneValue)<<16;
         if (coeff!=0 && fDebugCoeffLoad) {
            // cout << "\tConfig " << config << ", id " << configToId[config] << ", coeff = " << (coeff>>16) << "\n";
         }
      }
      else
         coeff=0;
      if (flags&kDir) {
         nPMB=configToPotMob[1][size][config];
         nPMW=configToPotMob[0][size][config];
         coeff+=(nPMB<<8)+nPMW;
      }
      coeffs[1][config]+=coeff;
      coeffs[0][nConfigs-config-1]+=coeff;
   }

   delete[] idToRaw;
}

void CSV::CoeffClear(int* coeffs[2]) const {
   int config;

   for (config=0; config<nConfigs; config++) {
      coeffs[0][config]=coeffs[1][config]=0;
   }
}

void CSV::CoeffLoadSub(FILE* fp, int* coeffs[2], CSV* pCSVSub, bool fClear, bool fReadCoeffs) const {
   if (fClear)
      CoeffClear(coeffs);

   int subcoeff, config;
   int nConfigsSub=pCSVSub->NConfigs();
   int* coeffsSub[2];
   int* configToSubconfig = CreateConfigToSubconfigTable(pCSVSub);

   coeffsSub[0]= new int[nConfigsSub];
   coeffsSub[1]= new int[nConfigsSub];

   if (!configToSubconfig || !coeffsSub[0] || !coeffsSub[1]) {
      cerr << "Out of memory in CSV::CoeffAddSub\n";
      cout << "Out of memory in CSV::CoeffAddSub\n";
      _exit(-1);
   }

   pCSVSub->CoeffLoad(fp, coeffsSub, true, fReadCoeffs);

   for (config=0; config<nConfigs; config++) {
      subcoeff = coeffsSub[0][configToSubconfig[config]];
      coeffs[0][config]+=subcoeff;
      coeffs[1][nConfigs-config-1]+=subcoeff;
   }

   delete[] coeffsSub[0];
   delete[] coeffsSub[1];
   delete[] configToSubconfig;
}

/////////////////////////////////////////////////////////////
// Mass incremental update determination routines
//
// usage:
//	Call ClearIUs() once to reset values to 0
//	Call IncrementIUs() repeatedly to calculate the IU value
//	Call PrintIUs() to print the generated code
/////////////////////////////////////////////////////////////

CSVs::CSVs(const char* asName, const char* asHabName, int aFlags) : sName(asName), sHabName(asHabName), flags(aFlags) {
}

void CSVs::ClearIUs() {
   int i;
   for (i=0; i<size(); i++)
      (*this)[i].ClearIUs();
}

void CSVs::IncrementIUs(int x, int y, int increment) {
   int i;
   for (i=0; i<size(); i++)
      (*this)[i].IncrementIUs(x,y,increment);
}

void CSVs::IncrementIUsDir(int x, int y, int increment, const CSV* svOK, int iOK, bool fNonDir) {
   int i;
   for (i=0; i<size(); i++)
      (*this)[i].IncrementIUsDir(x,y,increment,svOK,iOK,fNonDir);
}

void CSVs::PrintIUs(bool fPlus) {
   int i;
   for (i=0; i<size(); i++)
      (*this)[i].PrintIUs(fPlus);
}

void CSVs::PrintClear() {
   int i;
   for (i=0; i<size(); i++)
      (*this)[i].PrintClear();
}

void CSVs::PrintVariableDefinitions() const {
   int i;
   for (i=0; i<size(); i++)
      (*this)[i].PrintVariableDefinitions();
   cout << "\n\n";
}

void CSVs::PrintCoefficientTables() const {
   // pattern tables
   for (int i=0; i<size(); i++)
      (*this)[i].PrintCoefficientTables();

   // nonpattern tables
   for (int window=0; window<10; window++) {
      for (int subwindow=0; subwindow<2; subwindow++) {
         cout << "int coeffs_MP_" << window << subwindow << "[64], coeffs_MO_" << window << subwindow << "[64], ";
         cout << "coeffs_PMP_" << window << subwindow << "[256], coeffs_PMO_" << window << subwindow << "[256], ";
         cout << "coeffs_Constant_" << window << subwindow << ";\n";
      }
   }
   cout << "\n\n";
}

// generate routines to initialize the IU values
void CSVs::PrintInitSquareRoutine(int x, int y) {
   ClearIUs();
   IncrementIUs(x,y,1);

   cout << "void " << sName << "_Init_" << char('A'+x) << y+1 << "() {\n";
   cout << "\t";
   PrintIUs(true);
   cout << "\n}\n\n";
}

void CSVs::PrintInitSquareRoutines() {
   int x, y;
   for (x=0; x<8; x++) {
      for (y=0; y<8; y++) {
         PrintInitSquareRoutine(x,y);
      }
   }
}

void CSVs::PrintInitRoutine() {
   int x,y;

   cout << "void " << sName << "_Init_IU(const char* sBoard, bool fBlackMove) {\n";
   cout << "\t";
   PrintClear();
   cout << "\n";
   for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
         cout << "\tswitch(toupper(sBoard[" << CSV::Square(x,y) << "])) {\n";
         cout << "\tcase '*': case 'X': case 'B': " << sName << "_Init_" << char('A'+x) << y+1 << "();\n";
         cout << "\tcase '.': case '-': case '_': " << sName << "_Init_" << char('A'+x) << y+1 << "();\n";
         cout << "\tcase 'W': case 'O': case '0': break;\n";
         cout << "\tdefault: _ASSERT(0);\n";
         cout << "\t}\n";
      }
   }
   if (flags&kBitboard) {
      cout << "\tbb.Initialize(sBoard, fBlackMove);\n";
   }
   cout << "}\n\n";
}

// find the CSV corresponding to the direction following the given index
// inputs:
//	x, y - location of square
//	idir - direction index (0..3)
// outputs:
//	pCSV - pointer to the CSV for the new idir, or undefined if no more directions
//	ir - reflection index for the CSV for the new idir, undefined if no more directions
// returns:
//	true if a dir was found, false if no more dirs
bool CSVs::FindDirCSV(int x, int y, int idir, CSV*& pCSV, int& ir) {
   int i;
   for (i=0; i<size(); i++) {
      if ((*this)[i].FindDirCSV(x,y,idir,ir)) {
         pCSV=&((*this)[i]);
         return true;
      }
   }
   return false;
}

bool CSVs::FindDirCSV(int x, int y, int idir, const CSV*& pCSV, int& ir) const{
   int i;
   for (i=0; i<size(); i++) {
      if ((*this)[i].FindDirCSV(x,y,idir,ir)) {
         pCSV=&((*this)[i]);
         return true;
      }
   }
   return false;
}

// find the CSV corresponding to the direction following the given index
// inputs:
//	x, y - location of square
//	idir - current direction index (-1..3), -1 if we are to find the first direction
// outputs:
//	idir - new direction index (0..4), 4 if there are no more directions
//	pCSV - pointer to the CSV for the new idir, or NULL if no more directions
//	ir - reflection index for the CSV for the new idir, undefined if no more directions
bool CSVs::FindNextDirCSV(int x, int y, int& idir, const CSV*& pCSV, int& ir) const {
   _ASSERT(-1<=idir && idir<=3);
   pCSV=NULL;
   for (idir++; idir<4; idir++) {
      if (FindDirCSV(x, y, idir, pCSV, ir))
         return true;
   }
   return false;
}

bool CSVs::FindNextDirCSV(int x, int y, int& idir, CSV*& pCSV, int& ir) {
   _ASSERT(-1<=idir && idir<=3);
   pCSV=NULL;
   for (idir++; idir<4; idir++) {
      FindDirCSV(x, y, idir, pCSV, ir);
      if (pCSV)
         return true;
   }
   return false;
}

void CSVs::GetBitBoardUpdate(int x, int y, int idir, int nDown, int nUp, CBB& mover) {
   int sq0=CSV::Square(x,y);
   int dir=idirToDir[idir];
   int i,sq;

   mover.Clear();
   for (i=0, sq=sq0; i<nDown; i++) {
      sq-=dir;
      mover.SetBit(sq);
   }
   for (i=0, sq=sq0; i<nUp; i++) {
      sq+=dir;
      mover.SetBit(sq);
   }
}

void CSVs::PrintBitBoardUpdate(const CBB& mover) {
   if (flags&kBitboard) {
      if (mover.bits[0])
         printf("bb.mover.u4s[0]^=0x%X; ",mover.bits[0]);
      if (mover.bits[1])
         printf("bb.mover.u4s[1]^=0x%X; ",mover.bits[1]);
   }
}

string CSVs::FFName(int x, int y, int idir, int nDown, int nUp, int iColor) const {
   ostrstream os;
   os << sName << "_" << colors[iColor] << char(x+'A') << y+1 << "_" << idir << "_" << nDown << nUp << '\0';
   string result(os.str());
   os.rdbuf()->freeze(0);
   return result;
}

string CSVs::FFTableName(int x, int y, int idir, int iColor) const {
   ostrstream os;
   os << sName << "Table_" << colors[iColor] << char(x+'A') << y+1 << "_" << idir << '\0';
   string result(os.str());
   os.rdbuf()->freeze(0);
   return result;
}

string CSVs::FFTableInitName(int x, int y, int idir, int iColor) const {
   ostrstream os;
   os << sName << "TableInit_" << colors[iColor] << char(x+'A') << y+1 << "_" << idir << '\0';
   string result(os.str());
   os.rdbuf()->freeze(0);
   return result;
}

void CSVs::PrintFlipTableDefinitions(int x, int y, int idir) const {
   int ir;
   const CSV* pCSV;

   if (FindDirCSV(x,y,idir,pCSV,ir)) {
      cout << "TFFM ";
      cout << '*' << FFTableName(x,y,idir,false)<< "[" << pCSV->NConfigs() << "], ";
      cout << '*' << FFTableName(x,y,idir,true) << "[" << pCSV->NConfigs() << "];\n";
   }
}

void CSVs::PrintFlipTableDefinitions() const {
   int x,y,idir;

   for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
         for (idir=0; idir<4; idir++) {
            PrintFlipTableDefinitions(x,y,idir);
         }
      }
   }
}

void CSVs::PrintFlipSquareTables() const {
   for (int iColor=0; iColor<2; iColor++) {
      cout << "TFFM* " << sName << "_" << colors[iColor] << "Table[64] = {\n\t";
      for (int y=0; y<8; y++) {
         for (int x=0; x<8; x++) {
            if (SquareMayBeEmpty(x,y))
               cout << sName << "_" << colors[iColor] << char(x+'A') << y+1 << ", ";
            else
               cout << "BadFFM, ";
         }
      }
      cout << "\n};\n";
   }
}

void CSVs::PrintFlipRoutine(int x0, int y0, int idir, int nDown, int nUp, int iColor) {
   // calculate changes
   int i,x,y;
   static int dxs[4]={1,-1,0,1};
   static int dys[4]={0,1,1,1};
   int dx=dxs[idir];
   int dy=dys[idir];
   CSV* pCSV;
   int ir;
   const char* sColor=colors[iColor];
   CBB mover;

   _ASSERT(x0>=0 && y0>=0 && x0<8 && y0<8);

   // update patterns
   ClearIUs();
   int viu=iColor?-2:2;
   for (x=x0, y=y0, i=0; i<nDown; i++) {
      x-=dx;
      y-=dy;
      _ASSERT(x>=0 && y>=0);
      IncrementIUs(x,y,viu);
   }
   for (x=x0, y=y0, i=0; i<nUp; i++) {
      x+=dx;
      y+=dy;
      IncrementIUs(x,y,viu);
   }
   GetBitBoardUpdate(x0,y0,idir,nDown,nUp,mover);

   // update the square only in the direction flipped.
   // update nondirectional patterns when flipping horizontally (all squares can flip horizontally)
   FindDirCSV(x0,y0,idir,pCSV,ir);
   IncrementIUsDir(x0,y0,iColor?-1:1,pCSV,ir,idir==0);

   cout << "int " << FFName(x0,y0,idir,nDown,nUp,iColor) << "(const " << sHabName << "& hab) {\n";
   cout << "\t";
   PrintBitBoardUpdate(mover);
   PrintIUs(true);
   cout << "\n";
   cout << "	int result = ";

   // calculate subroutine to call
   FindNextDirCSV(x0,y0,idir,pCSV, ir);

   if (idir==4) {
      cout << sName << "_AB_" << sColor << "Moved(hab);\n";
   }
   else {
      cout << FFTableName(x0,y0,idir,iColor) << "[" << pCSV->sName << "_" << ir << "](hab);\n";
   }

   // unflip
   cout << "\t";
   PrintBitBoardUpdate(mover);
   PrintIUs(false);
   cout << "\n";
   cout << "	return result;\n";
   cout << "}\n\n";
}

void CSVs::PrintFlipRoutines() {
   int x,y,idir,nDown,nUp,nDownMax,nUpMax;

   for (y=0; y<8; y++) {
      for (x=0;x<8;x++) {
         if (!SquareMayBeEmpty(x,y))
            continue;
         cout << "\n// Routines for " << char(x+'A') << y+1 << "\n";
         idir=-1;
         CSV* pCSV;
         int ir;
         while (FindNextDirCSV(x,y,idir,pCSV,ir)) {
            _ASSERT(idir<4);
            pCSV->GetMaxFlips(x,y,ir, nDownMax, nUpMax);
            for (nDown=0; nDown<=nDownMax; nDown++) {
               for (nUp=0; nUp<=nUpMax; nUp++) {
                  _ASSERT(SquareMayBeEmpty(x,y));
                  PrintFlipRoutine(x,y,idir,nDown,nUp,false);
                  PrintFlipRoutine(x,y,idir,nDown,nUp,true);
               }
            }
         }
      }
   }
}

void CSVs::PrintInitTables() {
   int x,y,ir,idir, nDownMax, nUpMax;
   CSV* pCSV;

   for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
         if (!SquareMayBeEmpty(x,y))
            continue;
         idir=-1;
         while (FindNextDirCSV(x,y,idir,pCSV,ir)) {
            pCSV->GetMaxFlips(x,y,ir, nDownMax, nUpMax);
            PrintInitTable(x,y,idir, nDownMax, nUpMax, false);
            PrintInitTable(x,y,idir, nDownMax, nUpMax, true);
         }
      }
   }
}

// init for FFKTable_Black_H8_3
void CSVs::PrintInitTable(int x, int y, int idir, int nDownMax, int nUpMax, int iColor) const {
   int nDown, nUp;

   cout << "TFFM* " << FFTableInitName(x,y,idir,iColor);
   cout << "[7][7] = { \n\t";

   for (nDown=0; nDown<7; nDown++) {
      for (nUp=0; nUp<7; nUp++) {
         if (nDown<=nDownMax && nUp<=nUpMax)
            cout << FFName(x,y,idir,nDown, nUp, iColor);
         else
            cout << "BadFFM";
         cout << ", ";
      }
      cout << "\n\t";
   }
   cout << "};\n\n";
}

void CSVs::PrintInitTableRoutine() const {
   int x,y,ir,idir, length, loc,iColor;
   const CSV* pCSV;

   cout << "void " << sName << "_Table_Init() {\n";
   for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
         if (!SquareMayBeEmpty(x,y))
            continue;
         idir=-1;
         while (FindNextDirCSV(x,y,idir,pCSV,ir)) {
            for (iColor=0; iColor<2; iColor++) {
               pCSV->GetLengthAndLoc(x,y,ir,length,loc);
               cout << "	SetFFMs(" << FFTableInitName(x,y,idir,iColor!=0) << ", " << FFTableName(x,y,idir,iColor!=0)
                    << ", " << length << ", " << loc << ", " << (iColor?"false":"true") << ");\n";
            }
         }
      }
   }
   cout << "}\n";
}

void CSVs::PrintSquareRoutine(int x, int y, int iColor) const {
   int sq=CSV::Square(x,y);
   int idir=-1, ir;
   const CSV* pCSV;
   FindNextDirCSV(x,y,idir,pCSV,ir);
   _ASSERT(idir==0);

   cout << "int " << sName << "_" << colors[iColor] << char(x+'A') << y+1 << "(const " << sHabName << "& hab) {\n";
   if (flags&kBitboard) {
      cout << "	bb.empty.u4s[" << (sq>>5) << "] ^= 0x";
      printf("%X;\n",1<<(sq&31));
   }
   cout << "	int result = " << FFTableName(x,y,idir,iColor) << "[" << pCSV->sName << "_" << ir << "](hab);\n";
   if (flags&kBitboard) {
      cout << "	bb.empty.u4s[" << (sq>>5) << "] ^= 0x";
      printf("%X;\n",1<<(sq&31));
   }
   cout << "	return result;\n";
   cout << "}\n";
}

void CSVs::PrintSquareRoutines() const {
   int x,y,iColor;

   for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
         if (!SquareMayBeEmpty(x,y))
            continue;
         for (iColor=0; iColor<2; iColor++) {
            PrintSquareRoutine(x,y,iColor!=0);
         }
      }
   }
}

void CSVs::PrintCoeffTables(int window, int subwindow) const {
   cout << "\t\t{";
   CSVs::const_iterator i;
   for (i=begin(); i!=end(); i++) {
      if (i->flags&CSV::kEval)
         cout << "{" << i->CoeffTableName(window, subwindow, true)
              << ", " <<  i->CoeffTableName(window, subwindow, false)
              << "}, ";
   }
   cout << "{coeffs_MP_" << window << subwindow << ", NULL}, ";
   cout << "{coeffs_MO_" << window << subwindow << ", NULL}, ";
   cout << "{coeffs_PMP_" << window << subwindow << ", NULL}, ";
   cout << "{coeffs_PMO_" << window << subwindow << ", NULL}, ";
   cout << "{&coeffs_Constant_" << window << subwindow << ", NULL} ";
   cout << " },\n";
}

void CSVs::PrintCoeffTables(int window) const {
   cout << "\t{\n";
   for (int subwindow=0; subwindow<2; subwindow++)
      PrintCoeffTables(window, subwindow);
   cout << "\t},\n";
}

void CSVs::PrintEvalRoutine(int window, int subwindow, int iColor) const {
   cout << "int " << sName << "_Eval_" << colors[iColor] << window << subwindow << "(u4 nMP, u4 nMO) {\n";
   cout << "	int value = 0";

   // pattern component
   CSVs::const_iterator pCSV;
   for (pCSV=begin(); 1; ) {
      pCSV->PrintEvalSection(window, subwindow, iColor);
      pCSV++;
      if (pCSV==end()) {
         cout << ";\n";
         break;
      }
   }

   // nonpattern component
   cout << "	int nPMP = (value>>8)&0xFF;\n";
   cout << "	int nPMO = value&0xFF;\n";
   cout << "	value >>=16;\n";
   cout << "	value+=coeffs_PMP_" << window << subwindow << "[nPMP];\n";
   cout << "	value+=coeffs_PMO_" << window << subwindow << "[nPMO];\n";
   cout << "	value+=coeffs_MP_" << window << subwindow << "[nMP];\n";
   cout << "	value+=coeffs_MO_" << window << subwindow << "[nMO];\n";
   cout << "	value+=coeffs_Constant_" << window << subwindow << ";\n";
   cout << "	return value;\n";
   cout << "}\n\n";
}

void CSVs::PrintEvalRoutines() const {
   int window, subwindow, iColor;

   for (window=0; window<10; window++) {
      for (subwindow=0; subwindow<2; subwindow++) {
         for (iColor=0; iColor<2; iColor++) {
            PrintEvalRoutine(window, subwindow, iColor!=0);
         }
      }
   }

   for (iColor=0; iColor<2; iColor++) {
      cout << "TFnEval* " << sName << "_" << colors[iColor] << "Eval_Table[60] = {\n\t";
      for (int nEmpty=0; nEmpty<60; nEmpty++) {
         window=9-(nEmpty/6);
         subwindow=nEmpty&1;
         cout << sName << "_Eval_" << colors[iColor] << window << subwindow << ", ";
      }
      cout << "};\n\n";
   }

   // FFK coeff table is by window, subwindow, pattern, color
   cout << "int *FFK_Coeff_Tables[10][2][15][2] = {\n";
   for (window=0; window<10; window++)
      PrintCoeffTables(window);
   cout << "};\n\n";
}
