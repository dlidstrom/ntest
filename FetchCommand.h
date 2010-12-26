// $Id$
// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.
//! @file
//! Brief.
//! @author Daniel Lidstrom <dlidstrom@gmail.com>
//! @date   2009-12-22 14:59
//! @ingroup
//!

#if !defined(FETCHCOMMAND_H__20091222T1459)
#define FETCHCOMMAND_H__20091222T1459

#include "Fwd.h"

#include <boost/serialization/vector.hpp>
#include <vector>

//!
//! @author  Daniel Lidstrom <dlidstrom@gmail.com>
//! @date    2009-12-22 14:59
//! @ingroup
//! Brief.
//!
class FetchCommand
{
public:

   FetchCommand(CBook& book);

   //!
   //! Constructor.
   //!
   FetchCommand(CBook& book, const VariationCollection& variations);

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      archive & book;
      archive & games;
      archive & variations;
   }

   const std::vector<std::string>& GetGames() const;

   const VariationCollection& GetVariations() const;

private:

   CBook& book;
   std::vector<std::string> games;
   VariationCollection variations;
};

#endif
