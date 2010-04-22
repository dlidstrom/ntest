// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "Book.h"
#include "Debug.h"
#include "options.h"
#include "QPosition.h"
#include "Player.h"
#include "Pos2.h"
#include "Games.h"
#include "GDK/OsObjects.h"
#include "Variation.h"

#include <boost/static_assert.hpp>
#include <boost/foreach.hpp>

#include <algorithm>
#include <strstream>
#include <numeric>
#include <limits>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

bool fPrintCorrections=true;

namespace
{
   //////////////////////////////////////////////
   // Routines for picking move at random
   //////////////////////////////////////////////

   class CMVPS : public CMoveValue {
   public:
      int pass;
      const CBookData* sbd;
      void Out(ostream& os, int iLevel) const;
   };

   void CMVPS::Out(ostream& os, int iLevel) const {
      CMoveValue::Out(os);
      if (iLevel>1) {
         os << "\t";
         sbd->Out(os, pass!=1);
         os << '\n';
      }
   }

   // move-finding routines
   // Pick a move at random
   u4 LeftShift(u4 x, int n) {
      if (n>0)
         return x<<n;
      else
         return x>>-n;
   }

   const u4 nullProb=1<<10;

   // probability of picking a node (probs do not sum to 1)
   u4 Probability(u4 loss, int randomShift) {
      u4 ekt;	// energy loss / k*temperature

      ekt=LeftShift(loss,-randomShift);

      // bug in Pentium? 1024>>32 = 1024.
      if (ekt>=32)
         return 0;
      else
         return nullProb >> ekt;
   }

   void PickRandomMove(vector<CMVPS>& mvs, int randomShift, CMoveValue& chosen, bool fPrintRandomInfo) {
      int nChoices;

      QSSERT(!mvs.empty());

      // First get cutoff value
      sort(mvs.begin(), mvs.end());

      vector<int> possible_moves;

      int best_score = std::numeric_limits<int>::min();
      for (std::size_t i=0; i<mvs.size(); i++) {
         if( best_score<mvs[i].value
             || (best_score==mvs[i].value && (mvs[i].sbd->GameCount()==0 || mvs[i].sbd->IsPrivate())) ) {
            best_score = mvs[i].value;
            possible_moves.clear();
            possible_moves.push_back(i);
         }
         else if( best_score==mvs[i].value )
            possible_moves.push_back(i);
         if( mvs[i].sbd->GameCount()==0 )
            break;
      }
      nChoices = possible_moves.size();

      int i = rand()%nChoices;

      if (fPrintRandomInfo) {
         cout << "\n" << nChoices << " move" << (nChoices>1?"s":"") << " considered:";
         cout << "\nChoosing move " << possible_moves[i]+1 << "/" << mvs.size() << " - " << mvs[possible_moves[i]] << "\n";
      }

      chosen = mvs[i];
   }
}

////////////////////////////////////////////////////////////
// CBookValue
////////////////////////////////////////////////////////////

BOOST_STATIC_ASSERT(sizeof(CValue)==2);
BOOST_STATIC_ASSERT(sizeof(CBookValue)==10);
BOOST_STATIC_ASSERT(sizeof(CHeightInfo)==12);
BOOST_STATIC_ASSERT(sizeof(CBookData)==36);

CBookValue::CBookValue()
   : fSet(false),
     fAssigned(false),
     fWLDSolved(false)
{}

void CBookValue::Clear() {
   //epb = epo = 0;
   vHeuristic = vBlack = vWhite = 0;
   fWLDSolved=fSet=fAssigned=false;
}

void CBookValue::SetLeaf(CValue av, bool afWLDSolved) {
   vHeuristic=av;
   fWLDSolved=afWLDSolved;
   fSet=true;
}

void CBookValue::SetBranch(const CBookValue& valueNew) {
   vHeuristic=valueNew.vHeuristic;
   vBlack=valueNew.vBlack;
   vWhite=valueNew.vWhite;
   fSet=true;
}

// Assign - assign a value to a leaf book node
// inputs:
//	boni - winning bonuses
void CBookValue::AssignLeaf(const CBoni& boni) {
   //QSSERT(fSet);;

   vBlack=vHeuristic;
   vWhite=vHeuristic;
   if (Win()) {
      vBlack+=boni.blackBonus;
      vWhite-=boni.whiteBonus;
   }
   else if (Draw()) {
      vBlack+=boni.drawBonus;
      vWhite-=boni.drawBonus;
   }
   else if (Loss()) {
      vBlack+=boni.whiteBonus;
      vWhite-=boni.blackBonus;
   }
   fAssigned=true;
}

void CBookValue::AssignBranch() {
   QSSERT(fSet);
   fAssigned=true;
}

void CBookValue::Deassign() {
   fAssigned=false;
}

const char solvedText[5] = "?LDW";

char CBookValue::SolvedChar() const {
   if (!fWLDSolved)
      return '?';
   else if (vHeuristic==0)
      return 'D';
   else return vHeuristic>0?'W':'L';
}

void CBookValue::Out(ostream& os, bool fReverse) const {
   if (fSet) {
      if (fReverse) {
         os << std::setw(5) << -vHeuristic << SolvedChar() << " <";
         if (fAssigned)
            os << std::setw(5) << -vWhite << ":" << std::setw(5) << -vBlack;
         else
            os << "Unassigned";
         os << ">";
      }
      else {
         os << std::setw(5) << vHeuristic << SolvedChar() << " <";
         if (fAssigned)
            os << std::setw(5) << vBlack << ":" << std::setw(5) << vWhite;
         else
            os << "Unassigned";
         os << ">";
      }
   }
   else
      os << "No values";
}

CValue CBookValue::VMover(bool fBlackMove) const {
   return fBlackMove?vBlack:vWhite;
}

CValue CBookValue::VMover(bool fBlackMove, int vContempt, int nPass) const {
   CValue vUpperBound, vLowerBound;

   // calculate upper and lower possible values for draw
   if (vBlack<vWhite) {
      vUpperBound=vWhite;
      vLowerBound=vBlack;
   }
   else {
      vUpperBound=vBlack;
      vLowerBound=vWhite;
   }

   // adjust contempt factor for passes
   if (nPass)
      vContempt=-vContempt;

   // calculate value given contempt factor
   if (vContempt>vUpperBound)
      return vUpperBound;
   else if (vContempt<vLowerBound)
      return vLowerBound;
   else
      return vContempt;
}


// Merge - combine two CValues
// inputs:
//	maxval - the maximum so far
//	newval - the new value to be combined
// outputs:
//	maxval - the greater of maxval and newval
inline void ValueMerge(CValue& maxval, CValue newval) {
   if (maxval<newval)
      maxval=newval;
}

// Merge - updates a node's values given a subnode value
// inputs:
//	sub - the value of the subnode
//	pass - nonzero if the subnode had a pass
void CBookValue::Merge(const CBookValue& sbd, int pass) {
   if (pass) {
      ValueMerge(vBlack, sbd.vBlack);
      ValueMerge(vWhite, sbd.vWhite);
      ValueMerge(vHeuristic, sbd.vHeuristic);
   }
   else {
      ValueMerge(vBlack, -sbd.vWhite);
      ValueMerge(vWhite, -sbd.vBlack);
      ValueMerge(vHeuristic, -sbd.vHeuristic);
   }
}

// MergeTerminalValue - updates a node's margins given a subnode's terminal value
//	It is assumed that there was a pass before calculating vTerminal
// inputs:
//	terminalValue - the child value of the terminal subnode
void CBookValue::MergeTerminalValue(CValue vTerminal, const CBoni& boni) {
   CBookValue bv;

   bv.vHeuristic=vTerminal;
   bv.fWLDSolved=true;
   bv.AssignLeaf(boni);
   Merge(bv, 1);
}


////////////////////////////////////////////////////////////
// CBookData
////////////////////////////////////////////////////////////

CBookData::CBookData() {
   hi.Clear();
   nGames[0]=nGames[1]=0;
   cutoff=kInfinity;
   fRoot=false;
   values.Clear();
}

// Increase the height if needed. Return true if we have increased the height
bool CBookData::IncreaseHeight(const CHeightInfo& hiNew, int nEmpty) {
   bool fIncrease=hiNew>hi;

   if (fIncrease) {
      hi=hiNew;
      values.Clear();
      values.SetWLDSolved(hiNew.WLDSolved(nEmpty));
      cutoff=kInfinity;
   }

   return fIncrease;
}

// Update book data with the result of a search. If the height is at least the old height,
//	set the v value and the height.
//	If the height is greater than the old height, set cutoff = value
//	If height == old height, set cutoff = Min(old cutoff, value)
//	If rootNode, clear minimal flag
void CBookData::StoreLeaf(CHeightInfo hiNew, int nEmpty, CValue v, const CBoni& boni) {

   // update the node if the new value is more important
   if (hiNew>=hi) {
      IncreaseHeight(hiNew, nEmpty);

      if (IsLeaf()) {
         values.SetLeaf(v, hiNew.WLDSolved(nEmpty));
         values.AssignLeaf(boni);
      }
   }
}

void CBookData::StoreRoot(CHeightInfo hiNew, int nEmpty, CValue v,
                          CValue vCutoff, bool fFull, const CBoni& boni) {

   // update the node if the new value is more important
   fRoot=true;

   if (hiNew>=hi) {
      bool fIncrease=IncreaseHeight(hiNew, nEmpty);

      if (IsBranch()) {
         if (fIncrease || vCutoff<cutoff)
            cutoff=vCutoff;
      }
      else if (fFull) {
         values.SetLeaf(v, hiNew.WLDSolved(nEmpty));
         values.AssignLeaf(boni);
      }
   }
}

void CBookData::AllSubnodesInBook() {
   cutoff=-kInfinity;
}

void CBookData::AssignBranchValues(const CBookValue& valuesNew) {
   values.SetBranch(valuesNew);
   values.AssignBranch();
}

void CBookData::IncrementGameCount(int iGameType) {
   nGames[iGameType]++;
}

void CBookData::CorrectBranchness() {
   if (nGames[0] || nGames[1])
      UpgradeToBranch();
}

void CBookData::UpgradeToBranch() {
   fRoot=true;
}

void CBookData::Out(ostream& os, bool fReverse) const {
   static char cLeafness[3]= { 'u','b','s' };

   os << cLeafness[Leafness()] << ' ' << hi << " in "
      << std::setw(4) << nGames[0] << " + "
      << std::setw(4) << nGames[1] << " games: ";

   values.Out(os, fReverse);
   if (IsBranch())
      os << "; cutoff " << (fReverse?-cutoff:cutoff);
}

CBookData::TLeafness CBookData::Leafness() const {
   if (IsSolved())
      return kSolved;
   else if (IsBranch())
      return kBranch;
   else
      return kULeaf;
}

bool CBookData::IsMoreImportantThan(const CBookData& bd2) const {
   if (Leafness()>bd2.Leafness())
      return true;
   if (Leafness() < bd2.Leafness())
      return false;
   return hi > bd2.hi;
}

////////////////////////////////////////////////////////////
// CBook
////////////////////////////////////////////////////////////

map<CBookType, CBookPtr> CBook::bookList;

CBookPtr CBook::FindBook(char evalType, char coeffSet, CCalcParamsPtr pcp) {
   CBookType bookType;
   CBookPtr result;
   map<CBookType, CBookPtr>::iterator ptr;
   ostrstream os(bookType.bookName, sizeof(bookType.bookName));

   os << fnBaseDir << "coefficients/" << evalType << coeffSet << "_" << *pcp << ".book" << '\0';
   ptr=bookList.find(bookType);
   if (ptr==bookList.end()) {
      result.reset(new CBook(bookType.bookName));
      bookList[bookType]=result;
      QSSERT(result);
   }
   else {
      result=(*ptr).second;
   }
   return result;
}

void CBook::Clean() {
   bookList.clear();
}

// hash stuff for double-checking
u4 hash_a,hash_b,hash_c,hash_n;

void HashInit() {
   hash_a=hash_b=hash_c=hash_n=0;
}

void HashLong(u4 x) {
   switch(hash_n++) {
   case 0:
      hash_a+=x;
      break;
   case 1:
      hash_b+=x;
      break;
   case 2:
      hash_c+=x;
      bobMix(hash_a,hash_b,hash_c);
      hash_n=0;
      break;
   default:
      _ASSERT(0);
   }
}

void HashChunk(const void* p, int size) {
   const u4* p4 = (u4*)p;
   while (size>=4) {
      HashLong(*p4);
      p4++;
      size-=4;
   }
   const u2* p2 = (u2*)p4;
   if (size>=2) {
      HashLong(*p2);
      p2++;
   }
   const u1* p1 = (u1*)p2;
   if (size>=1) {
      HashLong(*p1);
      p1++;
   }
}

void CBook::ReadErr() {
   cerr << "WARNING: BOOK " << bookname << " is damaged, restore a backup.\n";
   _ASSERT(0);
   _exit(-3);
}

CBook::CBook(const char* filename)
   : tLastWrite(time(0)),
     nHashErr(0),
     save_when_closing(true)
{
   Init(filename);
}

CBook::CBook(const char* filename, no_save_type)
   : tLastWrite(time(0)),
     nHashErr(0),
     save_when_closing(false)
{
   Init(filename);
}

void CBook::Init(const char* filename)
{
   CBookData bd;
   CBitBoard board;
   FILE* fp;
   int nVersion;

   if (filename) {
      fp=fopen(filename,"rb");
      if (!fp) {
         switch(errno) {
         case ENOENT:
            fprintf(stderr, "WARNING: book %s doesn't exist, will be created when saving book\n",filename);
            break;
         default:
            fprintf(stderr, "WARNING: Book %s unavailable, book will start out empty and will not be saved. (errno %d)\n",filename, errno);
            filename=0;
         }
      }
      else {
         fprintf(stderr, "Loading book %s...",filename);

         if (fread(&nVersion, sizeof(nVersion), 1, fp)==0) {
            cerr << "WARNING: Book " << filename << " is empty, either restore a backup or delete the file\n";
            _exit(-3);
         }
         else {
            if (nVersion!=1)
               ReadVersion0(fp);
            else
               ReadVersion1(fp);
         }
         fclose(fp);

         fprintf(stderr, "Done\n");
      }
   }
   if (filename) {
      bookname=filename;
   }
   else
      bookname.erase(0,-1);
}

CBook::~CBook() {
   if( save_when_closing ) {
      std::cerr << "Closing book " << bookname << "..." << std::flush;
      Write();
      std::cerr << "Done\n";
   }
}
int CBook::NEmptyMin()  const {
   return hSolverStart+1;
}

// Mirror the book to disk - Write it every so often
void CBook::Mirror() {
   Mirror(0);
}

void CBook::Mirror(int pruneHeight) {
   if (time(0)>=tLastWrite+25*60) {
      std::cout << "Writing book " << bookname << "..." << std::flush;
      Write(pruneHeight);
      std::cout << "Done" << std::endl;
   }
}

int CBook::GetPruneHeight() const
{
   int pruneHeight = 0;
   if( pComputer ) {
      pruneHeight = pComputer->book_pcp ? pComputer->book_pcp->PerfectSolve() : 0;
   }
   // three perfect solved nodes, or a maximum at 24 empty
   return (std::min)(24, (std::max)(pruneHeight-2, 0));
}

void CBook::Prune(int pruneHeight)
{
   for (int nEmpties=0; nEmpties<pruneHeight; nEmpties++)
      entries[nEmpties].clear();
}

void CBook::Prune()
{
   Prune(GetPruneHeight());
}

void CBook::Write() {
   Write(GetPruneHeight());
}

void CBook::Write(int pruneHeight) {
   WriteVersion1(pruneHeight);
}

void CBook::WriteErr() {
   cerr << "WARNING: Error writing to book file " << bookname << " (errno " << errno << ")\n";
}

void CBook::ReadVersion0(FILE* fp) {
   CBitBoard board;
   CBookData bd;
   rewind(fp);

   while (fread(&board, sizeof(board), 1, fp) && fread(&bd, sizeof(bd), 1, fp)) {
      int nEmpty=CountBits(board.empty);
      if (nEmpty<nEmptyBookMax)
         entries[nEmpty][board]=bd;
      else {
         cerr << "RED ALERT: BOOK MAY BE CORRUPT\nEntry with " << nEmpty << " empties was ignored\n";
         cout << "RED ALERT: BOOK MAY BE CORRUPT\nEntry with " << nEmpty << " empties was ignored\n";
      }
   }
}

void CBook::WriteVersion1(int pruneHeight) {
   BookType::const_iterator i;
   FILE* fp;
   int nEmpties;
   int nSize, nVersion=1;

   if (bookname.empty())
      return;

   fp=fopen(bookname.c_str(),"wb");
   if (!fp) {
      cerr << "WARNING: Error opening book file " << bookname << " for writing (errno " << errno << ")\n";
      return;
   }

   // calculate nSize - number of book entries
   nSize=0;
   for (nEmpties=pruneHeight; nEmpties<nEmptyBookMax; nEmpties++)
      nSize+=entries[nEmpties].size();

   if (!fwrite(&nVersion, sizeof(nVersion), 1, fp) || !fwrite(&nSize, sizeof(nSize), 1, fp)) {
      WriteErr();
   }
   else {
      HashInit();
      for (nEmpties=nEmptyBookMax-1; nEmpties>=pruneHeight; nEmpties--) {
         const BookType& bookType = entries[nEmpties];
         for (i=bookType.begin(); i!=bookType.end(); i++) {
            const CBitBoard& bitboard = i->first;
            const CBookData& bookData = i->second;
            if (!(fwrite(&bitboard, sizeof(CBitBoard), 1, fp)
                  && fwrite(&bookData, sizeof(CBookData), 1, fp)))
            {
               WriteErr();
               fclose(fp);
               return;
            }
            HashChunk(&((*i).first), sizeof(CBitBoard));
            HashChunk(&((*i).second), sizeof(CBookData));
         }
      }
   }
   hash_a+=nHashErr;
   fwrite(&hash_a, sizeof(hash_a), 1, fp);

   fclose(fp);
   tLastWrite=time(0);
}

void CBook::ReadVersion1(FILE* fp) {
   CBitBoard board;
   CBookData bd;
   int nSize, nRead, nHashCheck;

   if (!fread(&nSize, sizeof(nSize), 1, fp))
      ReadErr();
   HashInit();	// init hash table for detection of corrupt book

   for (nRead=0; nRead<nSize && fread(&board, sizeof(board), 1, fp) && fread(&bd, sizeof(bd), 1, fp); nRead++) {
      HashChunk(&board, sizeof(board));
      HashChunk(&bd, sizeof(bd));
      int nEmpty=CountBits(board.empty);
      if (nEmpty<nEmptyBookMax)
         entries[nEmpty][board]=bd;
      else
         ReadErr();
   }
   if (nRead!=nSize)
      ReadErr();
   if (!fread(&nHashCheck, sizeof(nHashCheck), 1, fp))
      ReadErr();
   nHashErr=nHashCheck-hash_a;
   _ASSERT(nHashErr==0);
}

void CBook::WriteVersion0() {
   BookType::const_iterator i;
   FILE* fp;
   int nEmpties;

   if (!bookname.empty()) {
      fp=fopen(bookname.c_str(),"wb");
      if (!fp)
         printf("Error opening book file %s for writing (errno %d)\n",bookname.c_str(), errno);
      else {
         for (nEmpties=nEmptyBookMax-1; nEmpties>=0; nEmpties--) {
            for (i=entries[nEmpties].begin(); i!=entries[nEmpties].end(); i++)
               if (!(fwrite(&((*i).first), sizeof(CBitBoard), 1, fp) &&
                     fwrite(&((*i).second), sizeof(CBookData), 1, fp)))
                  fprintf(stderr, "WARNING: Error writing to book file %s (errno %d)\n", bookname.c_str(), errno);
         }
         fclose(fp);
         tLastWrite=time(0);
      }
   }
}

const CBookData* CBook::FindMinimal(const CBitBoard& board) const {
   return FindMinimal(board, CountBits(board.empty));
}

CBookData* CBook::FindMinimal(const CBitBoard& board) {
   return FindMinimal(board, CountBits(board.empty));
}

const CBookData* CBook::FindMinimal(const CBitBoard& board, int nEmpty) const {
   BookType::const_iterator i;

   if (nEmpty>=nEmptyBookMax)
      return 0;
   i=entries[nEmpty].find(board);
   if (i==entries[nEmpty].end())
      return 0;
   else {
      QSSERT((*i).second.Hi().height<=nEmpty);
      return &((*i).second);
   }
}

CBookData* CBook::FindMinimal(const CBitBoard& board, int nEmpty) {
   BookType::iterator i;

   if (nEmpty>=nEmptyBookMax)
      return 0;
   i=entries[nEmpty].find(board);
   if (i==entries[nEmpty].end())
      return 0;
   else {
      QSSERT((*i).second.Hi().height<=nEmpty);
      return &((*i).second);
   }
}

const CBookData* CBook::FindAnyReflection(const CBitBoard& board) const {
   return FindMinimal(board.MinimalReflection());
}

CBookData* CBook::FindAnyReflection(const CBitBoard& board) {
   return FindMinimal(board.MinimalReflection());
}

const CBookData* CBook::FindAnyReflection(const CBitBoard& board, int nEmpty) const {
   return FindMinimal(board.MinimalReflection(), nEmpty);
}

CBookData* CBook::FindAnyReflection(const CBitBoard& board, int nEmpty) {
   return FindMinimal(board.MinimalReflection(), nEmpty);
}

// read a value from the book

bool CBook::Load(const CBitBoard& board, CHeightInfo hi, CValue alpha, CValue beta, CValue& value) {
   return Load(board, hi, alpha, beta, value, CountBits(board.empty));
}

bool CBook::Load(const CBitBoard& board, CHeightInfo hi, CValue alpha, CValue beta, CValue& value, int nEmpty) {
   CBookData* bd;

   bd=FindAnyReflection(board, nEmpty);

   // no data, return false
   if (bd==NULL)
      return false;

   // data of height >= full-width search? return value
   if (bd->Hi() >= hi) {
      value=bd->Values().vHeuristic;
      return true;
   }

   // data of proper height but WLD only? See if we can return it
   hi.fWLD=true;
   if (bd->Hi() >= hi) {
      CValue vtmp = bd->Values().vHeuristic;
      if (beta<=kStoneValue && kStoneValue<=vtmp) {
         value=kStoneValue;
         return true;
      }
      else if (vtmp<=-kStoneValue && -kStoneValue<=alpha) {
         value=-kStoneValue;
         return true;
      }
      else if (vtmp==0) {
         value=0;
         return true;
      }
   }

   return false;
}

bool CBook::GetRandomMove(const CQPosition& pos, const CSearchInfo& si, CMVK& mvk) const {
   vector<CMVPS> mvs;
   CMVPS mv;
   CMove move;
   CMoves moves, submoves;
   int pass;
   const CBookData *bd, *sbd;
   CQPosition subpos;
   u4 fPrintLevel=si.PrintBookLevel();

   pos.CalcMoves(moves);

   // Find the book data. Return false if unusable.
   bd=FindAnyReflection(pos.BitBoard());
   if (!bd || bd->IsUleaf())
      return false;

   bool fSolved=bd->IsLeaf();
   QSSERT(fSolved ? bd->Values().IsSolved() : true);
   int vContempt=fSolved?0:si.vContempt;

   // Check subnodes in turn to see if they're in book
   for (move.Set(-1); moves.GetNext(move);) {
      subpos=pos;
      subpos.MakeMove(move);
      pass=subpos.CalcMovesAndPass(submoves);
      sbd=FindAnyReflection(subpos.BitBoard());

      // If subnode is in book, add it to the list.
      if (sbd) {
         mv.move=move;
         mv.value=sbd->Values().VMover(subpos.BlackMove(), vContempt, pass);
         if (!pass)
            mv.value=-mv.value;
         mv.pass=pass;
         mv.sbd=sbd;
         mvs.push_back(mv);

         // correctness checks when debugging
         if(!sbd->Values().IsAssigned()) {
            cout << "============= RED ALERT: BOOK BUG ==========\n";
            pos.Print();
            cout << mv << "\n";
            subpos.Print();
            cout << (*sbd);
            cout << "\n=========== End book bug ==================\n";
         }
      }
   }

   // check for error condition and return false if we have one
   if (mvs.empty()) {
      if (!fSolved) {
         pos.Print();
         QSSERT(0);
         printf("RED ALERT: BOOK HAS NO MOVES\n");
      }
      // restore board
      Initialize(pos.BitBoard(), pos.BlackMove());
      return false;
   }

   // sort the moves, and print them if we're debugging
   sort(mvs.begin(), mvs.end());
   if (fPrintLevel) {
      cout << "BOOK: " << *bd << "\n";
      cout << "Contempt: " << si.vContempt << "; Randomness: " << si.rs << "\n";
      if (fPrintLevel>1) {
         vector<CMVPS>::iterator i;
         for (i=mvs.begin(); i!=mvs.end(); i++)
            i->Out(cout, fPrintLevel);
         if (fPrintLevel==2)
            cout << '\n';
      }
   }

   // choose a move
   // assertion fails when we have uncorrected transpositions
   //QSSERT(mvs[0].value==bd->Values().VMover(fBlackMove) || bd->IsSolved());
   bool fUsable=!fSolved;
   int rs = si.rs;

   if (fSolved) {
      rs = 0;
      if (bd->Values().Loss()) {
         if (mvs[0].value>=0) {
            cout << "RED ALERT: Book error, Loss has a possible win or draw\n";
            fUsable = true;
         }
      }
      else if (bd->Values().Draw()) {
         if (mvs[0].value>0)
            cout << "RED ALERT: Book error, Draw has a possible win\n";
         if (mvs[0].value>=0)
            fUsable=true;
      }
      else {
         if (mvs[0].value>0)
            fUsable=true;
      }
   }

   if (fUsable) {
      PickRandomMove(mvs, rs, mvk, si.PrintBookRandomInfo()!=0);

      // really the only info from the book is the height, although we could go to the
      //	subnode to get the ply of the chosen move. But I'm too lazy to implement this
      mvk.hiFull=mvk.hiBest=bd->Hi();
   }
   Initialize(pos.BitBoard(), pos.BlackMove());
   return fUsable;
}

bool CBook::GetEdmundMove(const CBitBoard& board, bool fBlackMove, CMoveValue& mv, bool fPrintEdmund, bool timeOut1, bool timeOut2) const {
   vector<CMoveValue> mvs;
   CMove move;
   CMoves submoves;
   int pass;
   const CBookData *bd, *sbd;
   CQPosition pos(board, fBlackMove), subpos;
   CMoveValue mvBestPlayed;
   CMoves moves;


   // Find the book data. Return false if unusable.
   bd=FindAnyReflection(board);
   if (!bd || bd->IsUleaf() || bd->IsSolved() || /*bd->Hi().height+hSolverStart!=board.NEmpty() || */bd->GameCount()==0)
      return false;

   // Check subnodes in turn to see if they're in book
   mvBestPlayed.value=-kInfinity;

   pos.CalcMoves(moves);
   for (move.Set(-1); moves.GetNext(move);) {
      subpos=pos;
      subpos.MakeMove(move);
      pass=subpos.CalcMovesAndPass(submoves);
      sbd=FindAnyReflection(subpos.BitBoard());

      // If subnode is in book, add it to the list.
      if (sbd) {
         mv.move=move;
         mv.value=sbd->Values().vHeuristic;
         if (!pass)
            mv.value=-mv.value;
         if (sbd->GameCount() ) {
            if( mvBestPlayed.value<mv.value)
               mvBestPlayed=mv;
         }
         else
            mvs.push_back(mv);

         // correctness checks when debugging
         if(!sbd->Values().IsAssigned()) {
            cout << "============= RED ALERT: BOOK BUG ==========\n";
            pos.Print();
            cout << mv << "\n";
            subpos.Print();
            cout << (*sbd);
            cout << "\n=========== End book bug ==================\n";
         }
      }
   }

   // restore board
   Initialize(board, fBlackMove);

   // check for error condition and return false if we have one
   if (mvs.empty()) {
      //board.Print(fBlackMove);
      //QSSERT(0);
      //printf("RED ALERT: BOOK HAS NO MOVES\n");

      return false;
   }

   // sort the moves, and print them if we're debugging
   sort(mvs.begin(), mvs.end());

   mv=mvs[0];

   bool best_draw_or_worse = mvBestPlayed.value<=0 || mvBestPlayed.value==10000;
   bool fEdmund = (mv.move != mvBestPlayed.move)
      && ((mv.value>(timeOut1?(timeOut2?-1:-100):-200) && best_draw_or_worse) || (!timeOut1 && abs(mvBestPlayed.value)<=200 && abs(mv.value)<=200 && mv.value>mvBestPlayed.value));
   //CValue v = pComputer->cd.vContempts[pos.BlackMove()];
   //bool fEdmund= ((mv.value>v && mvBestPlayed.value<=v) || (mv.value==v && mvBestPlayed.value<v));
   if (fEdmund && fPrintEdmund) {
      cout << "--- Found an edmund position ---\n";
      cout << "Best played : " << mvBestPlayed << ".  Best deviation: " << mv << ".\n";
      board.Print(fBlackMove);
      cout << "--------------------------------\n";
   }
   return fEdmund;
}

int CBook::NEdmundNodes() const {
   int nEmpty, nGames;
   nGames=0;
   BookType::const_iterator i;
   CMoveValue mv;

   for (nEmpty=0; nEmpty<nEmptyBookMax; nEmpty++) {
      int nGamesNempty = 0;
      for (i=entries[nEmpty].begin(); i!=entries[nEmpty].end(); i++) {
         // only count edmunds >0 at draw-or-worse nodes
         if (GetEdmundMove((*i).first, true, mv, false, true, true)) {
            nGames++;
            nGamesNempty ++;
         }
      }
      if( nGamesNempty )
         std::cout << nEmpty << ":" << nGamesNempty << " ";
   }
   std::cout << std::endl;

   return nGames;
}

void CBook::PlayEdmundGame(const CBitBoard& bbNormalized, bool fBlackMove, CMoveValue& mv, Timer<double>& timer) {
   char sBoard[NN+1];
   CCalcParamsPtr temp_pcp = pComputer->search_pcp;
   pComputer->search_pcp = pComputer->book_pcp;
   CGame game(pComputer, pComputer);
   CQPosition pos(bbNormalized, fBlackMove);
   game.Initialize(pos.GetSBoard(sBoard), pos.BlackMove());
   CSGMoveListItem mli;
   mli.mv=CSGSquare(mv.move.Col(),mv.move.Row());
   mli.dEval=mv.value/(double)kStoneValue;
   mli.tElapsed=0;
   game.Update(mli);

   if (!game.GameOver()) game.Play(timer);
   pComputer->search_pcp = temp_pcp;
}

// Store a node in the book
// Inputs:
//	board - bitboard to add, doesn't need to be min reflection
//	hi - height info for node
//	value - v value

void CBook::PlayEdmundGames() {
   int nEmpty;
   int direction;
   int end;
   BookType::iterator i;
   CMoveValue mv;

   Timer<double> outer_timer;
   bool s26 = pComputer->cd.sCalcParams=="s26";
   int played = 0;
   int n_max_played;
   if( rand()%2 ) {
      // from 0 to nEmptyBookMax-1
      nEmpty = 0;
      direction = 1;
      end = nEmptyBookMax;
      n_max_played = s26 ? 10 : 100;
   }
   else {
      // from nEmptyBookMax-1 to 0
      nEmpty = nEmptyBookMax-1;
      direction = -1;
      end = -1;
      n_max_played = s26 ? 10 : 10;
   }

   // play edmunds for no more than 8 hours
   for (; outer_timer.elapsed()<8*3600 && nEmpty!=end; nEmpty+=direction) {
      bool fBlackMove=!(nEmpty&1);
      for (i=entries[nEmpty].begin(); i!=entries[nEmpty].end(); i++) {
         // only play edmunds nodes >-100
         if (GetEdmundMove((*i).first, fBlackMove, mv, true, true, false) && (/*!s26 ||*/ (rand()%20==1)) ) {
            Timer<double> timer;
            PlayEdmundGame((*i).first, fBlackMove, mv, timer);
            played ++;
            std::cout
               << "Played "
               << played
               << " edmund games. Will stop after "
               << n_max_played
               << "."
               << std::endl
               ;
            if( played == n_max_played )
               return;
         }
      }
   }
}

void CBook::JoinBook(const char* fn) {
   CBook book2(fn);
   JoinBook(book2);
}

void CBook::JoinBook(const CBook& book2)
{
   int nEmpty;
   BookType::const_iterator i;
   CBookData* pbd;

   for (nEmpty=0; nEmpty<nEmptyBookMax; nEmpty++) {
      for (i=book2.entries[nEmpty].begin(); i!=book2.entries[nEmpty].end(); i++) {
         pbd=FindMinimal((*i).first);
         const CBookData& bd2=(*i).second;
         if (pbd==NULL || bd2.IsMoreImportantThan(*pbd) ) {
            entries[nEmpty][(*i).first]=(*i).second;
         }
         if (pbd!=NULL && pbd->IsBranch() && bd2.IsBranch() && pbd->Hi()==bd2.Hi() && bd2.Cutoff() < pbd->Cutoff())
            pbd->cutoff=bd2.cutoff;
      }
   }
}

void CBook::StoreLeaf(const CBitBoard& board, CHeightInfo hi, CValue value) {
   // calculate minimal reflection, nEmpty, fWLD
   int nEmpty=CountBits(board.empty);
   CBitBoard minReflection=board.MinimalReflection();
   CBookData* bd=&(entries[nEmpty][minReflection]);

   // assign value if we can, or mark unassigned if we should
   QSSERT(hi.Valid());
   bd->StoreLeaf(hi, nEmpty, value, boni);
   QSSERT(bd->Hi().height>0);
}

void CBook::StoreRoot(const CBitBoard& board, CHeightInfo hi, CValue value, CValue vCutoff, bool fFull) {
   // calculate minimal reflection, nEmpty, fWLD
   int nEmpty=CountBits(board.empty);
   CBitBoard minReflection=board.MinimalReflection();
   CBookData* bd=&(entries[nEmpty][minReflection]);

   // assign value if we can, or mark unassigned if we should
   QSSERT(hi.Valid());
   bd->StoreRoot(hi, nEmpty, value, vCutoff, fFull, boni);
   QSSERT(bd->Hi().height>0);
}

bool CBook::SetBoni(const CBoni& aBoni) {
   bool changed;

   changed=(boni!=aBoni);
   if (changed)
      boni=aBoni;
   return changed;
}

void CBook::SetComputer(CPlayerComputerPtr apComputer) {
   pComputer=apComputer;
}

// correction and assignment

// correct and assign values to whole book, checking for transpositions
void CBook::CorrectAll(int& nSearches) {
   int nEmpty;
   CQPosition pos;

   if( !pComputer ) {
      std::cerr << "ERROR: No computer set, can not correct all." << std::endl;
      return;
   }
   bool s26 = pComputer->cd.sCalcParams=="s26";
   for (nEmpty=0; nEmpty<nEmptyBookMax; nEmpty++) {
      BookType& bookType = entries[nEmpty];

      if (nEmpty<NEmptyMin()) {
         int size=bookType.size();
         if (size) {
            cerr << "RED ALERT: BOOK MAY BE CORRUPT\nErasing " << size << " entries at " << nEmpty << "empties\n";
            cout << "RED ALERT: BOOK MAY BE CORRUPT\nErasing " << size << " entries at " << nEmpty << "empties\n";
            bookType.clear();
         }
      }
      else {
         std::cout << "Correcting at " << nEmpty << " empties" << std::flush;
         int corrected = nSearches;
         const int n_max_searches = 400;
         std::vector<double> times;

         std::size_t nPosition = 0;
         for (BookType::iterator i=bookType.begin(); i!=bookType.end(); i++) {
            if( (++nPosition%10000)==0 ) {
               std::cout << '.' << std::flush;
            }
            pos.Initialize((*i).first,true);
            CorrectPosition(pos, &((*i).second), nSearches);
            Mirror();
         }
         std::cout << std::endl;
      }
   }
}

void CBook::CorrectPosition(CQPosition pos, int& nSearches) {
   CBookData* bd=FindAnyReflection(pos.BitBoard());
   if (bd)
      CorrectPosition(pos, bd, nSearches);
   else
      QSSERT(0);
}

void CBook::EndgameError(const CQPosition& pos, const CBookData* bd, const CBookValue& bv, int& nSearches) {
#if 0	// don't alarm people, mostly this is because someone made a mistake in endgame.
	// but beware, there IS a bug and sometimes solved values are wrong
	// display information about problem
   cout << "------------- RED ALERT: Misvalued solve node -----------\n";
   cout << "Node Value: " << bd->Values() << "\n";
   cout << "Calculated value: " << bv << "\n";
   pos.Print();
#endif
#if 0	// Don't fix problem, this fix corrupts the book
   CMVK mvk;
   CSearchInfo si(pComputer->cd.iPruneMidgame, pComputer->cd.iPruneEndgame,
                  0, 0, kNeedValue, 15*60, 0);
   GetRandomMove(pos, si, mvk);

   // fix problem
   CMoves moves;
   pos.CalcMoves(moves);
   Initialize(pos.BitBoard(), pos.BlackMove());
   IterativeValue(moves, *(pComputer->pcp), si, mvk);
   nSearches++;
   bd=FindAnyReflection(pos.BitBoard());
   Mirror();
#endif
}

void CBook::CorrectPosition(CQPosition pos, CBookData* bd, int& nSearches) {
   int nEmpty=pos.NEmpty();

   QSSERT(bd->Hi().height!=0);

   // enforce a minimum height for book branch nodes and solved nodes
   // this could potentially result in a branch node being solved.
   bd->CorrectBranchness();
   if (bd->IsBranch() || bd->IsSolved()) {
      CHeightInfo hiMin = pComputer->book_pcp->MinHeight(nEmpty);
      if (bd->Hi() < hiMin)
         IncreaseHeight(pos, bd, hiMin, nSearches);
   }

   CSearchInfo si(pComputer->cd.iPruneMidgame, pComputer->cd.iPruneEndgame,
                  0, 0, kNeedMove +kNeedValue, 1e6, 0);

   // For branch nodes, get the max of the subnodes values
   if (bd->IsBranch()) {
      CBookValue bv, bvUleaf;
      CMoves movesNonbook;

      while (1) {
         MaxSubnodeValues(pos,bd, bv,bvUleaf, movesNonbook, nSearches);

         if (!movesNonbook.NMoves()) {
            bd->AllSubnodesInBook();
            break;
         }
         else if (bd->Cutoff() <= bvUleaf.vHeuristic)
            break;
         else {
            // need to value a nonbook node to get the cutoff down
            CCalcParamsFixedHeight cp(bd->Hi());
            CMVK mvk;
            Initialize(pos.BitBoard(), pos.BlackMove());
            if (fPrintCorrections)
               cout << "\n" << pos.NEmpty() << " search to get cutoff down\n";
            IterativeValue(movesNonbook, cp, si, mvk);
            nSearches++;
         }
      }
      bd->AssignBranchValues(bv);
   }
   // New code:
   else if (bd->IsSolved()) {
      CBookValue bv, bvUleaf;
      CMoves movesNonbook;

      MaxSubnodeValues(pos,bd, bv,bvUleaf, movesNonbook, nSearches);
      if (Sign(bd->Values().vHeuristic)!=Sign(bv.vHeuristic) && pos.NEmpty()>9) {
         EndgameError(pos, bd, bv, nSearches);
      }
   }


   QSSERT(nEmpty==59||bd->Values().IsSetAndAssigned());
}

// iGameType is 0 for private games, 1 for public games, -1 to not include the game in the game count

void CBook::CorrectGame(const COsGame& game,
                        int iGameType,
                        int& nSearches,
                        bool fEdmundAfter,
                        Timer<double>& timer,
                        int movesToAdd)
{
   int iMove, nMoves;
   CMoves moves;
   CBookData *bd;
   CNodeStats nsStart, nsEnd;

   nsStart.Read();

   nMoves=game.ml.size();

   // correct from back to front so changes propagate
   for (iMove=nMoves; iMove>=0 && --movesToAdd>=0; iMove--) {
      CQPosition pos(game, iMove);
      if (pos.NEmpty()>=nEmptyBookMax)
         break;

      // if this is a valid book node
      if (pos.NEmpty()>=NEmptyMin() && pos.CalcMoves(moves)) {
         if (fPrintCorrections) {
            cout<<"------------------------------------\nCorrecting Position from game:\n";
            //pos.Print();
         }
         bd=FindAnyReflection(pos.BitBoard());

         // The node might not exist because e.g. it was a forced move, or an opponent move in a non-toot
         //	game or wasn't played by this computer at all.
         // For the moment we just calculate the move and deviations using the normal timing routines.
         //	This may be problematic for calcParams that use tRemaining to determine when to cut off,
         //	I'll just set tRemaining to 15 minutes.
         if (!bd) {
            CMVK mvk;

            if (fPrintCorrections) {
               cout << "OLD: not in book\n";
               cout << "\n" << pos.NEmpty() << " search because node not in book\n";
            }
            Initialize(pos.BitBoard(), pos.BlackMove());
            CSearchInfo si(pComputer->cd.iPruneMidgame,
                           pComputer->cd.iPruneEndgame,
                           0,
                           0,
                           kNeedValue,
                           15*60,
                           0);

            IterativeValue(moves, *(pComputer->book_pcp), si, mvk);
            nSearches++;
            bd=FindAnyReflection(pos.BitBoard());
            QSSERT(bd);
         }
         else {
            if (fPrintCorrections)
               cout << "OLD: " << *bd << "\n";
         }
         if (iGameType>=0)
            bd->IncrementGameCount(iGameType);
         CorrectPosition(pos, bd, nSearches);
         // only edmund if we have been searching less than 12 hours
         timer.elapsed();
         if( fPrintCorrections ) {
            std::cout
               << "Spent "
               << int(timer.last())
               << " seconds correcting game. Will not expand more edmunds after two hours."
               << std::endl
               ;
         }
         if (fEdmundAfter && timer.last()<12*3600) {
            CMoveValue mv;
            if (GetEdmundMove(pos.BitBoard(), pos.BlackMove(), mv, true, true, true/*timer.last()>1800, timer.last()>7200*/)) {
               PlayEdmundGame(pos.BitBoard(), pos.BlackMove(), mv, timer);
               CorrectPosition(pos,bd,nSearches);
            }
         }
         if (fPrintCorrections)
            cout << "NEW: " << *bd << "\n";
      }
   }
   nsEnd.Read();

   if (fPrintCorrections) {
      cout <<"\nDone correcting game positions! ";
      cout << nSearches << " searches in " << (nsEnd-nsStart).Seconds() << "s\n";
   }
}

void CBook::IncreaseHeight(CQPosition pos, CBookData* bd, CHeightInfo hi, int& nSearches){

   // maybe after the height increase we have solved it?
   bool fWLDSolved=hi.WLDSolved(pos.NEmpty());

   // branch?
   if (!bd->IsLeaf() && !fWLDSolved) {
      QSSERT(hi.Valid());
      bd->IncreaseHeight(hi, pos.NEmpty());
      CorrectPosition(pos, bd, nSearches);
   }

   // solved? Ignore for now. This happens when we do a full-width search
   //	on a previously WLD solved position, e.g. when we are recalcing a
   //	book at a higher height.
   else if (bd->IsSolved()) {
      QSSERT(hi.height==pos.NEmpty()-hSolverStart);
      QSSERT(fWLDSolved);
      return;
   }

   // Uleaf?
   else {
      CCalcParamsFixedHeight cp(hi);
      CMVK mvk;
      CMoves moves;

      if (!pos.CalcMoves(moves)) {
         QSSERT(0);
      }

      Initialize(pos.BitBoard(), pos.BlackMove());

      // This is a total kludge. IterativeValue() will save the value as a root node,
      //	and we don't want that if we are just increasing the height of a leaf.
      //	so we store the rootness and restore it afterwards.
      bool fRoot=bd->IsRoot();

      if (fPrintCorrections) {
         hi.SetNEmpty(pos.NEmpty());
         cout << "\n" << pos.NEmpty() << " search increased-height leaf node ("
              << bd->Hi() << "->" << hi << ")\n";
      }
      CSearchInfo si(pComputer->cd.iPruneMidgame,
                     pComputer->cd.iPruneEndgame,
                     0,
                     0,
                     kNeedValue,
                     1e6,
                     0);

      IterativeValue(moves, cp, si, mvk);
      nSearches++;

      bd->SetRoot(fRoot);

      bd->StoreLeaf(hi, pos.NEmpty(), mvk.value, boni);
      QSSERT(bd->IsSolved()==fWLDSolved);
   }
   QSSERT(bd->IsSolved()==fWLDSolved);
}

// Find max subnode value, and max unsolved leaf subnode value
// Increase book height for subnodes if required.
// Inputs :
//	pos - position
//	bd - bookdata for position
// Outputs:
//	bv - book value for position
//	bvUleaf - book value of the best "unsolved leaf" node
void CBook::MaxSubnodeValues(const CQPosition& pos, CBookData* bd,
                             CBookValue& bv, CBookValue& bvUleaf, CMoves& movesNonbook, int& nSearches) {
   CMoves moves, submoves;
   CMove move;
   int pass;
   CBookData *sbd;
   CQPosition posSub;

   if (!pos.CalcMoves(moves)) {
      QSSERT(0);
      return;
   }

   // Initialize return values
   movesNonbook=moves;
   bv.MinValues();
   bvUleaf.MinValues();

   while (moves.GetNext(move)) {
      posSub=pos;
      posSub.MakeMove(move);
      pass=posSub.CalcMovesAndPass(submoves);

      // terminal subnode:
      if (pass==2) {
         bv.MergeTerminalValue(pos.TerminalValue(), boni);
      }

      // nonterminal subnode:
      else {
         sbd=FindAnyReflection(posSub.BitBoard());

         // book subnode:
         if (sbd) {

            // Increase height if too low
            if (sbd->Hi() < bd->Hi()-1)
               IncreaseHeight(posSub, sbd, bd->Hi()-1, nSearches);

            // Merge in values
            bv.Merge(sbd->Values(), pass);
            if (sbd->IsUleaf()) {
               bvUleaf.Merge(sbd->Values(), pass);
            }
         }
      }
      if (pass==2 || sbd)
         movesNonbook.Delete(move);
   }
}

const std::string& CBook::Bookname() const
{
   return bookname;
}

void CBook::AddData(CBitBoard bitboard, const CBookData& bd)
{
   entries[bitboard.NEmpty()][bitboard.MinimalReflection()] = bd;
}

CBookPtr CBook::ExtractPositions(const VariationCollection& games) const
{
   CBookPtr book(new CBook(0));

   BOOST_FOREACH(const Variation& game, games)
   {
      std::stringstream stream;
      game.OutputGGF(stream);
      COsGame osGame;
      stream >> osGame;
      // only extract the last three positions and their successors
      //const std::size_t startIndex = (std::max)(osGame.ml.size(), 3U) - 3;
      const std::size_t startIndex = 0;

      for( std::size_t iMove=startIndex; iMove<osGame.ml.size(); iMove++ ) {
         CQPosition pos(osGame, iMove);
         // find *all* successors and add to book
         CMove move;
         CMoves moves;
         pos.CalcMoves(moves);
         while( moves.GetNext(move) ) {
            CQPosition subpos = pos;
            subpos.MakeMove(move);
            const CBookData* bd2 = FindAnyReflection(subpos.BitBoard());
            if( bd2 ) {
               book->AddData(subpos.BitBoard(), *bd2);
            }
         }
      }
   }
   return book;
}

std::size_t CBook::Positions() const
{
   std::size_t nSize = 0;
   for( int nEmpties=0; nEmpties<nEmptyBookMax; nEmpties++ )
      nSize+=entries[nEmpties].size();

   return nSize;
}
