// $Id$
// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.
//! @file
//! Brief.
//! @author Daniel Lidstrom <dlidstrom@gmail.com>
//! @date   2010-05-03 09:25
//! @ingroup
//!

#if !defined(SYNCCOMMAND2_H__20100503T0925)
#define SYNCCOMMAND2_H__20100503T0925

#include "Fwd.h"
#include "Variation.h"
#include <boost/serialization/vector.hpp>
#include <vector>

//!
//! @author  Daniel Lidstrom <dlidstrom@gmail.com>
//! @date    2010-05-03 09:25
//! @ingroup
//! Brief.
//!
class SyncCommand2
{
public:

   SyncCommand2(CBook& book);

   //!
   //! Constructor.
   //!
   SyncCommand2(CBook& book, const VariationCollection& variations);

   template<class Archive>
   void save(Archive& archive, const unsigned int version) const
   {
      archive & *book.GetAddedOrUpdated();
      archive & variations;
   }

   template<class Archive>
   void load(Archive& archive, const unsigned int version)
   {
      archive & book;
      archive & variations;
   }

   const VariationCollection& GetVariations() const;

   BOOST_SERIALIZATION_SPLIT_MEMBER();

private:

   CBook& book;
   VariationCollection variations;
};

#endif
