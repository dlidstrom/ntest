
#include "PreCompile.h"
#include "Variation.h"
#include "Utils.h"
#include "QPosition.h"
#include "Games.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>

struct Variation::Impl
{
   Impl()
   {}

   Impl(int delta, int bestScore, const std::vector<CMove>& moves)
      : delta(delta),
        bestScore(bestScore),
        moves(moves)
   {}

   template<class Archive>
   void serialize(Archive& archive, const unsigned int version)
   {
      archive & delta;
      archive & bestScore;
      archive & moves;
   }

   int delta;
   int bestScore;
   std::vector<CMove> moves;
};

template<class Archive>
void Variation::serialize(Archive& archive, const unsigned int version)
{
   mImpl->serialize(archive, version);
}

template
void Variation::serialize(boost::archive::binary_iarchive& archive, const unsigned int version);
template
void Variation::serialize(boost::archive::binary_oarchive& archive, const unsigned int version);

Variation::Variation()
   : mImpl(new Impl)
{}

Variation::Variation(int delta, int bestScore, const std::vector<CMove>& moves)
   : mImpl(new Impl(delta, bestScore, moves))
{}

void Variation::PlayMoves(CGame& game) const
{
   char sBoard[NN+1];
   CQPosition pos;
   pos.Initialize();
   game.Initialize(pos.GetSBoard(sBoard), true);
   for( std::size_t i=0; i<mImpl->moves.size(); i++ ) {
      CMove move = mImpl->moves[i];
      CSGMoveListItem mli;
      mli.mv = CSGSquare(move.Col(), move.Row());
      mli.dEval = 0;//mv.value/(double)kStoneValue;
      mli.tElapsed = 0;
      if( !game.pos.board.IsMoveLegal(mli.mv) ) {
         std::copy(mImpl->moves.begin(), mImpl->moves.end(), std::ostream_iterator<CMove>(std::cout, " "));
         std::cout << std::endl;
         std::cin.get();
      }
      game.Update(mli);
   }
}

bool Variation::operator <(const Variation& right) const
{
   int sc1 = mImpl->delta;
   int sc2 = right.mImpl->delta;

   int lengthFactor1 = 5;
   int lengthFactor2 = 5;
   // draws need to be prioritized
   if( sc1==0 ) {
      // adjust other
      lengthFactor2 = 1000;
   }
   if( sc2==0 ) {
      // adjust other
      lengthFactor1 = 1000;
   }

   int firstScore = sc1 + lengthFactor1*mImpl->moves.size();
   int secondScore = sc2 + lengthFactor2*right.mImpl->moves.size();
   return firstScore<secondScore || (firstScore==secondScore && mImpl->moves<right.mImpl->moves);
}

bool Variation::operator==(const Variation& right) const
{
   return mImpl->moves == right.mImpl->moves;
}

void Variation::OutputGGF(std::ostream& out) const
{
   out << "(;GM[Reversi]PC[Local]DT[2009-10-28 12:37:22 GMT]PB[s20]PW[s20]RE[?]TI[0:00//2:00]TY[8]BO[8 ---------------------------O*------*O--------------------------- *";
   // "B[D3]W[C3]"
   for( std::size_t i=0; i<mImpl->moves.size(); i+=2 ) {
      out << "]B[" << mImpl->moves[i];
      if( i<mImpl->moves.size()-1 )
         out << "]W[" << mImpl->moves[i+1];
   }
   out << "];)";
}

bool Variation::IsDraw() const
{
   return mImpl->delta==0 && mImpl->bestScore == 0;
}

std::ostream& operator<<(std::ostream& out, const Variation& v)
{
   out << std::setw(4) << v.mImpl->delta << "/" << std::setw(4) << v.mImpl->bestScore;
   for( std::size_t i=0; i<v.mImpl->moves.size(); i++ )
      out << ' ' << v.mImpl->moves[i];
   return out;
}

void WriteVariations(const VariationCollection& variations, const char* filename)
{
   std::ofstream out(filename);
   if( out ) {
      std::copy(variations.begin(),
                variations.end(),
                std::ostream_iterator<Variation>(out, "\n"));
   }
   else
      std::cerr << "Could not open " << filename << std::endl;
}
