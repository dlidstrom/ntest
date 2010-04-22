// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// Cache header file

#pragma once

#ifdef NDEBUG
#undef _DEBUG
#endif

#include "BitBoard.h"
#include "Moves.h"
#include "off.h"
#include "Debug.h"

class CCacheData {
public:
   CCacheData();
   CCacheData(CValue anLBound, CValue aUBound, int aHeight, int aPrune, int nEmpty);

   void Clear();
   void Initialize(const CBitBoard& board, int height, int iPrune, int nEmpty);
   void Initialize(const CBitBoard& board, const CHABM& hab);
   bool Loadable(int aheight, int aPrune, int nEmpty) const;
   bool Loadable(const CHABM& habm) const;
   bool Storeable(int aheight, int aPrune, int nEmpty) const;
   bool Storeable(const CHABM& habm) const;
   bool Replaceable(int aheight, int aPrune, int nEmpty) const;
   bool Replaceable(const CHABM& habm) const;
   static int Importance(int aheight, int aPrune, int nEmpty);
   static int Importance(const CHABM& habm);

   // misc
   void SetStale(bool fNewStale=true);

   // info
   const CBitBoard& Board() const;

   // moves
   CMove BestMove() const;
   void BestMove(CMove newBestMove);

   // load info from cache. Returns TRUE if we can return value immediately
   bool Load(int height, int iPrune, int nEmpty, CValue alpha, CValue beta, CMove& bestMove,
             int& iFastestFirst, CValue& searchAlpha, CValue& searchBeta, CValue& value);
   bool Load(CHABM& habm, int& iffCache, CMoveValue& mv) const;
   bool AlphaCutoff(int height, int iPrune, int nEmpty, CValue alpha) const;

   // store info in cache.
   void Store(int height, int iPrune, int nEmpty, CMove bestMove, int iFastestFirst, CValue searchAlpha, CValue searchBeta, CValue& value);
   void Store(const CHABM& habm, int iffCache, CMoveValue& mv);

   // debugging
   void Print(bool fBlackMove) const;
   ostream& OutData(ostream& os) const;

   bool operator<(const CCacheData& b) const;

private:
   CBitBoard board;
   CValue lBound, uBound;
   u1 height, iPrune, nEmpty;

   CMove bestMove;
   u1 iFastestFirst;

   u1 fStale;

   friend class CCache;
};

inline CCacheData::CCacheData() {};
inline CCacheData::CCacheData(CValue anLBound, CValue aUBound, int aHeight,int aPrune,int anEmpty) { lBound=anLBound; uBound=aUBound; height=aHeight; iPrune=aPrune; nEmpty=anEmpty; }
inline CMove CCacheData::BestMove() const {return bestMove;}
inline void CCacheData::BestMove(CMove newBestMove) {bestMove=newBestMove;}
inline bool CCacheData::operator<(const CCacheData& b) const { return board<b.board;}
inline ostream& operator<<(ostream& os, const CCacheData& cd) { return cd.OutData(os);}
inline void CCacheData::SetStale(bool fNewStale) { fStale=fNewStale; }
inline const CBitBoard& CCacheData::Board() const { return board; }

/////////////////////////////////////////////////
// CCache class
/////////////////////////////////////////////////

class CCache {
public:
   CCache(u4 nBuckets);
   ~CCache();

   // Negamax read/write routines
   void Read(const CPosition& pos, int height, int iPrune, CValue& alpha, CValue& beta,
             CValue& value, CMove& bestMove, CMoves& moves, CValue& uBound, CValue& lBound);
   void Write(const CPosition& pos, int height, int iPrune, CValue& alpha, CValue& beta,
              CValue& value, CMove& bestMove, CMoves& moves, CValue& uBound, CValue& lBound);

   // Negamove read/write routines
   void ReadMove(const CPosition& pos, CMove& bestMove);
   void WriteMove(const CPosition& pos, int height, CMove& bestMove);

   // get ready to use it
   void Prepare();

   // copy a cache
   int CopyData(const CCache& cache2);

   // Other
   void SetStale();
   void PrintStats() const;
   void ClearStats();
   void Clear();

   CCacheData* FindOld(const CBitBoard& pos, u4 hash);
   CCacheData* FindNew(const CBitBoard& pos, u4 hash, int height, int iPrune, int nEmpty);
   CCacheData* FindOrCreate(const CBitBoard& pos, int hash, const CHABM& habm);
   CCacheData* Find(const CBitBoard& pos, const CHABM& habm);
private:
   i4 queries, readMoves, readValues, writes;
   CCacheData* buckets;
   u4 nBuckets;
   u4 mask;	// nBuckets-1
   int staleCount;
};
