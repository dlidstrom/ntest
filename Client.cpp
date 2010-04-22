// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "Client.h"
#include "Book.h"
#include "Games.h"
#include "Log.h"
#include "Commands.h"
#include "FetchCommand.h"
#include "SyncCommand.h"
#include "Variation.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>

void expandLines(const std::string& lines_file,
                 CPlayerComputerPtr computer,
                 const std::string& server_address,
                 const std::string& server_port)
{
   fPrintCorrections = false;
   int n_games = 0;
   int expanded = 0;
   int linesTotal = 0;
   int linesDone = 0;
   // count lines
   {
      std::ifstream in(lines_file.c_str());
      if( in ) {
         std::ostringstream stream;
         stream << in.rdbuf();
         std::string contents = stream.str();
         linesTotal = std::count_if(contents.begin(), contents.end(), std::bind2nd(std::equal_to<char>(), '\n'));
      }
   }
   std::ifstream in(lines_file.c_str());
   if( !in ) {
      throw std::runtime_error("Could not open " + lines_file);
   }

   Timer<double> timer;
   CGame game(computer, computer);
   while( in >> game && timer.elapsed()<12*3600 ) {
      Log(std::cout)
         << "Analyzing game "
         << ++n_games
         << endl
         ;
      int nBlack;
      int nWhite;
      int nEmpty;
      game.pos.board.GetPieceCounts(nBlack, nWhite, nEmpty);
      int perfectSolve = computer->book_pcp->PerfectSolve();
      // must expand perfectly solved nodes completely
      if( nEmpty<=perfectSolve ) {
         std::stringstream stream;
         stream << game;
         // adjust moves to add temporarily
         int temp = nEmpty;
         std::swap(computer->movesToAdd, temp);
         // adjust to book_pcp temporarily
         std::swap(computer->search_pcp, computer->book_pcp);
         CGame game2(computer, computer);
         stream >> game2;
         Timer<double> timer;
         game2.Play(timer);
         // restore moves to add
         std::swap(temp, computer->movesToAdd);
         // restore to search_pcp
         std::swap(computer->search_pcp, computer->book_pcp);
      }
      else {
         computer->AnalyzeGame(game, timer);
      }
      Log(std::cout)
         << "Expansion pace is "
         << int(timer.elapsed()/n_games+0.5)
         << " seconds per line"
         << std::endl
         ;
   }
   if( computer->cd.sCalcParams=="s26" ) {
      CBookPtr bp = computer->book.lock();
      int searches;
      bp->CorrectAll(searches);
   }
}

void client_fetch(std::iostream& stream, const std::string& lines_file)
{
   Log(std::cout) << "Requesting work." << std::endl;
   stream << FETCH_COMMAND << " " << GetComputerNameAsString() << std::endl;
   int n_lines = 0;
   {
      std::ofstream out(lines_file.c_str());
      std::string line;
      if( std::getline(stream, line) ) {
         n_lines = boost::lexical_cast<int>(line);
         for( int i=0; i<n_lines && std::getline(stream, line); i++ ) {
            out << line << '\n';
         }
      }
   }
   if( n_lines==0 )
      throw std::runtime_error("Received 0 lines, connection was probably broken.");
   Log(std::cout) << "Received " << n_lines << " lines." << std::endl;
}

FetchCommand client_fetch2(std::iostream& stream,
                           CBook& book,
                           const std::string& lines_file)
{
   Log(std::cout) << "Requesting work." << std::endl;
   stream << FETCH2_COMMAND << " " << GetComputerNameAsString() << std::endl;
   std::ofstream out(lines_file.c_str());
   if( !out )
      throw std::runtime_error("Could not open " + lines_file);

   FetchCommand command(book);
   boost::archive::binary_iarchive ia(stream);
   ia >> command;

   foreach(const std::string& game, command.GetGames()) {
      out << game << std::endl;
   }

   Log(std::cout)
      << "Received "
      << command.GetGames().size()
      << " lines and "
      << book.Positions()
      << " positions."
      << std::endl
      ;
   return command;
}

void client_sync(std::iostream& stream, const CBook& book)
{
   stream << SYNC_COMMAND << " " << GetComputerNameAsString() << std::endl;
   boost::archive::binary_oarchive oa(stream);
   Log(std::cout)
      << "Sending book..."
      << std::endl
      ;
   oa << book;
   stream << std::flush;
   Log(std::cout)
      << "Done"
      << std::endl
      ;
}

void client_sync2(std::iostream& stream, const SyncCommand& syncCommand)
{
   stream << SYNC2_COMMAND << " " << GetComputerNameAsString() << std::endl;
   boost::archive::binary_oarchive oa(stream);
   Log(std::cout)
      << "Sending book..."
      << std::endl
      ;
   oa << syncCommand;
   stream << std::flush;
   Log(std::cout)
      << "Done"
      << std::endl
      ;
}

void client_merge(std::iostream& stream, CBook& book)
{
   stream << MERGE_COMMAND << " " << GetComputerNameAsString() << std::endl;
   const std::string joinBook("Coefficients/join.book");
   const std::string oldJoinBook = joinBook + ".old";
   ::remove(oldJoinBook.c_str());
   ::rename(joinBook.c_str(), oldJoinBook.c_str());
   CBook serverBook(joinBook.c_str(), no_save);
   boost::archive::binary_iarchive ia(stream);
   Log(std::cout)
      << "Receiving book..."
      << std::endl
      ;
   ia >> serverBook;
   Log(std::cout)
      << "Done"
      << std::endl
      ;
   Log(std::cout)
      << "Joining books..."
      << std::endl
      ;
   book.JoinBook(serverBook);
   Log(std::cout)
      << "Done"
      << std::endl
      ;
   int searches;
   book.CorrectAll(searches);
}

int client(CComputerDefaults cd1,
           const std::string& server_address,
           const std::string& server_port,
           bool isS26)
{
#if !defined(_DEBUG)
   if( !SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE) )
      Log(std::cout) << "Could not change process priority" << std::endl;
   else
      Log(std::cout) << "Changed process priority to idle" << std::endl;
#endif

   cd1.fEdmundAfter = false;
   CPlayerComputerPtr computer;
   if( isS26 ) { // add all moves
      computer = CPlayerComputer::Create(cd1, false);
   }
   else {
      //const int movesToAdd = 2;
      //computer = CPlayerComputer::Create(cd1, false, movesToAdd);
      computer = CPlayerComputer::Create(cd1, false);
   }
   CBookPtr bp = computer->book.lock();
   if (!bp) {
      cerr << "ERR: You need a book to use Expand mode\n";
      return EXIT_FAILURE;
   }
   std::tr1::shared_ptr<SyncCommand> syncCommand(new SyncCommand(*bp));

   int success = EXIT_SUCCESS;

   enum STATE {
      FETCH,
      FETCH2,
      SYNC,
      //MERGE,
      IDENTITY,
      DEFAULT
   };
   STATE state = FETCH2;
   const std::string lines_file("lines.ggf");
   try {
      expandLines(lines_file, computer, server_address, server_port);
      state = SYNC;//MERGE;
   }
   catch( std::exception& )
   { }

   // drop out after a long time, to enable
   // updates of the exe for example
   Timer<double> totalTimer;

   while( totalTimer.elapsed()<24*3600 ) {
      try {
         Log(std::cout) << "Connecting to " << server_address << ":" << server_port << std::endl;
         {
            using boost::asio::ip::tcp;

            tcp::iostream stream(server_address, server_port);
            if( !stream ) {
               throw std::runtime_error("Could not connect to server.");
            }
            else {
               Log(std::cout) << "Connected." << std::endl;
               switch( state ) {
               case IDENTITY:
               {
                  stream << IDENTITY_COMMAND << ": " << GetComputerNameAsString() << std::endl;
               } break;
               case FETCH:
               {
                  client_fetch(stream, lines_file);
               } break;
               case FETCH2:
               {
                  syncCommand.reset(new SyncCommand(*bp, client_fetch2(stream, *bp, lines_file).GetVariations()));
               } break;
               case SYNC:
               {
                  //client_sync(stream, *bp);
                  client_sync2(stream, *syncCommand);
                  bp->Write();
                  const std::string bookname = bp->Bookname();
                  const std::string oldBookname = bookname+".old";
                  ::remove(oldBookname.c_str());
                  ::rename(bookname.c_str(), oldBookname.c_str());
                  *bp = CBook(bookname.c_str());
                  bp->SetComputer(computer);
               } break;
               default: break;
               }

               stream.close();

               // perform any work and update state
               switch( state ) {
               case FETCH:
               case FETCH2:
               {
                  expandLines(lines_file, computer, server_address, server_port);
                  state = SYNC;
               } break;
               case SYNC:
               {
                  state = FETCH2;
               } break;
               default:
               {
                  state = FETCH2;
               } break;
               }
            }
         }
      }
      catch( std::exception& ex ) {
         std::cerr << ex.what() << std::endl;
#if !defined(_DEBUG)
         // wait here a few seconds, before trying to connect again
         Sleep((5 + rand()%15)*1000);
#endif
      }
   }

   return success;
}

int Client(int argc, char** argv, const CComputerDefaults& cd1)
{
   int success = EXIT_SUCCESS;

   bool isS26 = cd1.sCalcParams=="s26";
   if( argc<7 ) {
      cerr << "No server address specified." << std::endl;
      success = EXIT_FAILURE;
   }
   else {
      std::string server_address(argv[6]);
      if( argc<8 ) {
         cerr << "No server port specified." << std::endl;
         success = EXIT_FAILURE;
      }
      else {
         try {
            std::string server_port = argv[7];
            boost::lexical_cast<int>(server_port);
            success = client(cd1, server_address, server_port, isS26);
         }
         catch( std::bad_cast& )
         {
            cerr << "Invalid server port." << std::endl;
            success = EXIT_FAILURE;
         }
      }
   }

   return success;
}
