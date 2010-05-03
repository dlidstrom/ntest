// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "SyncCommand2.h"

SyncCommand2::SyncCommand2(CBook& book)
   : book(book)
{}

SyncCommand2::SyncCommand2(CBook& book, const VariationCollection& variations)
   : book(book)
   , variations(variations)
{ }

const VariationCollection& SyncCommand2::GetVariations() const
{
   return variations;
}
