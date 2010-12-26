// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// utility stuff

#pragma once

#ifndef _H_UTILS
#define _H_UTILS

#include <iostream>

/////////////////////////////////////////
// Generic types
/////////////////////////////////////////

// for compatibility with unix-like OS
#ifdef __unix__

typedef long long __int64;
#include <unistd.h>

#endif //__unix__


typedef unsigned char u1;
typedef unsigned short u2;
typedef unsigned long u4;
typedef signed char i1;
typedef signed short i2;
typedef signed long i4;
typedef __int64 i8;

inline void CHECKNEW(bool x) {
   if (!x) {
      std::cerr << "New failed in " << __FILE__ << " line " << __LINE__ << "\n";
      _exit(-1);
   }
}

/////////////////////////////////////////
// u8 data type
/////////////////////////////////////////

class u8 {
public:
   union {
      i8 i8s;
      u4 u4s[2];
      u2 u2s[4];
      u1 u1s[8];
   };

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      archive & i8s;
   }

   // access operator
   const u4& operator[](int a)const {return u4s[a];};
   u4& operator[](int a) {return u4s[a];};

   // bit operators
   u8 operator&(const u8& b) const { u8 result; result[0]=u4s[0]&b[0]; result[1]=u4s[1]&b[1]; return result;};
   u8 operator|(const u8& b) const { u8 result; result[0]=u4s[0]|b[0]; result[1]=u4s[1]|b[1]; return result;};
   u8 operator^(const u8& b) const { u8 result; result[0]=u4s[0]^b[0]; result[1]=u4s[1]^b[1]; return result;};
   u8 operator~() const { u8 result; result[0]=~u4s[0]; result[1]=~u4s[1]; return result;};
   u8 operator<<(int n) const {u8 result; result[1]=(u4s[1]<<n)|(u4s[0]>>(32-n)); result[0]=u4s[0]<<n; return result;}
   u8 operator>>(int n) const {u8 result; result[0]=(u4s[0]>>n)|(u4s[1]<<(32-n)); result[1]=u4s[1]>>n; return result;}

   // assignment operators
   u8 operator&=(const u8& b) { u4s[0]&=b[0]; u4s[1]&=b[1]; return *this;};
   u8 operator|=(const u8& b) { u4s[0]|=b[0]; u4s[1]|=b[1]; return *this;};
   u8 operator^=(const u8& b) { u4s[0]^=b[0]; u4s[1]^=b[1]; return *this;};
   u8 operator=(const u8& b) { u4s[0] =b[0]; u4s[1] =b[1]; return *this;};
   u8 operator<<=(int n) {u4s[1]=(u4s[1]<<n)|(u4s[0]>>(32-n)); u4s[0]<<=n; return *this;}
   u8 operator>>=(int n) {u4s[0]=(u4s[0]>>n)|(u4s[1]<<(32-n)); u4s[1]>>=n; return *this;}

   // comparison operators
   bool operator==(const u8& b) const { return u4s[0]==b[0] && u4s[1]==b[1];}
   bool operator!=(const u8& b) const { return !(*this==b);}
   bool operator<(const u8& b) const { return (u4s[1]==b[1])?(u4s[0]<b[0]):(u4s[1]<b[1]);};
   bool operator>(const u8& b) const { return (u4s[0]==b[0])?(u4s[1]>b[1]):(u4s[0]>b[0]);};

   // other operators
   bool operator!() const { return u4s[0]==0 && u4s[1]==0; };

   void SetBit(int n)		{ u4s[n>>5]|=(1<<(n&31)); }
   void FlipBit(int n)		{ u4s[n>>5]^=(1<<(n&31)); }
   void ClearBit(int n)	{ u4s[n>>5]&=~(1<<(n&31)); }

   int GetBit(int n) const { return u4s[n>>5]&(1<<(n&31)); }	// returns nonzero if bit is set, but not guaranteed to be 1
};

/////////////////////////////////////////
// Min and Max
/////////////////////////////////////////

inline i2 Max(i2 a, i2 b) {return a>b?a:b;};
inline i2 Min(i2 a, i2 b) {return a<b?a:b;};
int Sign(int a);

/////////////////////////////////////////
// node value type
/////////////////////////////////////////

typedef i2 CValue;

//////////////////////////////////////
// Constants
//////////////////////////////////////

enum direction { north, south, east, west, northeast,southwest, northwest, southeast };

const CValue kStoneValue = 100;
const CValue kWipeout = 64*kStoneValue;
const CValue kMaxHeuristic = kWipeout-1;
const CValue kMaxBonus = 10000;
const CValue kInfinity = kWipeout+kMaxBonus;

const int N=8;
const int NN=N*N;

//////////////////////////////////////////
// bit routines
//////////////////////////////////////////

u2 CountBits(u4 a);
u2 CountBits(u8 a);

const u4 cbmask1=0x55555555;
const u4 cbmask2=0x33333333;
const u4 cbmask4=0x0F0F0F0F;

inline u2 CountBitsInline(u4 a)  {
   a=(a&cbmask1)+((a>>1)&cbmask1);
   a=(a&cbmask2)+((a>>2)&cbmask2);
   a=(a&0xFFFF)+(a>>16);
   a=(a&cbmask4)+((a>>4)&cbmask4);
   a=(a&0xFF)+(a>>8);
   return (u2)a;
}

inline u2 CountBitsInline(u8 a) {
   u4 count;

   a[0]=(a[0]&cbmask1)+((a[0]>>1)&cbmask1);
   a[0]=(a[0]&cbmask2)+((a[0]>>2)&cbmask2);
   a[1]=(a[1]&cbmask1)+((a[1]>>1)&cbmask1);
   a[1]=(a[1]&cbmask2)+((a[1]>>2)&cbmask2);
   count=a[0]+a[1];
   count=(count&cbmask4)+((count>>4)&cbmask4);
   count=(count&0xFFFF)+(count>>16);
   count=(count&0xFF)+(count>>8);

   return (u2)count;
}

//////////////////////////////////////////
// hash functions
//////////////////////////////////////////

// bob jenkins hash.
// usage example with four u4s of data:
//	a=b=0; c=data[0];
//	bobMix(a,b,c);
//	a+=data[1]; b+=data[2]; c+=data[3];
//	bobMix(a,b,c);
//	return c;

inline void bobMix(u4& a, u4& b, u4& c) {
   a-=b; a-=c; a^= (c>>13);
   b-=c; b-=a; b^= (a<<8);
   c-=a; c-=b; c^= (b>>13);
   a-=b; a-=c; a^= (c>>12);
   b-=c; b-=a; b^= (a<<16);
   c-=a; c-=b; c^= (b>>5);
   a-=b; a-=c; a^= (c>>3);
   b-=c; b-=a; b^= (a<<10);
   c-=a; c-=b; c^= (b>>15);
};

// bob jenkins older hash, 'lookup'. May not be as great but it works on 4 bytes at a time
inline void bobLookup(u4& a, u4& b, u4& c, u4& d) {
   a+=d; d+=a; a^=(a>>7);
   b+=a; a+=b; b^=(b<<13);
   c+=b; b+=c; c^=(c>>17);
   d+=c; c+=d; d^=(d<<9);
   a+=d; d+=a; a^=(a>>3);
   b+=a; a+=b; b^=(b<<7);
   c+=b; b+=c; c^=(c>>15);
   d+=c; c+=d; d^=(d<<11);
};

inline u4 Hash4(u4 a, u4 b, u4 c, u4 d){
   bobLookup(a,b,c,d);
   return d;
}

///////////////////////////////////////////
// Evaluator functions
///////////////////////////////////////////
class CPosition;

typedef CValue Evaluator(const CPosition& pos);

///////////////////////////////////////////
// Coefficient type
///////////////////////////////////////////

typedef i4 TCoeff;


// square values
const int SIGNED_CONFIGS=0;
const int WHITE=0-SIGNED_CONFIGS;
const int EMPTY=1-SIGNED_CONFIGS;
const int BLACK=2-SIGNED_CONFIGS;

// convert from various board characters to (-1, 0, 1) or (0,1,2) depending on SIGNED_CONFIGS and back again
int  TextToValue(char c);
char ValueToText(int item);

// convert from square number to row and column
inline int Row(int square) {
   if (N==8)
      return square>>3;
   else
      return square/N;
}

inline int Col(int square) {
   if (N==8)
      return square&7;
   else
      return square%N;
}

inline char RowText(int square) {
   return Row(square)+'1';
}

inline char ColText(int square) {
   return Col(square)+'A';
}

inline int Square(int row, int col) {
   if (N==8)
      return (row<<3)+col;
   else
      return row*N+col;
}

inline int Square(char text[2]) {
   return Square(text[1]-'1',text[0]-'A');
}

////////////////////////////////////////////////////
// HeightInfo - used in books and iterative value
////////////////////////////////////////////////////

class CSearchInfo;

class CHeightInfo {
public:
   int height;
   int iPrune;
   bool fWLD;

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      archive & height;
      archive & iPrune;
      archive & fWLD;
      archive & fKnownSolve;
   }

   CHeightInfo();
   CHeightInfo(int aHeight, int aiPrune, bool afWLD);
   CHeightInfo(int aHeight, int aiPrune, bool afWLD, int nEmpty);

   void Clear();
   bool NextRound(int nEmpty, const CSearchInfo& si);

   bool operator<(const CHeightInfo& b) const;
   bool operator<=(const CHeightInfo& b) const;
   bool operator>(const CHeightInfo& b) const;
   bool operator>=(const CHeightInfo& b) const;
   bool operator!=(const CHeightInfo& b) const;
   bool operator==(const CHeightInfo& b) const;

   bool WLDSolved(int nEmpty) const;
   bool ExactSolved(int nEmpty) const;

   bool Valid() const { return iPrune==4 || fKnownSolve; };

   void SetNEmpty(int nEmpty);

   CHeightInfo operator-(int n) const;
   CHeightInfo operator+(int n) const;

   void Out(std::ostream& os) const;
protected:
   bool fKnownSolve;
   i4 Order() const;
};
std::ostream& operator<<(std::ostream& os, const CHeightInfo& hi);

std::string format_time(int seconds);
std::string GetComputerNameAsString();

#endif // defined(_H_UTILS)
