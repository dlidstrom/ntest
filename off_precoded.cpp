#include "Utils.h"
#include "off.h"
#include "Pos2All.h"
#include "TreeDebug.h"
#include "Debug.h"

#define FFK

void InitNewFF() {
   cerr << "Init New FF...";

   void InitCountTables();
   void InitFlipTables();

   InitCountTables();
   InitFlipTables();

   cerr << "Done\n";
}

// the square is occupied, or something else is wrong, we shouldn't have come here
int BadFF(const CHAB& hab) {
   QSSERT(0);
   return -kInfinity;
}

int BadFFM(const CHABM& habm) {
   QSSERT(0);
   return -kInfinity;
}

int BadCF() {
   QSSERT(0);
   return -kInfinity;
}

void ClearFfs(TFF *ffs[7][7]) {
   int left, right;
   for (left=0; left<7; left++)
      for (right=0; right<7; right++)
         ffs[left][right]=BadFF;
}

void SetFfs(TFF *ffs[7][7], TFF* fftable[], int length, int loc, bool fMover) {
   int config;
   int size=lengthToSize(length);
   int left, right;

   for (config=0; config<size; config++) {
      if (NFlipped(config, length, loc, fMover, left, right))
         fftable[config]=BadFF;
      else
         fftable[config]=ffs[left][right];
   }
}

void ClearFFM(TFFM *ffs[7][7]) {
   int left, right;
   for (left=0; left<7; left++)
      for (right=0; right<7; right++)
         ffs[left][right]=BadFFM;
}

void SetFFMs(TFFM *ffs[7][7], TFFM* fftable[], int length, int loc, bool fMover) {
   int config;
   int size=lengthToSize(length);
   int left, right;

   for (config=0; config<size; config++) {
      if (NFlipped(config, length, loc, fMover, left, right))
         fftable[config]=BadFFM;
      else
         fftable[config]=ffs[left][right];
   }
}

// return the value of the game from Black's point of view.
//	hab.alpha and hab.beta are from Black's point of view
//	hab.nEmpty is current
//	hab.nDiscDiff is from Black's point of view
#if CHECK_NODES
int AB_BlackToMove_(const CHAB& hab) {
#else
   int AB_BlackToMove(const CHAB& hab) {
#endif
      nSNodesQuick++;
      if (hab.nEmpty==1) {
         return hab.nDiscDiff+emptyHead.next->Terminal_Black();
      }
      else {
         CHAB habSub;
         int best,value;
         CEmpty* pem;
         int nFlipped;

         best=-kInfinity;

         // does black have a move?
         habSub.nEmpty=hab.nEmpty-1;
         habSub.alpha=-hab.beta;
         habSub.beta=-hab.alpha;
         for (pem=emptyHead.next; pem!=emEOL; pem=pem->next) {
            nFlipped=(pem->Count_Black)();
            if (nFlipped) {
               habSub.nDiscDiff=-(nFlipped+nFlipped+1+hab.nDiscDiff);
               pem->Remove();
               value=-pem->AB_Black(habSub);
               pem->Add();
               if (value>best) {
                  best=value;
                  if (value>hab.beta)
                     return value;
                  if (value>-habSub.beta)
                     habSub.beta=-value;
               }
            }
         }
         if (best!=-kInfinity)
            return best;

         // does white have a move?
         nSNodesQuick++;
         habSub.alpha=hab.alpha;
         habSub.beta=hab.beta;
         best=kInfinity;
         for (pem=emptyHead.next; pem!=emEOL; pem=pem->next) {
            nFlipped=(pem->Count_White)();
            if (nFlipped) {
               habSub.nDiscDiff=hab.nDiscDiff-(nFlipped+nFlipped+1);
               pem->Remove();
               value=pem->AB_White(habSub);
               pem->Add();
               if (value<best) {
                  best=value;
                  if (value<hab.alpha)
                     return value;
                  if (value<habSub.beta)
                     habSub.beta=value;
               }
            }
         }
         if (best!=kInfinity)
            return best;
         else
            return hab.nDiscDiff;
      }
   }

#if CHECK_NODES
   int AB_BlackToMove(const CHAB& hab) {
      char sBoardText[NN+1];
      int value=AB_BlackToMove_(hab);
      CheckValue(GetText(sBoardText),hab.alpha,hab.beta,true,value);
      return value;
   }
#endif

// return the value of the game from White's point of view
//	hab.alpha and hab.beta are from White's point of view
//	hab.nEmpty is current
//	hab.nDiscDiff is from White's point of view
#if CHECK_NODES
   int AB_WhiteToMove_(const CHAB& hab) {
#else
      int AB_WhiteToMove(const CHAB& hab) {
#endif
         nSNodesQuick++;
         if (hab.nEmpty==1) {
            return hab.nDiscDiff+(emptyHead.next->Terminal_White)();
         }
         else {
            CHAB habSub;
            int best,value;
            CEmpty* pem;
            int nFlipped;

            best=-kInfinity;

            // does white have a move?
            habSub.nEmpty=hab.nEmpty-1;
            habSub.alpha=-hab.beta;
            habSub.beta=-hab.alpha;
            for (pem=emptyHead.next; pem!=emEOL; pem=pem->next) {
               nFlipped=(pem->Count_White)();
               if (nFlipped) {
                  habSub.nDiscDiff=-(nFlipped+nFlipped+1+hab.nDiscDiff);
                  pem->Remove();
                  value=-pem->AB_White(habSub);
                  pem->Add();
                  if (value>best) {
                     best=value;
                     if (value>hab.beta)
                        return value;
                     if (value>-habSub.beta)
                        habSub.beta=-value;
                  }
               }
            }
            if (best!=-kInfinity)
               return best;

            // does black have a move?
            habSub.alpha=hab.alpha;
            habSub.beta=hab.beta;
            best=kInfinity;

            for (pem=emptyHead.next; pem!=emEOL; pem=pem->next) {
               nFlipped=(pem->Count_Black)();
               if (nFlipped) {
                  habSub.nDiscDiff=hab.nDiscDiff-(nFlipped+nFlipped+1);
                  pem->Remove();
                  value=pem->AB_Black(habSub);
                  pem->Add();
                  if (value<best) {
                     best=value;
                     if (value<hab.alpha)
                        return value;
                     if (value<habSub.beta)
                        habSub.beta=value;
                  }
               }
            }
            if (best!=kInfinity)
               return best;
            else
               return hab.nDiscDiff;
         }
      }

#if CHECK_NODES
      int AB_WhiteToMove(const CHAB& hab) {
         char sBoardText[NN+1];
         int value=AB_WhiteToMove_(hab);
         CheckValue(GetText(sBoardText),hab.alpha,hab.beta,false,value);
         return value;
      }
#endif
