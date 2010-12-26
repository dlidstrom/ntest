// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "FetchCommand.h"
#include "Pos2.h"
#include "Variation.h"

FetchCommand::FetchCommand(CBook& book)
   : book(book)
{}

FetchCommand::FetchCommand(CBook& book, const VariationCollection& variations)
   : book(book)
   , variations(variations)
{
   foreach(const Variation& variation, variations) {
      std::ostringstream stream;
      variation.OutputGGF(stream);
      games.push_back(stream.str());
   }
}

const std::vector<std::string>& FetchCommand::GetGames() const
{
   return games;
}

const VariationCollection& FetchCommand::GetVariations() const
{
   return variations;
}
