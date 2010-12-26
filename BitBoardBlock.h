// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// BitBoard header file

#pragma once

#include "Utils.h"

//////////////////////////////////////
// CBitBoardBlock class
//////////////////////////////////////

class CBitBoardBlock : public u8 {
public:
   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      u8::serialize(archive, version);
   }

   CBitBoardBlock() {};
   CBitBoardBlock(const u8& b) { u4s[0]=b[0]; u4s[1]=b[1];};
   void Clear();
   CBitBoardBlock Symmetry(int sym);

   void FlipHorizontal();
   void FlipVertical();
   void FlipDiagonal();

private:
   void FlipHorizontal32(u4& rows);
   void FlipDiagonal32(u4& rows);
};

inline void CBitBoardBlock::Clear() {u4s[0]=u4s[1]=0;};
