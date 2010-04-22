// $Id$
//! @file   Variation.h
//! Brief.
//! @author Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date   2010-04-06 11:23
//! @ingroup
//!

#if !defined(VARIATION_H__20100406T1123)
#define VARIATION_H__20100406T1123

#include "Fwd.h"
#include <vector>
#include <iosfwd>

//!
//! @author  Daniel Lidstrom <daniel.lidstrom@sbg.se>
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

   struct Impl;
   std::tr1::shared_ptr<Impl> mImpl;
};

void WriteVariations(const VariationCollection& variations, const char* filename);

#endif
