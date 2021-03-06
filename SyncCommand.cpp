// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "SyncCommand.h"

SyncCommand::SyncCommand(CBook& book)
   : book(book)
{}

SyncCommand::SyncCommand(CBook& book, const VariationCollection& variations)
   : book(book)
   , variations(variations)
{ }

const VariationCollection& SyncCommand::GetVariations() const
{
   return variations;
}
