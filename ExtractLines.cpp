// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "ExtractLines.h"
#include "Player.h"
#include "Pos2.h"
#include "Variation.h"

int ExtractLines(int argc,
                 char** argv,
                 const char* submode,
                 const CComputerDefaults& cd1,
                 const CComputerDefaults& cd2,
                 int nGames)
{
   int success = EXIT_SUCCESS;

#if defined(_DEBUG)
   CPlayerPtr p0=GetPlayer(submode[0],cd1,cd2, true);
   CPlayerPtr p1=GetPlayer(submode[1],cd1,cd2, true);
#else
   CPlayerPtr p0=GetPlayer(submode[0],cd1,cd2, false);
   CPlayerPtr p1=GetPlayer(submode[1],cd1,cd2, false);
#endif
   CPlayerComputerPtr cp1 = std::dynamic_pointer_cast<CPlayerComputer>(p0);
   if( cp1 ) {
      VariationCollection variations;
      ExtractLines(*cp1->book.lock(), variations, nGames);

      if( argc>4 ) {
         std::ofstream out(argv[4]);
         if( out ) {
            std::cout << "Writing ggf to " << argv[4] << std::endl;
            for( VariationCollection::const_iterator it=variations.begin();
                 it!=variations.end();
                 ++ it )
            {
               it->OutputGGF(out);
               out << "\n";
            }
            if( argc>5 ) {
               std::cout << "Writing variations to " << argv[5] << std::endl;
               WriteVariations(variations, argv[5]);
            }
         }
         else {
            std::cerr << "Could not open " << argv[4] << std::endl;
            success = EXIT_FAILURE;
         }
      }
   }

   return success;
}
