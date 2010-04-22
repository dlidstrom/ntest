
#include "PreCompile.h"
#include "FetchCommand.h"
#include "Pos2.h"
#include "Variation.h"

#include <boost/foreach.hpp>

FetchCommand::FetchCommand(CBook& book)
   : book(book)
{}

FetchCommand::FetchCommand(CBook& book, const VariationCollection& variations)
   : book(book)
   , variations(variations)
{
   BOOST_FOREACH(const Variation& variation, variations)
   {
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
