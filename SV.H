// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

////////////////////////////////////////////////////
// CSV class for incremental update determination
////////////////////////////////////////////////////

typedef unsigned long u4;
typedef unsigned short u2;
typedef unsigned char u1;

#include "Debug.h"

#include <vector>
#include <string>
#include <strstream>

class CSV {
public:
   u4 svs[64];
   std::string sName;
   std::vector<int> rs;
   std::vector<int> dirs;
   u4 flags;

   CSV() {};
   CSV(int nSquares, const u1* squares, const char* name, u4 flags);
   CSV(const CSV& b);
   virtual ~CSV();

   u4& operator[](int i) {return svs[i];};
   const u4& operator[](int i) const { return svs[i];};
   u4& Value(int x, int y) {return svs[x+8*y];};
   const u4& Value(int x, int y) const { return svs[x+8*y];};
   u4& ValueI(int x, int y, int ir) {return svs[SquareI(x,y,ir)];};
   const u4& ValueI(int x, int y, int ir) const { return svs[SquareI(x,y,ir)];};
   u4& ValueI(int sq, int ir) {return svs[SquareI(sq,ir)];};
   const u4& ValueI(int sq, int ir) const { return svs[SquareI(sq,ir)];};
   u4& ValueR(int x, int y, int r) {return svs[SquareR(x,y,r)];};
   const u4& ValueR(int x, int y, int r) const { return svs[SquareR(x,y,r)];};
   u4& ValueR(int sq, int r) {return svs[SquareR(sq,r)];};
   const u4& ValueR(int sq, int r) const { return svs[SquareR(sq,r)];};

   bool operator<(const CSV& b) const;
   bool operator==(const CSV& b) const;

   int SquareI(int x, int y, int i) const {return SquareR(x,y,rs[i]);}
   int SquareI(int sq, int i) const {return SquareR(X(sq),Y(sq),rs[i]);}
   static int Square(int x, int y) { _ASSERT(x>=0 && x<8 && y>=0 && y<8); return x+8*y;};
   static int X(int sq) { return sq&7; };
   static int Y(int sq) { return sq>>3;};

   void Out(std::ostream& os) const;
   void Out(std::ostream& os, int ir) const;

   // Incremental update calculations
   std::vector<int> ius;
   void ClearIUs();
   void IncrementIUs(int x, int y, int increment);
   void IncrementIUsDir(int x, int y, int increment, const CSV* svOK, int iOK, bool fNonDir);
   void PrintIUs(bool fPlus);
   void PrintClear();
   void PrintVariableDefinitions() const;
   void PrintCoefficientTables() const;
   void PrintEvalSection(int window, int subwindow, int iColor) const;

   // Direction calculation
   int Dir(int i) const;
   bool FindDirCSV(int x, int y, int idir, int& ir) const;
   void GetMaxFlips(int x, int y, int ir, int& nDownMax, int& nUpMax) const;
   void GetLengthAndLoc(int x, int y, int ir, int& length, int& loc) const;

   // configToId information
   int Size() const;
   int NConfigs() const;
   int ConfigToId(int config) const;
   int NIds() const;
   int* CreateConfigToSubconfigTable(const CSV* pCSVSub) const;
   void GetItritToSquareTable(int* itritToSquare) const;
   void GetItritToValueTable(const int* itritToSquare, int nTrits, int ir, int* itritToValue) const;

   // coefficient loading
   void CoeffClear(int* coeffs[2]) const;
   void CoeffLoad(FILE* fp, int* coeffs[2], bool fClear=true, bool fReadCoeffs=true) const;
   void CoeffLoadSub(FILE* fp, int* coeffs[2], CSV* pCSVSub, bool fClear=true, bool fReadCoeffs=true) const;

   // misc
   std::string CoeffTableName(int window, int subwindow, int iColor) const;

   // flags
   enum { kDir = 1, kEval = 2 };
protected:
   int size, nConfigs;

   void ClearSquares();
   void CalcReflections();
   void CalcDirs();
   void CalcDirIr(int ir);

   bool Similar(int r1, int r2) const;
   bool ProperOrder(int r1, int r2) const;
   bool ContainsSubconfig(const CSV* pSub, int ir) const;

   void CreateConfigToIdTable();
   int* configToId;
   int nIds;

   static int SquareR(int x, int y, int r);
   static int SquareR(int sq, int r);

   void Copy(const CSV& b);
};

inline int CSV::NConfigs() const { return nConfigs;}
inline int CSV::Size() const { return size;}
inline int CSV::NIds() const { return nIds;}
inline int CSV::ConfigToId(int config) const { _ASSERT(configToId); _ASSERT(config>=0 && config<nConfigs); return configToId[config];}

inline std::ostream& operator<<(std::ostream& os, const CSV& sv) { sv.Out(os); return os;}

class CBB;

class CSVs: public std::vector<CSV> {
public:
   CSVs(const char* sName, const char* sHabName, int aFlags);

   void PrintVariableDefinitions() const;
   void PrintCoefficientTables() const;
   void PrintFlipTableDefinitions() const;

   void PrintInitSquareRoutines();
   void PrintInitRoutine();

   void PrintFlipRoutines();
   void PrintInitTables();
   void PrintInitTableRoutine() const;
   void PrintSquareRoutines() const;
   void PrintEvalRoutines() const;
   void PrintFlipSquareTables() const;

   enum { kBitboard=1 };
protected:
   std::string sName;
   std::string sHabName;
   int flags;

   void ClearIUs();
   void IncrementIUs(int x, int y, int increment);
   void IncrementIUsDir(int x, int y, int increment, const CSV* svOK, int iOK, bool fNonDir);
   void PrintIUs(bool fPlus);
   void PrintClear();

   void PrintFlipTableDefinitions(int x, int y, int idir) const;
   void PrintInitSquareRoutine(int x, int y);
   void PrintFlipRoutine(int x0, int y0, int idir, int nDown, int nUp, int iColor);
   void PrintInitTable(int x, int y, int idir, int nDownMax, int nUpMax, int iColor) const;
   void PrintSquareRoutine(int x, int y, int iColor) const;
   void PrintEvalRoutine(int window, int subwindow, int iColor) const;
   void PrintCoeffTables(int window) const;
   void PrintCoeffTables(int window, int subwindow) const;
   bool FindDirCSV(int x, int y, int idir, CSV*& pCSV, int& ir);
   bool FindNextDirCSV(int x, int y, int& idir, CSV*& pCSV, int& ir);

   bool FindDirCSV(int x, int y, int idir, const CSV*& pCSV, int& ir) const;
   bool FindNextDirCSV(int x, int y, int& idir, const CSV*& pCSV, int& ir) const;

   static void GetBitBoardUpdate(int x, int y, int idir, int nDown, int nUp, CBB& mover);
   void PrintBitBoardUpdate(const CBB& mover);

   std::string FFName(int x, int y, int idir, int nDown, int nUp, int iColor) const;
   std::string FFTableName(int x, int y, int idir, int iColor) const;
   std::string FFTableInitName(int x, int y, int idir, int iColor) const;
};
