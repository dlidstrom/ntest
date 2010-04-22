// $Id$
// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.
//! @file   SyncCommand.h
//! Brief.
//! @author Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date   2010-04-06 11:13
//! @ingroup
//!

#if !defined(SYNCCOMMAND_H__20100406T1113)
#define SYNCCOMMAND_H__20100406T1113

#include "Fwd.h"
#include "Variation.h"
#include <boost/serialization/vector.hpp>
#include <vector>

//!
//! @author  Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date    2010-04-06 11:13
//! @ingroup
//! Brief.
//!
class SyncCommand
{
public:

   SyncCommand(CBook& book);

   //!
   //! Constructor.
   //!
   SyncCommand(CBook& book, const VariationCollection& variations);

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      archive & book;
      archive & variations;
   }

   const VariationCollection& GetVariations() const;

private:

   CBook& book;
   VariationCollection variations;
};

#endif
