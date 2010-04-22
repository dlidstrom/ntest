
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
