// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// book header file

#pragma once

#ifndef BOOK_H
#define BOOK_H

#include "Utils.h"
#include "BitBoard.h"
#include "Timing.h"
#include "timer.h"
#include "unordered_map_serialization.h"
#include "Fwd.h"

#include <boost/unordered_map.hpp>
#include <boost/serialization/map.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <map>

extern bool fPrintCorrections;

class CBoni {
public:
   CBoni();

   CValue blackBonus;	// add to black/subtract from white for a black win
   CValue drawBonus;	// add to black/subtract from white for a draw
   CValue whiteBonus;	// add to black/subtract from white for a white win

   bool operator==(const CBoni& b);
   bool operator!=(const CBoni& b);
};

std::istream& operator>>(std::istream& is, CBoni& boni);

class CBookValue {

public:

   CBookValue();

   CValue vHeuristic;
   CValue vBlack;
   CValue vWhite;

protected:

   char fSet;        // true if v has been set
   char fAssigned;   // true if black and white values have been assigned
   char fWLDSolved;  // solved for WLD?

   char SolvedChar() const;

public:

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      archive & fSet;
      archive & fAssigned;
      archive & fWLDSolved;
      archive & vHeuristic;
      archive & vBlack;
      archive & vWhite;
   }

   // setting v values
   void Clear();
   void SetLeaf(CValue v, bool afWLDSolved);
   void SetBranch(const CBookValue& valueNew);
   void SetWLDSolved(bool fWLDSolved);

   // assignment (assign values to black and white)
   void AssignLeaf(const CBoni& boni);
   void AssignBranch();
   void Deassign();

   // merging together
   void MergeTerminalValue(CValue vTerminal, const CBoni& boni);
   void MinValues();
   void Merge(const CBookValue& sbv, int pass);

   // information
   bool Win() const;
   bool Draw() const;
   bool Loss() const;
   bool IsSolved() const;
   bool IsAssigned() const;
   bool IsSetAndAssigned() const;
   CValue VMover(bool fBlackMove) const;
   CValue VMover(bool fBlackMove, int vContempt, int nPass) const;

   // I/O
   void Out(std::ostream& os, bool fReverse=0) const;
};

inline std::ostream& operator<<(std::ostream& os, const CBookValue& bv) { bv.Out(os); return os; };

inline bool CBookValue::IsAssigned() const { return fAssigned!=0;}
inline bool CBookValue::IsSetAndAssigned() const { return fSet && IsAssigned();}
inline void CBookValue::SetWLDSolved(bool afWLDSolved) { fWLDSolved=afWLDSolved; }
class CBookData {
public:
   CBookData();

   // update routines
   bool IncreaseHeight(const CHeightInfo& hi, int nEmpty);
   void StoreRoot(CHeightInfo hi, int nEmpty, CValue value, CValue vCutoff, bool fFull, const CBoni& boni);
   void StoreLeaf(CHeightInfo hi, int nEmpty, CValue value, const CBoni& boni);

   // correction routines
   void AllSubnodesInBook();
   void AssignBranchValues(const CBookValue& valuesNew);
   void IncrementGameCount(int iGameType);
   void UpgradeToBranch();
   void CorrectBranchness();

   // Information
   CValue Cutoff() const { return cutoff; }
   const CHeightInfo& Hi() const;
   const CBookValue& Values() const;

   bool IsMoreImportantThan(const CBookData& bd2) const;

   void Out(std::ostream& os, bool fReverse=0) const;

   bool IsLeaf() const;
   bool IsBranch() const;
   bool IsUleaf() const;
   bool IsSolved() const;

   int GameCount() const;
   bool IsPrivate() const;

   // these are here for a very bad reason, don't use them
   bool IsRoot() const;
   void SetRoot(bool fRoot);

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      archive & hi;
      archive & cutoff;
      archive & values;
      archive & nGames[0];
      archive & nGames[1];
      archive & fRoot;
   }

protected:
   CHeightInfo hi;
   CValue cutoff;
   CBookValue values;
   u4 nGames[2];

   bool fRoot;	// true if has been the root of a search tree => branch or solved

   enum TLeafness { kULeaf, kBranch, kSolved } ;
   TLeafness Leafness() const;

   friend class CBook;
};

inline std::ostream& operator<<(std::ostream& os, const CBookData& bd) { bd.Out(os); return os; }

class CBookType {
public:
   char bookName[200];
};

inline bool operator<(const CBookType&a, const CBookType&b) {return strcmp(a.bookName, b.bookName)<0;}

const int nEmptyBookMax=NN-3;

struct no_save_type
{};

static no_save_type no_save;

class CBook {
public:
   static CBookPtr FindBook(char evalType, char coeffSet, CCalcParamsPtr pcp);
   static void Clean();

   CBook(const char* filename);
   CBook(const char* filename, no_save_type);
   ~CBook();

   bool SetBoni(const CBoni& aBoni);
   void SetComputer(CPlayerComputerPtr pComputer);

   const CBookData* FindMinimal(const CBitBoard& board) const;
   /* */ CBookData* FindMinimal(const CBitBoard& board);
   const CBookData* FindMinimal(const CBitBoard& board, int nEmpty) const;
   /* */ CBookData* FindMinimal(const CBitBoard& board, int nEmpty);
   const CBookData* FindAnyReflection(const CBitBoard& board) const;
   /* */ CBookData* FindAnyReflection(const CBitBoard& board);
   const CBookData* FindAnyReflection(const CBitBoard& board, int nEmpty) const;
   /* */ CBookData* FindAnyReflection(const CBitBoard& board, int nEmpty);

   // load info from book. return TRUE if found
   bool Load(const CBitBoard& board, CHeightInfo hi, CValue alpha, CValue beta, CValue& value);
   bool Load(const CBitBoard& board, CHeightInfo hi, CValue alpha, CValue beta, CValue& value, int nEmpty);
   bool GetRandomMove(const CQPosition& pos, const CSearchInfo& si, CMVK& mvk) const;
   bool GetEdmundMove(const CBitBoard& board, bool fBlackMove, CMoveValue& mv, bool fPrintEdmund, bool timeOut1, bool timeOut2) const;
   int NEdmundNodes() const;

   // store info in book
   void StoreLeaf(const CBitBoard& board, CHeightInfo hi, CValue value);
   void StoreRoot(const CBitBoard& board, CHeightInfo hi, CValue value, CValue vCutoff, bool fFull);

   // get info
   int NEmptyMin() const;
   const CBoni& Boni() const;

   // save data to file
   void Mirror();
   void WriteErr();
   void Write();
   void WriteVersion0();
   void WriteVersion1(int pruneHeight);

   void ReadErr();
   void ReadVersion0(FILE* fp);
   void ReadVersion1(FILE* fp);

   // Correction and assignment
   void CorrectAll(int& nSearches);
   void CorrectPosition(CQPosition pos, int& nSearches);
   void CorrectPosition(CQPosition pos, CBookData* bd, int& nSearches);
   void CorrectGame(const COsGame& game,
                    int iGameType,
                    int& nSearches,
                    bool fEdmundAfter,
                    Timer<double>& timer,
                    int movesToAdd);
   void PlayEdmundGame(const CBitBoard& board, bool fBlackMove, CMoveValue& mv, Timer<double>& timer);
   void PlayEdmundGames();
   void JoinBook(const char* fn);
   void JoinBook(const CBook& book);

   template<typename T,
           typename UserAllocator=boost::default_user_allocator_new_delete>
   class fixed_fast_pool_allocator : public boost::fast_pool_allocator<T,UserAllocator>
   {
      typedef boost::fast_pool_allocator<T,UserAllocator> super;
   public:
      template<typename U>
      struct rebind
      {
         typedef fixed_fast_pool_allocator<U, UserAllocator> other;
      };
      fixed_fast_pool_allocator()
      {}
      fixed_fast_pool_allocator(const fixed_fast_pool_allocator&x)
         : super(x)
      {}
      template<typename U>
      fixed_fast_pool_allocator(const fixed_fast_pool_allocator<U,UserAllocator>& x)
         : super(x)
      {}
      bool release_memory() const
      {
         return boost::singleton_pool<boost::fast_pool_allocator_tag, sizeof(T)>::release_memory();
      }
   }; 
   template<typename UserAllocator>
   class fixed_fast_pool_allocator<void,UserAllocator>
   {
   public:
      typedef void* pointer;
      typedef const void* const_pointer;
      typedef void value_type;
      template<typename U> struct rebind
      {
         typedef fixed_fast_pool_allocator<U, UserAllocator> other;
      };
   };

   // data
   typedef fixed_fast_pool_allocator<std::pair<CBitBoard, CBookData>> FastPoolAlloc;
   typedef boost::unordered_map<CBitBoard, CBookData/*, boost::hash<CBitBoard>, std::equal_to<CBitBoard>, FastPoolAlloc*/> BookType;

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      for( int i=0; i<nEmptyBookMax; i++ ) {
         archive & entries[i];
      }
   }

   const std::string& Bookname() const;

   //! Get prune height. This is the level where book
   //! positions with less empties are pruned (erased).
   int GetPruneHeight() const;

   //! These functions will prune positions in the book.
   void Prune(int pruneHeight);
   void Prune();

   CBookPtr ExtractPositions(const VariationCollection& games) const;
   CBookPtr GetAddedOrUpdated() const;

   std::size_t Positions() const;

private:

   BookType entries[nEmptyBookMax];
   std::string bookname;
   static std::map<CBookType, CBookPtr> bookList;
   CBoni boni;
   CPlayerComputerPtr pComputer;
   time_t tLastWrite;
   int nHashErr;
   /*const*/ bool save_when_closing;
   typedef std::vector<std::pair<CBitBoard, CBookData> > Updated;
   Updated updated;

   void AddData(CBitBoard bitboard, const CBookData& bd);
   void Init(const char* filename);
   void Mirror(int pruneHeight);
   void Write(int pruneHeight);
   void IncreaseHeight(CQPosition pos, CBookData* bd, CHeightInfo hi, int& nSearches);
   void MaxSubnodeValues(const CQPosition& pos,
                         CBookData* bd,
                         CBookValue& bv,
                         CBookValue& bvUleaf,
                         CMoves& movesNonbook,
                         int& nSearches);
   void EndgameError(const CQPosition& pos, const CBookData* bd, const CBookValue& bv, int& nSearches);
};

inline const CBoni& CBook::Boni() const { return boni;}

inline void CBookValue::MinValues() {
   vHeuristic=vBlack=vWhite=-kInfinity;
   fWLDSolved=false;
}

inline bool CBookValue::Win() const {
   return fWLDSolved && vHeuristic>0;
}

inline bool CBookValue::Draw() const {
   return fWLDSolved && vHeuristic==0;
}

inline bool CBookValue::Loss() const {
   return fWLDSolved && vHeuristic<0;
}

inline bool CBookValue::IsSolved() const {
   return fWLDSolved!=0;
}

inline const CHeightInfo& CBookData::Hi() const {
   return hi;
}

inline const CBookValue& CBookData::Values() const {
   return values;
}

inline bool CBookData::IsLeaf() const {
   return !IsBranch();
}

inline int CBookData::GameCount() const {
   return nGames[0]+nGames[1];
}

inline bool CBookData::IsPrivate() const
{
   return nGames[0]>0 && nGames[1]==0;
}

inline bool CBookData::IsBranch() const {
   return fRoot && !IsSolved();
}

inline bool CBookData::IsUleaf() const {
   return !fRoot && !IsSolved();
}

inline bool CBookData::IsSolved() const {
   return values.IsSolved();
}

inline bool CBookData::IsRoot() const {
   return fRoot;
}

inline void CBookData::SetRoot(bool afRoot) {
   fRoot=afRoot;
}

inline bool CBoni::operator==(const CBoni& b) {
   return blackBonus==b.blackBonus && whiteBonus==b.whiteBonus && drawBonus==b.drawBonus;
}

inline CBoni::CBoni(): blackBonus(kMaxBonus), drawBonus(kMaxBonus), whiteBonus(-kMaxBonus) {
}

inline bool CBoni::operator!=(const CBoni& b) {
   return blackBonus!=b.blackBonus || whiteBonus!=b.whiteBonus || drawBonus!=b.drawBonus;
}

inline std::istream& operator>>(std::istream& is, CBoni& boni) {
   return is >> boni.blackBonus >> boni.drawBonus >> boni.whiteBonus;
}

#endif // _BOOK_H
