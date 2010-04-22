
#include "PreCompile.h"
#include "ExtractDraws.h"
#include "Player.h"
#include "Pos2.h"
#include "Variation.h"
#include <boost/foreach.hpp>

int ExtractDraws(int argc,
                 char** argv,
                 const char* submode,
                 const CComputerDefaults& cd1,
                 const CComputerDefaults& cd2)
{
   int success = EXIT_SUCCESS;

#if defined(_DEBUG)
   CPlayerPtr p0=GetPlayer(submode[0],cd1,cd2, true);
#else
   CPlayerPtr p0=GetPlayer(submode[0],cd1,cd2, false);
#endif
   CPlayerComputerPtr cp1 = std::tr1::dynamic_pointer_cast<CPlayerComputer>(p0);
   if( cp1 ) {
      VariationCollection variations = ExtractDraws(*cp1->book.lock());

      if( argc>2 ) {
         std::string filename(argv[4]);
         std::ofstream out(filename.c_str());
         if( out ) {
            std::cout << "Writing ggf to " << filename << std::endl;
            foreach(const Variation& variation, variations) {
               variation.OutputGGF(out);
               out << "\n";
            }
         }
         else {
            std::cerr << "Could not open " << filename << std::endl;
            success = EXIT_FAILURE;
         }
      }
   }

   return success;
}
