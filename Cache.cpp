// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// Cache for transposition table and move ordering

#include "PreCompile.h"
#include "Debug.h"
#include "Cache.h"
#include "options.h"
const bool printCacheStores=false;

#define CACHE_STATS 0
#if CACHE_STATS
#define UPDATE_CACHE_STATS queries++; if (result) writes++;
#define UPDATE_CACHE_STATS2 readMoves++; if (result->data.Importance()>=Importance(height,aPrune,nEmpty)) readValues++;
#else
#define UPDATE_CACHE_STATS
#define UPDATE_CACHE_STATS2
#endif

// if nMPCCache is 1, we allow the cache to return an MPC value instead of a full-width value
//	when we're not solving.
// if it is 2, we allow the cache to return an MPC value instead of a FW value when the MPC value is
//	to a strictly greater height than the FW value requested.
// if it is 0, we never return an MPC value instead of a full-width value.
const int nMPCCache=2;

int CCacheData::Importance(int aHeight, int aPrune, int nEmpty) {
   // boost depth by 2 when solving
   switch(nMPCCache) {
   case 2:
      if (!aPrune && nEmpty<=hSolverStart+aHeight)
         return (aHeight+2)<<8;
      else
         return (aHeight<<8)-aPrune;
   case 0:
   case 1:
   default:
      if (aPrune==0 && nEmpty<=hSolverStart+aHeight)
         return aHeight+2;
      else
         return aHeight;
   }
}

int CCacheData::Importance(const CHABM& hab) {
   // boost depth by 2 when solving
   switch(nMPCCache) {
   case 2:
      if (!hab.iPrune && hab.nEmpty<=hSolverStart+hab.height)
         return (hab.height+2)<<8;
      else
         return (hab.height<<8)-hab.iPrune;
   case 0:
   case 1:
   default:
      _ASSERT(0);
   }
}

// return true if we can use the value from the cache
inline bool CCacheData::Loadable(const CHABM& habm) const {
   return habm.height<height || (habm.height==height && habm.iPrune>=iPrune);
}

inline bool CCacheData::Loadable(int aHeight, int aPrune, int anEmpty) const {
   switch(nMPCCache) {
   case 2:
      //return (aHeight<height && aHeight<anEmpty-hSolverStart) || (aHeight==height && aPrune>=iPrune);
      return aHeight<height || (aHeight==height && aPrune>=iPrune);
   case 1:
      return Importance(aHeight, aPrune, anEmpty)<=Importance(height,iPrune, nEmpty);
   case 0:
   default:
      return aHeight<=height && aPrune>=iPrune;
   }
}

// return true if the value can be stored in the cache
inline bool CCacheData::Storeable(int aHeight, int aPrune,int anEmpty) const {
   return Importance(aHeight, aPrune, anEmpty)>Importance(height,iPrune, nEmpty) || (aHeight>=height && aPrune<=iPrune);
}

// return true if the value can be stored in the cache
inline bool CCacheData::Storeable(const CHABM& habm) const {
   return Importance(habm)>Importance(height,iPrune, nEmpty) || (habm.height>=height && habm.iPrune<=iPrune);
}

// determine whether to replace an old cache entry with this one
bool CCacheData::Replaceable(int aHeight, int aPrune, int anEmpty) const {
   return Importance(aHeight, aPrune, anEmpty)>Importance(height,iPrune, nEmpty);
}

bool CCacheData::Replaceable(const CHABM& hab) const {
   return Importance(hab)>Importance(height,iPrune, nEmpty);
}

bool CCacheData::Load(int aHeight, int aPrune, int nEmpty, CValue alpha, CValue beta, CMove& aBestMove,
                      int& aiFastestFirst, CValue& searchAlpha, CValue& searchBeta, CValue& value) {

   if (printCacheStores)
      printf("Loading from cache at %lx: [%hd, %hd] at height %d/%d - window is (%hd,%hd)\n",this,lBound, uBound, height, aPrune, searchAlpha, searchBeta);
   //	printf("Loading from cache: [%hd, %hd] at height %d/%d - window is (%hd,%hd)\n",lBound, uBound, height, aPrune, searchAlpha, searchBeta);

   aBestMove=bestMove;
   aiFastestFirst=iFastestFirst;

   if (Loadable(aHeight, aPrune, nEmpty)) {
      // check for immediate return conditions via cutoff
      QSSERT(aHeight<=height);

      if (lBound>=beta || lBound==uBound) {
         value=lBound;
         return true;
      }
      if (uBound<=alpha) {
         value=uBound;
         return true;
      }

      // update searchAlpha and searchBeta
      searchAlpha=Max(alpha, lBound);
      searchBeta=Min(beta, uBound);
      QSSERT(searchAlpha<searchBeta);
   }
   else {
      searchAlpha=alpha;
      searchBeta=beta;
   }

   return false;
}

// Load data from cache
// cache bitboard [POV LMO]
// values [POV LMO]
bool CCacheData::Load(CHABM& habm, int& iffCache, CMoveValue& mv) const {

   if (printCacheStores)
      printf("Loading from cache at %lx: [%d, %d] at height %d/%d - window is (%d,%d)\n",
             this,lBound, uBound, habm.height, habm.iPrune, habm.alpha, habm.beta);
   //	printf("Loading from cache: [%hd, %hd] at height %d/%d - window is (%hd,%hd)\n",lBound, uBound, height, aPrune, searchAlpha, searchBeta);

   mv.move=bestMove;
   iffCache=iFastestFirst;

   if (Loadable(habm)) {
      // check for immediate return conditions via cutoff
      QSSERT(habm.height<=height);

      if (lBound>=habm.beta || lBound==uBound) {
         mv.value=lBound;
         return true;
      }
      if (uBound<=habm.alpha) {
         mv.value=uBound;
         return true;
      }

      // update searchAlpha and searchBeta
      if (habm.alpha<lBound)
         habm.alpha=lBound;
      if (habm.beta>uBound)
         habm.beta=uBound;
      QSSERT(habm.alpha<habm.beta);
   }

   return false;
}

// return true if the node will alpha-cutoff. Used in move ordering.
bool CCacheData::AlphaCutoff(int aHeight, int aPrune, int nEmpty, CValue alpha) const {

   if (printCacheStores)
      printf("Checking Alpha Cutoff from cache at %lx: [%hd, %hd] at height %d/%d - alpha is %d\n",this,lBound, uBound, height, aPrune, alpha);
   //	printf("Loading from cache: [%hd, %hd] at height %d/%d - window is (%hd,%hd)\n",lBound, uBound, height, aPrune, searchAlpha, searchBeta);

   return (Loadable(aHeight, aPrune, nEmpty) && uBound<=alpha);
}


// store info in cache.
void CCacheData::Store(int aHeight, int aPrune, int anEmpty, CMove aBestMove, int aiFastestFirst, CValue searchAlpha, CValue searchBeta, CValue& value) {
   _ASSERT(!abortRound);
   _ASSERT(anEmpty);
   _ASSERT(aHeight<=anEmpty-hSolverStart);

   // If we can use the value from the cache, constrain the return value to be in [lBound, uBound]
   if (Loadable(aHeight, aPrune, anEmpty) && !Storeable(aHeight, aPrune, anEmpty)) {
      if (value<lBound)
         value=lBound;
      else if (value>uBound)
         value=uBound;
      return;
   }
   // If the new search height is higher than the old search height, replace the old stuff
   if (Replaceable(aHeight, aPrune, anEmpty)) {
      height=aHeight;
      iPrune=aPrune;
      lBound=-kInfinity;
      uBound=kInfinity;
      nEmpty=anEmpty;
   }
   if (Storeable(aHeight, aPrune, anEmpty)) {
      QSSERT(aBestMove.Row()>=0);
      bestMove=aBestMove;
      iFastestFirst|=aiFastestFirst;
      height=aHeight;
      iPrune=aPrune;
      nEmpty=anEmpty;

      QSSERT(searchAlpha<searchBeta && lBound<=uBound);

      if (value<searchBeta && value<uBound) {
         if(value<lBound) {
            //QSSERT(::iPruneMidgame || ::iPruneEndgame);
            lBound=value;
         }
         uBound=value;
      }

      if (value>searchAlpha && value>lBound) {
         if(value>uBound) {
            //QSSERT(::iPruneMidgame || ::iPruneEndgame);
            uBound=value;
         }
         lBound=value;
      }

      if (printCacheStores)
         printf("Added to cache at %lx: [%hd, %hd] at height %d - window was (%hd,%hd)\n",this,lBound, uBound, height,searchAlpha, searchBeta);
      //	printf("Added to cache: [%hd, %hd] at height %d - window was (%hd,%hd)\n",lBound, uBound, height,searchAlpha, searchBeta);
   }
}

// store info in cache. Update value if the search was lower than the cache search and value was outside the cache bounds.
void CCacheData::Store(const CHABM& habm, int iffCache, CMoveValue& mv) {
   _ASSERT(!abortRound);
   _ASSERT(habm.nEmpty);
   _ASSERT(habm.height<=habm.nEmpty-hSolverStart);

   // If we can use the mv.value from the cache, constrain the return mv.value to be in [lBound, uBound]
   if (Loadable(habm) && !Storeable(habm)) {
      if (mv.value<lBound)
         mv.value=lBound;
      else if (mv.value>uBound)
         mv.value=uBound;
      return;
   }
   // If the new search height is higher than the old search height, replace the old stuff
   if (Replaceable(habm)) {
      height=habm.height;
      iPrune=habm.iPrune;
      lBound=-kInfinity;
      uBound=kInfinity;
      nEmpty=habm.nEmpty;
   }
   if (Storeable(habm)) {
      QSSERT(mv.move.Square()>=0 && mv.move.Square()<64);
      bestMove=mv.move;
      iFastestFirst|=iffCache;
      height=habm.height;
      iPrune=habm.iPrune;
      nEmpty=habm.nEmpty;

      QSSERT(habm.alpha<habm.beta && lBound<=uBound);

      if (mv.value<habm.beta && mv.value<uBound) {
         if(mv.value<lBound) {
            //QSSERT(::iPruneMidgame || ::iPruneEndgame);
            lBound=mv.value;
         }
         uBound=mv.value;
      }

      if (mv.value>habm.alpha && mv.value>lBound) {
         if(mv.value>uBound) {
            //QSSERT(::iPruneMidgame || ::iPruneEndgame);
            uBound=mv.value;
         }
         lBound=mv.value;
      }

      if (printCacheStores)
         printf("Added to cache at %lx: [%hd, %hd] at height %d - window was (%d,%d)\n",this,lBound, uBound, height,habm.alpha, habm.beta);
      //	printf("Added to cache: [%hd, %hd] at height %d - window was (%hd,%hd)\n",lBound, uBound, height,searchAlpha, searchBeta);
   }
}

ostream& CCacheData::OutData(ostream& os) const {
   os << bestMove;
   os << "[" << lBound << "," << uBound << "] @ " << int(height) << ":" << int(iPrune) << "-" << int(nEmpty) << "\n";
   return os;
}

void CCacheData::Clear() {
   // store an impossible position to prevent collisions
   board.Impossible();
   // make stale to allow overwrites
   SetStale();
   // set other stuff to 0 for debugging purposes
   height=iPrune=nEmpty=iFastestFirst=0;
   lBound=uBound=0;
}

void CCacheData::Initialize(const CBitBoard& aBoard, int aheight, int aPrune, int nEmpty) {
   board=aBoard;
   height=aheight;
   iPrune=aPrune;
   nEmpty=nEmpty;
   fStale=false;
   lBound=-kInfinity;
   uBound= kInfinity;
   bestMove.Set(-1);
   iFastestFirst=0;
}

void CCacheData::Initialize(const CBitBoard& aBoard, const CHABM& hab) {
   board=aBoard;
   height=hab.height;
   iPrune=hab.iPrune;
   nEmpty=hab.nEmpty;
   fStale=false;
   lBound=-kInfinity;
   uBound= kInfinity;
   bestMove.Set(-1);
   iFastestFirst=0;
}

void CCacheData::Print(bool blackMove) const {
   board.Print(blackMove);
   printf(fStale?"stale ":"nonstale ");
   OutData(cout);
}

/////////////////////////////////////////////////
// CCache class
/////////////////////////////////////////////////

CCache::CCache(u4 anbuckets) {
   fprintf(stderr, "Creating cache with %d buckets (%d MB)\n",anbuckets, anbuckets*sizeof(CCacheData)>>20);
   nBuckets=anbuckets;
   mask=nBuckets-1;
   buckets=new CCacheData[nBuckets];
   QSSERT(buckets);
   QSSERT((nBuckets&(nBuckets-1))==0);

   Clear();
   ClearStats();
}

CCache::~CCache() {
   delete[] buckets;
}

void CCache::Clear() {
   CCacheData* i;

   for (i=buckets; i<buckets+nBuckets; i++)
      i->Clear();
   staleCount=0;
}

int CCache::CopyData(const CCache& cache2) {
   if (nBuckets!=cache2.nBuckets)
      return -2;
   else {
      memcpy(buckets, cache2.buckets, nBuckets*sizeof(CCacheData));
      return 0;
   }
}

void CCache::SetStale() {
   int i;

   //cout << "+++ ";
   for (i=0; i<nBuckets; i++)
      buckets[i].SetStale();
}

void CCache::PrintStats() const {
   printf(" cache: %6ld queries, %6ld read moves, %6ld read values, %6ld writes\n",
          queries, readMoves, readValues, writes);
}

void CCache::ClearStats() {
   queries=readMoves=readValues=writes=0;
}

// FindOld -- find an entry in the cache. If there is no entry return NULL.
//	If there is an entry set its stale flag to false and return it.
CCacheData* CCache::FindOld(const CBitBoard& board, u4 hash) {
   CCacheData* result, *result2;

   hash&=nBuckets-1;
   result=buckets+hash;
   result2=buckets+(hash^1);

   // is this position in cache?
   if (result->board==board) {
      result->SetStale(false);
   }
   else if (result2->board==board) {
      result=result2;
      result->SetStale(false);
   }
   else
      result=0;

   return result;
}

// FindNew -- find an entry in the cache. If there is no entry create one, with height and iPrune preset.
//	Return the cache entry.
CCacheData* CCache::FindNew(const CBitBoard& board, u4 hash, int height, int aPrune, int anEmpty) {
   CCacheData* result, *result2;

   hash&=nBuckets-1;
   result=buckets+hash;
   result2=buckets+(hash^1);

   // is this position in cache?
   if (result->board==board) {
      result->SetStale(false);
   }
   else if (result2->board==board) {
      result=result2;
      result->SetStale(false);
   }

   // This position isn't the one in the cache...
   else if (result->fStale || result->Replaceable(height, aPrune, anEmpty)) {
      result->Initialize(board, height, aPrune, anEmpty);
   }
   else if (result2->fStale || result2->Replaceable(height, aPrune, anEmpty)) {
      result=result2;
      result->Initialize(board, height, aPrune, anEmpty);
   }

   else
      result=0;

   UPDATE_CACHE_STATS;

   return result;
}

// FindNew -- find an entry in the cache. If there is no entry create one, with height and iPrune preset.
//	Return the cache entry.
CCacheData* CCache::FindOrCreate(const CBitBoard& board, int hash, const CHABM& hab) {
   CCacheData* result, *result2;
   hash&=mask;

   result=buckets+hash;
   result2=buckets+(hash^1);

   // is this position in cache?
   if (result->board==board) {
      result->SetStale(false);
   }
   else if (result2->board==board) {
      result=result2;
      result->SetStale(false);
   }

   // This position isn't the one in the cache...
   else if (result->fStale || result->Replaceable(hab)) {
      result->Initialize(board, hab);
   }
   else if (result2->fStale || result2->Replaceable(hab)) {
      result=result2;
      result->Initialize(board, hab);
   }

   else
      result=0;

   UPDATE_CACHE_STATS;

   return result;
}

// FindNew -- find an entry in the cache. If there is no entry return NULL.
//	Return the cache entry.
CCacheData* CCache::Find(const CBitBoard& board, const CHABM& hab) {
   CCacheData* result, *result2;
   int hash=board.Hash()&mask;

   result=buckets+hash;
   result2=buckets+(hash^1);

   // is this position in cache?
   if (result->board==board) {
      result->SetStale(false);
   }
   else if (result2->board==board) {
      result=result2;
      result->SetStale(false);
   }

   else
      result=0;

   UPDATE_CACHE_STATS;

   return result;
}

void CCache::Prepare() {
   ::cache=this;
   tSetStale=nBuckets*1E-7/dGHz;
}
