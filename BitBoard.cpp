// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// board source code

#include "PreCompile.h"
#include "BitBoard.h"
#include "Moves.h"
#include "Patterns.h"
#include "Debug.h"
#include "bbm.h"

///////////////////////////////////////////
// CBitBoard class
///////////////////////////////////////////

void CBitBoard::Initialize(const char* boardText, bool fBlackMove) {
   int i;
   u4 mask, half;

   QSSERT(strlen(boardText)==NN);

   empty.Clear();
   mover.Clear();

   for (i=0; i<NN; i++) {
      mask=1<<(i&31);
      half=i>>5;
      switch(boardText[i]) {
      case '.':
      case '-':
      case '_':
         empty[half]|=mask;
         break;
      case 'x':
      case 'X':
      case '*':
      case 'b':
      case 'B':
         mover[half]|=mask;
         break;
      case 'o':
      case 'O':
      case 'w':
      case 'W':
         break;
      default:
         QSSERT(0);
      }
   }

   // if white to move, swap around so mover is to move
   if (!fBlackMove) {
      InvertColors();
   }
}

bool operator<(const CBitBoard& a, const CBitBoard& b) {
   if (a.mover==b.mover)
      return a.empty<b.empty;
   else
      return a.mover<b.mover;
}

std::size_t hash_value(const CBitBoard& b)
{
   return b.Hash();
}

bool CBitBoard::Write(FILE* fp) const {
   if (!fwrite(&mover,sizeof(mover),1,fp)  || !fwrite(&empty,sizeof(empty),1,fp)) {
      printf("Error writing bitboard to file\n");
      return false;
   }
   else
      return true;
}

bool CBitBoard::Read(FILE* fp) {
   return fread(&mover,sizeof(mover),1,fp)  && fread(&empty,sizeof(empty),1,fp);
}

u4 CBitBoard::Hash() const {
   u4 a,b,c,d;
   a=empty.u4s[0];
   b=empty.u4s[1];
   c=mover.u4s[0];
   d=mover.u4s[1];
   bobLookup(a,b,c,d);
   return d;
}

CBitBoard CBitBoard::MinimalReflection() const {
   CBitBoard result, temp;
   int i,j,k;

   temp=result=*this;
   for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
         for (k=0; k<2; k++) {
            if (temp<result)
               result=temp;
            temp.FlipVertical();
         }
         temp.FlipHorizontal();
      }
      temp.FlipDiagonal();
   }
   return result;
}

void CBitBoard::Print(bool fBlackMove) const {
   FPrint(stdout, fBlackMove);
}

void CBitBoard::FPrintHeader(FILE* fp) const {
   int col;

   fprintf(fp,"  ");
   for (col=0; col<N; col++)
      fprintf(fp, " %c", 'A'+col);
   fprintf(fp, "\n");
}

void CBitBoard::FPrint(FILE* fp, bool fBlackMove) const {
   int row,col;
   u2 e,b;
   int value;

   fprintf(fp, "\n");

   // print the board
   FPrintHeader(fp);
   for (row=0; row<N; row++) {
      fprintf(fp, "%2d ",row+1);
      e=empty.u1s[row];
      b=mover.u1s[row];
      if (!fBlackMove)
         b^=~e;
      for (col=0; col<N; col++) {
         if (e&1)
            value=EMPTY;
         else
            value=(b&1)?BLACK:WHITE;
         e>>=1;
         b>>=1;
         fprintf(fp, "%c ",ValueToText(value));
      }
      fprintf(fp, "%d\n",row+1);
   }
   FPrintHeader(fp);

   // disc info
   int nEmpty, nBlack, nWhite;
   NDiscs(fBlackMove, nBlack, nWhite, nEmpty);
   fprintf(fp,"Black: %d  White: %d  Empty: %d\n", nBlack, nWhite, nEmpty);

#ifdef PUT_THIS_BACK_IN
   // mobility info
   u4 nMovesBlack, nMovesWhite;
   if (fBlackMove)
      CalcMobility(nMovesBlack, nMovesWhite);
   else
      CalcMobility(nMovesWhite, nMovesBlack);
   fprintf(fp, "Black has %d moves, White has %d moves\n",nMovesBlack, nMovesWhite);
#endif

}

// return the output board string. Assume the bitboard is not normalized with mover to move
char* CBitBoard::GetSBoardNonnormalized(char sBoard[NN+1]) const {
   int row, col, e, b, square;
   char c;

   square=0;
   for (row=0;row<N; row++) {
      e=empty.u1s[row];
      b=mover.u1s[row];
      for (col=0; col<N; col++) {
         if (e&1)
            c=ValueToText(EMPTY);
         else if (b&1)
            c=ValueToText(BLACK);
         else
            c=ValueToText(WHITE);
         sBoard[square++]=c;
         e>>=1;
         b>>=1;
      }
   }
   sBoard[square]=0;
   return sBoard;
}

// return the output board string. Assume the bitboard is normalized with mover to move
char* CBitBoard::GetSBoardNormalized(char sBoard[NN+1], bool fBlackMove) const {
   int row, col, e, b, square;
   char c;

   square=0;
   for (row=0;row<N; row++) {
      e=empty.u1s[row];
      b=mover.u1s[row];
      if (!fBlackMove)
         b^=~e;
      for (col=0; col<N; col++) {
         if (e&1)
            c=ValueToText(EMPTY);
         else if (b&1)
            c=ValueToText(BLACK);
         else
            c=ValueToText(WHITE);
         sBoard[square++]=c;
         e>>=1;
         b>>=1;
      }
   }
   sBoard[square]=0;
   return sBoard;
}

// set a row of the board based on the text in sRow
void CBitBoard::SetRow(const char* sRow, int row) {
   int col, e, b;

   e=b=0;
   for (col=0; col<N; col++) {
      e<<=1;
      b<<=1;
      switch(TextToValue(*(sRow++))) {
      case BLACK:
         b+=1;
         break;
      case EMPTY:
         e+=1;
         break;
      }
      empty.u1s[row]=e;
      mover.u1s[row]=b;
   }
}

int CBitBoard::NEmpty() const {
   return CountBits(empty);
}

void CBitBoard::NDiscs(bool fBlackMove, int& nBlack, int& nWhite, int& nEmpty) const {
   nEmpty=CountBits(empty);
   if (fBlackMove) {
      nBlack=CountBits(mover);
      nWhite=NN-nBlack-nEmpty;
   }
   else {
      nWhite=CountBits(mover);
      nBlack=NN-nWhite-nEmpty;
   }
}

int CBitBoard::TerminalValue() const {
   int nEmpty=CountBits(empty);
   int nMover=CountBits(mover);
   return kStoneValue*(nMover+nMover-NN+nEmpty);
}

// CalcMoves - return true if a move is available
// internal info:
//	a - have seen a mover square and no empty square since
//	b - a and last square was white
//	c - color of this row
//	e - empty of this row

// use dima's assembly language routines for mobility?
#define DIMA 1
const __int64 nega_one=0xffffffffffffffff;
#define CHECK 0

bool CBitBoard::CalcMoves(CMoves& moves) const {
#if (CHECK || !DIMA)
   int nrow, srow;
   u4 ae, aw, a;
   u4 blackRowMoves, c, e, be, bw, b;
   CBitBoardBlock blackMoves;
   u4 blackPattern;

   // east and west flips
   for (nrow=0; nrow<8; nrow++) {
      QSSERT(rowMovesTableSize==6561);
      blackPattern=Base2ToBase3(mover.u1s[nrow], empty.u1s[nrow]);
      blackMoves.u1s[nrow]=rowAllMovesTable[blackPattern];
   }

   // other moves
   // high byte is N moves, low byte is S moves.
   // high word is mover, low word is white
   // a, b = a and b in north and south directions
   //	ae, be = a and b in northeast and southeast directions
   //	aw, bw = a and b in northwest and southwest directions
   a=b=ae=be=aw=bw=0;
   for (nrow=0, srow=7; srow>=0; nrow++, srow--) {
      e = (empty.u1s[nrow]<<8)|empty.u1s[srow];
      c = (mover.u1s[nrow]<<8)|mover.u1s[srow];
      blackRowMoves = (be|b|bw)&e;
      blackMoves.u1s[nrow]|=blackRowMoves>>8;
      blackMoves.u1s[srow]|=blackRowMoves&0xFF;

      ae=(~e)&(ae|c);
      be=ae&~c;
      ae=(ae>>1)&0x7F7F7F7F;
      be=(be>>1)&0x7F7F7F7F;

      aw=(~e)&(aw|c);
      bw=aw&~c;
      aw=(aw<<1)&0xFEFEFEFE;
      bw=(bw<<1)&0xFEFEFEFE;

      a=(~e)&(a|c);
      b=a&~c;
   }
   moves.Set(blackMoves);

#endif // (CHECK || !DIMA)

#if (CHECK || DIMA)
   const i8 moverBits=mover.i8s;
   const i8 emptyBits=empty.i8s;
   // calculate opponent bits
   i8 opponentBits;
   i8 movesi8;
   __asm {
      movq	mm1,	moverBits;		// my bits
      por		mm1,	emptyBits;		// empties
      pxor	mm1,	nega_one;		// opponent bits
      movq	 opponentBits, mm1;
   }
   DimaMoves(moverBits,opponentBits);
   __asm {
      movq movesi8, mm0;
      emms;
   }
#if CHECK
   _ASSERT(movesi8==blackMoves.i8s);
#else
   moves.Set(movesi8);
   return moves.HasMoves();
#endif // CHECK
#endif // (CHECK || DIMA)

#if (CHECK || !DIMA)
   // calc pass code
   return (blackMoves.u4s[0] || blackMoves.u4s[1]);
#endif
}


void CBitBoard::Initialize() {
   mover[0]=0x10000000;
   mover[1]=0x00000008;
   empty[0]=0xE7FFFFFF;
   empty[1]=0xFFFFFFE7;
}

// simultaneously calculate the mobility of board.mover and white
// ignore X-squares when the corner is unoccupied, unless fCountXMobs is true
// return the pass code: 0 = mover has a move, 1=mover has no move but opponent does, 2=no moves
int CBitBoard::CalcMobility(u4& nMovesPlayer, u4& nMovesOpponent) const {
   int pass;
   if (DIMA) {
      const i8 moverBits=mover.i8s;
      const i8 emptyBits=empty.i8s;
      // calculate opponent bits
      i8 opponentBits;
      __asm {
         movq	mm1,	moverBits;		// my bits
         por		mm1,	emptyBits;		// empties
         pxor	mm1,	nega_one;		// opponent bits
         movq	 opponentBits, mm1;
      }
      nMovesPlayer = DimaMobility(moverBits,opponentBits);
      nMovesOpponent=DimaMobility(opponentBits,moverBits);
      __asm emms;
      if (nMovesPlayer)
         pass = 0;
      else if (nMovesOpponent)
         pass = 1;
      else
         pass = 2;
#if (!CHECK)
      return pass;
#endif
   }
#if CHECK
   int passDima=pass;
   int nmpDima=nMovesPlayer;
   int nmoDima=nMovesOpponent;
#endif

#if (CHECK || !DIMA)
   CMoves moves;

   // old calcmobility function
   int nrow, srow;
   u4 ae, aw, a, be, bw, b, c, e;
   u2 blackRowMoves, whiteRowMoves, c2, e2;
   u8 whiteMoves, blackMoves;
   u2 blackPattern, whitePattern;

   // east and west flips
   for (nrow=0; nrow<8; nrow++) {
      QSSERT(rowMovesTableSize==6561);
      blackPattern=Base2ToBase3(mover.u1s[nrow], empty.u1s[nrow]);
      whitePattern=Base2ToBase3(mover.u1s[nrow]^~empty.u1s[nrow], empty.u1s[nrow]);
      whiteMoves.u1s[nrow]=rowAllMovesTable[whitePattern];
      blackMoves.u1s[nrow]=rowAllMovesTable[blackPattern];
   }

   // other moves
   // high byte is N moves, low byte is S moves.
   // high word is mover, low word is white
   a=b=ae=be=aw=bw=0;
   for (nrow=0, srow=7; srow>=0; nrow++, srow--) {
      e2 = (empty.u1s[nrow]<<8)|empty.u1s[srow];
      c2 = (mover.u1s[nrow]<<8)|mover.u1s[srow];
      c = ((u4)c2<<16) | (c2^(u2)(~e2));
      e = ((u4)e2<<16) | e2;
      blackRowMoves = ((be|b|bw)&e)>>16;
      whiteRowMoves = ((be|b|bw)&e)&0xFFFF;
      whiteMoves.u1s[nrow]|=whiteRowMoves>>8;
      whiteMoves.u1s[srow]|=whiteRowMoves&0xFF;
      blackMoves.u1s[nrow]|=blackRowMoves>>8;
      blackMoves.u1s[srow]|=blackRowMoves&0xFF;

      ae=(~e)&(ae|c);
      be=ae&~c;
      ae=(ae>>1)&0x7F7F7F7F;
      be=(be>>1)&0x7F7F7F7F;

      aw=(~e)&(aw|c);
      bw=aw&~c;
      aw=(aw<<1)&0xFEFEFEFE;
      bw=(bw<<1)&0xFEFEFEFE;

      a=(~e)&(a|c);
      b=a&~c;
   }

   // calc pass code
   if (blackMoves.u4s[0] || blackMoves.u4s[1])
      pass=0;
   else if (whiteMoves.u4s[0] || whiteMoves.u4s[1])
      pass=1;
   else
      pass=2;

   moves.Set(blackMoves);


   // add 'em up
   nMovesPlayer=CountBits(blackMoves);
   nMovesOpponent=CountBits(whiteMoves);

#if CHECK
   if	((nmpDima!=nMovesPlayer) ||
         (nmoDima!=nMovesOpponent) ||
         (passDima!=pass)) {
      Print(true);
      _ASSERT(0);
   }
#endif

#endif // (CHECK || !DIMA)
   return pass;
}

static void TestGetRemainingMoves() {
   CBitBoard bb;
   CMoves moves;

   bb.Initialize("................................................................",true);
   int passx=bb.CalcMoves(moves);
   CBitBoardBlock bbb=moves.GetRemainingMoves();
   QSSERT(bbb.u4s[0]==0);
   QSSERT(bbb.u4s[1]==0);
}

static void TestCalcMobility(const char* board, bool fBlackMove, int pass, int nMovesPlayer, int nMovesOpponent) {
   CBitBoard bb;
   CMoves moves;
   u4 nMovesPlayerx, nMovesOpponentx;

   bb.Initialize(board,fBlackMove);
#ifdef OLD
   int passx=bb.CalcMobility(nMovesPlayerx, nMovesOpponentx, moves);
   QSSERT(pass==passx);
   QSSERT(nMovesPlayer==nMovesPlayerx);
   QSSERT(nMovesOpponent==nMovesOpponentx);
#endif
   int passx=bb.CalcMobility(nMovesPlayerx, nMovesOpponentx);
   QSSERT(pass==passx);
   QSSERT(nMovesPlayer==nMovesPlayerx);
   QSSERT(nMovesOpponent==nMovesOpponentx);
}

static void TestCalcMobility() {
   TestCalcMobility("................................................................",true,2,0,0);
   TestCalcMobility("*O..............................................................",true,0,1,0);
   TestCalcMobility("*O..............................................................",false,1,0,1);
   TestCalcMobility("..........................*OO*.....OOO..........................",false,0,5,3);
}

void CBitBoard::Test() {
   TestCalcMobility();
}
