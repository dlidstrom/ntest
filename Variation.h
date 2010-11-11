// $Id$
// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.
//! @file   Variation.h
//! Brief.
//! @author Daniel Lidstrom <dlidstrom@gmail.com>
//! @date   2010-04-06 11:23
//! @ingroup
//!

#if !defined(VARIATION_H__20100406T1123)
#define VARIATION_H__20100406T1123

#include "Fwd.h"
#include <vector>
#include <iosfwd>

//!
//! @author  Daniel Lidstrom <dlidstrom@gmail.com>
//! @date    2010-04-06 11:23
//! @ingroup
//! Brief.
//!
struct Variation
{
   Variation();

   Variation(int delta, int bestScore, const std::vector<CMove>& moves);

   void PlayMoves(CGame& game) const;

   bool operator<(const Variation& right) const;
   bool operator==(const Variation& right) const;
   friend std::ostream& operator<<(std::ostream&, const Variation&);
   void OutputGGF(std::ostream&) const;

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version);

   bool IsDraw() const;

   std::size_t Length() const;

   struct Impl;
   std::shared_ptr<Impl> mImpl;
};

void WriteVariations(const VariationCollection& variations, const char* filename);

#endif
