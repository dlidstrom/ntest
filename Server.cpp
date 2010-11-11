// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "Server.h"
#include "Variation.h"
#include "Pos2.h"
#include "Log.h"
#include "FetchCommand.h"
#include "SyncCommand.h"
#include "SyncCommand2.h"
#include "Commands.h"
#include "Book.h"
#include "Player.h"

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <functional>

struct is_drawn : public std::unary_function<Variation, bool>
{
   bool operator()(const Variation& variation) const
   {
      return variation.IsDraw();
   }
};

struct is_in
{
   const VariationCollection& variations;

   is_in(const VariationCollection& variations)
      : variations(variations)
   {}

   bool operator()(const Variation& variation) const
   {
      return std::find(variations.begin(),
                       variations.end(),
                       variation)!=variations.end();
   }
};

std::pair<double, double> StandardDeviation(VariationCollection::const_iterator begin, VariationCollection::const_iterator end)
{
   double sum = 0;
   std::for_each(begin, end, [&sum](const Variation& variation) {
      sum += variation.Length();
   });
   double mean = sum / std::distance(begin, end);

   double variance = 0;
   std::for_each(begin, end, [mean,&variance](const Variation& variation) {
      variance += (mean - variation.Length()) * (mean - variation.Length());
   });

   double deviation = std::sqrt(variance / (std::distance(begin, end) - 1));
   return std::make_pair(mean, deviation);
}

void extractVariations(CBook& book,
                       const int bounds,
                       VariationCollection& variations,
                       const std::size_t maxVariations,
                       const std::size_t startCount)
{
   variations.clear();
   ExtractLines(book, variations, bounds, maxVariations);
   std::size_t drawnVariations
      = std::count_if(variations.begin(), variations.end(), is_drawn());
   const std::size_t keep = 30 * startCount;
   // keep the draws when there are 150 or more
   if( drawnVariations>=keep ) {
      variations.erase(variations.begin()+drawnVariations, variations.end());
   }
   // if lines vary too much in length, keep first 150
   else {
      std::pair<double, double> mean = StandardDeviation(variations.begin(), variations.begin()+std::min(keep, variations.size()));
      Log(std::cout)
         << boost::format("Mean@Deviation = %1$.02f@%2$.02f") % mean.first % mean.second
         << std::endl
         ;
      if( mean.second>2 && variations.size()>keep ) {
         std::size_t n_first = std::min(keep, variations.size());
         Log(std::cout)
            << "Prioritizing first "
            << n_first
            << " lines."
            << std::endl
            ;
         variations.erase(variations.begin()+n_first, variations.end());
      }
   }
   WriteVariations(variations, "variations.txt");
}

void server_fetch(std::iostream& stream,
                  VariationCollection& variations,
                  const std::size_t varToSend,
                  const std::size_t maxVariations)
{
   std::size_t linesToSend = std::min(varToSend, variations.size());
   Log(std::cout)
      << "Sending "
      << linesToSend
      << " variations to client ("
      << variations.size()
      << " in queue)."
      << std::endl
      ;
   stream << linesToSend << '\n';
   while( !variations.empty() && linesToSend-- ) {
      variations.front().OutputGGF(stream);
      variations.erase(variations.begin());
      stream << '\n';
   }
   stream << std::flush;
}

void server_fetch2(std::iostream& stream,
                   const CBook& serverBook,
                   VariationCollection& variations,
                   const std::size_t varToSend,
                   const std::size_t maxVariations,
                   bool isS26)
{
   std::size_t linesToSend = std::min(varToSend, variations.size());
   Log(std::cout)
      << "Sending "
      << linesToSend
      << " variations to client ("
      << variations.size()
      << " in queue)."
      << std::endl
      ;

   // extract variations and book
   VariationCollection subCollection
      = VariationCollection(variations.begin(), variations.begin()+linesToSend);
   variations.erase(variations.begin(), variations.begin()+linesToSend);
   CBookPtr bookExtract;
   if( isS26 ) {
      struct null_deleter {
         void operator()(void*) const {}
      };
      bookExtract.reset(&const_cast<CBook&>(serverBook), null_deleter());
   }
   else {
      bookExtract = serverBook.ExtractPositions(subCollection);
   }

   FetchCommand command(*bookExtract, subCollection);
   boost::archive::binary_oarchive oa(stream);
   oa << command;

   stream << std::flush;
}

void server_sync(std::iostream& stream,
                 CBook& computerBook,
                 const int bounds,
                 VariationCollection& variations,
                 const std::size_t maxVariations,
                 const std::size_t startCount,
                 std::size_t& varToSend,
                 bool isS26)
{
   const std::string joinBook("Coefficients/join.book");
   const std::string oldJoinBook = joinBook + ".old";
   ::remove(oldJoinBook.c_str());
   ::rename(joinBook.c_str(), oldJoinBook.c_str());
   CBook clientBook(joinBook.c_str(), no_save);
   Log(std::cout)
      << "Receiving book..."
      << std::endl
      ;
   boost::archive::binary_iarchive ia(stream);
   ia >> clientBook;
   // prune clientBook from low-ply positions
   clientBook.Prune(computerBook.GetPruneHeight());
   Log(std::cout)
      << "Done"
      << std::endl
      ;
   Log(std::cout)
      << "Joining books..."
      << std::endl
      ;
   computerBook.JoinBook(clientBook);
   computerBook.Prune();
   Log(std::cout)
      << "Done. Book contains "
      << computerBook.Positions()
      << " positions."
      << std::endl
      ;
   // save book
   computerBook.Mirror();
   //// filter variations, remove those that are in book already
   //VariationCollection::iterator it
   //   = std::remove_if(variations.begin(), variations.end(), has_all_deviations(computerBook));
   //if( it!=variations.end() ) {
   //   int filteredVariations = std::distance(it, variations.end());
   //   variations.erase(it, variations.end());
   //   Log(std::cout)
   //      << "Filtered "
   //      << filteredVariations
   //      << " variations that are in book already."
   //      << std::endl;
   //}
   // extract variations if necessary
   if( variations.size()<varToSend ) {
      int searches;
      computerBook.CorrectAll(searches);
      extractVariations(computerBook, bounds, variations, maxVariations, startCount);
      // adjust varToSend to allow faster processing of first few
      // lines, which are probably mostly endsolve lines anyway
      varToSend = 5U;
   }
}

void UpdateStats(const std::string& clientName, std::size_t inc)
{
   // update stats
   CreateDirectory(TEXT("stats"), 0);
   const std::string filename = "stats/" + clientName;
   std::size_t total = 0;
   {
      std::ifstream in(filename.c_str());
      if( in ) {
         in >> total;
      }
   }
   Log(std::cout)
      << "Adding "
      << inc
      << " nodes to "
      << filename
      << std::endl
      ;
   std::ofstream out(filename.c_str());
   out << total + inc;
}

template<class SyncCommand>
void server_sync_common(SyncCommand& syncCommand,
                        std::iostream& stream,
                        CBook& clientBook,
                        CBook& computerBook,
                        const int bounds,
                        VariationCollection& variations,
                        const std::size_t maxVariations,
                        std::size_t& varToSend,
                        const std::string& clientName,
                        std::size_t startCount)
{
   Log(std::cout)
      << "Receiving book..."
      << std::endl
      ;
   boost::archive::binary_iarchive ia(stream);
   ia >> syncCommand;
   // prune clientBook from low-ply positions
   clientBook.Prune(computerBook.GetPruneHeight());
   Log(std::cout)
      << boost::format("Done. Finished %1% variations.")
      % syncCommand.GetVariations().size()
      << std::endl
      ;
   Log(std::cout)
      << "Joining books..."
      << std::endl
      ;
   // number of positions before merge
   std::size_t positionsBeforeMerge = computerBook.Positions();
   computerBook.JoinBook(clientBook);
   computerBook.Prune();
   std::size_t positionsAfterMerge = computerBook.Positions();
   Log(std::cout)
      << boost::format("Done. Book contains %1% positions.")
      % positionsAfterMerge
      << std::endl
      ;
   // save book
   computerBook.Mirror();
   UpdateStats(clientName, positionsAfterMerge - positionsBeforeMerge);
   // filter variations, remove those that are in book already
   VariationCollection::iterator it
      = std::remove_if(variations.begin(),
                       variations.end(),
                       is_in(syncCommand.GetVariations()));
   if( it!=variations.end() ) {
      int filteredVariations = std::distance(it, variations.end());
      variations.erase(it, variations.end());
      Log(std::cout)
         << "Filtered "
         << filteredVariations
         << " variations that are in book already."
         << std::endl;
   }
   // extract variations if necessary
   if( variations.size()<varToSend ) {
      int searches;
      computerBook.CorrectAll(searches);
      extractVariations(computerBook, bounds, variations, maxVariations, startCount);
      // adjust varToSend to allow faster processing of first few
      // lines, which are probably mostly endsolve lines anyway
      varToSend = startCount;
   }
}

void server_sync2(std::iostream& stream,
                  CBook& computerBook,
                  const int bounds,
                  VariationCollection& variations,
                  const std::size_t maxVariations,
                  std::size_t& varToSend,
                  bool isS26,
                  const std::string& clientName,
                  std::size_t startCount)
{
   const std::string joinBook("Coefficients/join.book");
   const std::string oldJoinBook = joinBook + ".old";
   ::remove(oldJoinBook.c_str());
   ::rename(joinBook.c_str(), oldJoinBook.c_str());
   CBook clientBook(joinBook.c_str(), no_save);
   SyncCommand syncCommand(clientBook);
   server_sync_common(syncCommand, stream, clientBook, computerBook, bounds, variations, maxVariations, varToSend, clientName, startCount);
}

void server_sync3(std::iostream& stream,
                  CBook& computerBook,
                  const int bounds,
                  VariationCollection& variations,
                  const std::size_t maxVariations,
                  std::size_t& varToSend,
                  bool isS26,
                  const std::string& clientName,
                  std::size_t startCount)
{
   const std::string joinBook("Coefficients/join.book");
   const std::string oldJoinBook = joinBook + ".old";
   ::remove(oldJoinBook.c_str());
   ::rename(joinBook.c_str(), oldJoinBook.c_str());
   CBook clientBook(joinBook.c_str(), no_save);
   SyncCommand2 syncCommand(clientBook);
   server_sync_common(syncCommand, stream, clientBook, computerBook, bounds, variations, maxVariations, varToSend, clientName, startCount);
}

void server_merge(std::iostream& stream, const CBook& book)
{
   boost::archive::binary_oarchive oa(stream);
   Log(std::cout)
      << "Sending book..."
      << std::endl
      ;
   oa << book;
   Log(std::cout)
      << "Done"
      << std::endl
      ;
}

struct acceptor;
typedef std::shared_ptr<acceptor> acceptor_ptr;

struct acceptor
{
   boost::asio::io_service io_service;
   boost::asio::ip::tcp::acceptor accept;

   static acceptor_ptr create(int port)
   {
      return acceptor_ptr(new acceptor(port));
   }

private:

   acceptor(int server_port)
      : accept(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), server_port))
   {}
};

int server(int server_port, CComputerDefaults cd1, int bounds, bool isS26)
{
   int success = EXIT_SUCCESS;
   CPlayerComputerPtr computer = CPlayerComputer::Create(cd1, false);
   CBookPtr bp = computer->book.lock();
   if (!bp) {
      cerr << "ERR: You need a book to use Expand mode\n";
      return EXIT_FAILURE;
   }
#if !defined(_DEBUG)
   {
      int searches;
      bp->CorrectAll(searches);
   }
#endif

   std::size_t startCount = 5U;
   std::size_t varToSend = startCount;
   bool increaseStartCount = false;
   // try to keep maxVariations 30 times varToSend, or more
   std::size_t maxVariations = 30*varToSend;
   VariationCollection variations;
   extractVariations(*bp, bounds, variations, maxVariations, startCount);

   // this timer is used to control fetch&sync times, not too often
   // and not too seldom
   Timer<double> timer;
   // drop out after a long time, to enable
   // updates of the exe for example
   Timer<double> totalTimer;

   boost::circular_buffer<double> timeHistory(20);
   // adjust slowly by starting with an hour average
   timeHistory.push_back(3600);

   bool keepRunning = true;

   using boost::asio::ip::tcp;

   acceptor_ptr acceptor = acceptor::create(server_port);

   // 4 days
   while( keepRunning && totalTimer.elapsed()<4*24*3600 ) {
      try {
         Log(std::cout) << "Waiting for client to connect on port " << server_port << "." << std::endl;
         tcp::iostream stream;
         acceptor->accept.accept(*stream.rdbuf());
         Log(std::cout) << "Client connected." << std::endl;

         std::string command;
         if( std::getline(stream, command) ) {
            boost::algorithm::trim(command);
            Log(std::cout) << "Received from client: " << command << std::endl;
            if( command.substr(0, SYNC3_COMMAND.size())==SYNC3_COMMAND ) {
               std::string clientName = command.substr(SYNC3_COMMAND.size()+1);
               server_sync3(stream, *bp, bounds, variations, maxVariations, varToSend, isS26, clientName, startCount);
               stream.close();
            }
            else if( command.substr(0, SYNC2_COMMAND.size())==SYNC2_COMMAND ) {
               std::string clientName = command.substr(SYNC2_COMMAND.size()+1);
               server_sync2(stream, *bp, bounds, variations, maxVariations, varToSend, isS26, clientName, startCount);
               stream.close();
            }
            else if( command.substr(0, SYNC_COMMAND.size())==SYNC_COMMAND ) {
               server_sync(stream, *bp, bounds, variations, maxVariations, startCount, varToSend, isS26);
               stream.close();
            }
            else if( command.substr(0, FETCH2_COMMAND.size())==FETCH2_COMMAND ) {
               server_fetch2(stream, *bp, variations, varToSend, maxVariations, isS26);
               stream.close();
               // extract variations if necessary
               if( variations.size()<varToSend ) {
                  int searches;
                  bp->CorrectAll(searches);
                  extractVariations(*bp, bounds, variations, maxVariations, startCount);
                  if( increaseStartCount )
                     startCount = (std::size_t)std::max(startCount + 2., startCount * 1.1);
                  else
                     startCount = std::max(5U, startCount - 2);
                  Log(std::cout) << "Start count adjusted to " << startCount << std::endl;
                  varToSend = startCount;
                  increaseStartCount = true;
               }
               timeHistory.push_back(timer.elapsed());
               // compute mean sync time
               std::vector<double> times(timeHistory.begin(), timeHistory.end());
               std::sort(times.begin(), times.end());
               double meanTime = times.at(times.size()/2);
               Log(std::cout) << "Mean sync time is " << static_cast<int>(meanTime) << " seconds." << std::endl;
               // try to adjust varToSend so that time between fetches is around 15 minutes
               int drawnVariations = std::count_if(variations.begin(), variations.end(), is_drawn());
               if( drawnVariations<5 ) {
                  if( meanTime<10*60 ) {
                     varToSend += 2;
                  }
                  else if( meanTime>20*60 ) {
                     increaseStartCount = false;
                     varToSend -= 2;
                  }
                  else  {
                     increaseStartCount = false;
                  }

                  // never less than 5
                  varToSend = std::max(5U, varToSend);
                  // keep maxVariations at 30 times varToSend, or more
                  maxVariations = std::max(30*varToSend, maxVariations);
                  Log(std::cout) << "Adjusted varToSend to " << varToSend << " and maxVariations to " << maxVariations << std::endl;
               }
               else {
                  Log(std::cout) << boost::format("%1% draws remaining in queue") % drawnVariations << std::endl;
               }
               // reset timer
               timer.start();
            }
            else if( command.substr(0, FETCH_COMMAND.size())==FETCH_COMMAND ) {
               Log(std::cout) << "Client needs upgrade." << std::endl;
               stream.close();
               //acceptor.reset();
            }
            else if( command.substr(0, MERGE_COMMAND.size())==MERGE_COMMAND ) {
               Log(std::cout) << "Client requests merge." << std::endl;
               server_merge(stream, *bp);
               stream.close();
               //acceptor.reset();
               Log(std::cout) << "Sent book to client for merging." << std::endl;
            }
            else if( command.substr(0, QUIT_COMMAND.size())==QUIT_COMMAND ) {
               keepRunning = false;
            }
         }
      }
      catch( std::exception& ex )
      {
         std::cerr << ex.what() << std::endl;
      }
   }
   bp->Write();

   return success;
}

int Server(int argc, char** argv, const CComputerDefaults& cd1, int nGames)
{
   int success = EXIT_SUCCESS;

   bool isS26 = cd1.sCalcParams=="s26";
   if( argc<7 ) {
      std::cerr << "No server port specified." << std::endl;
   }
   else {
      try {
         int server_port = boost::lexical_cast<int>(argv[6]);
         success = server(server_port, cd1, nGames, isS26);
      }
      catch( std::bad_cast& ) {
         std::cerr << "Invalid server port." << std::endl;
         success = EXIT_FAILURE;
      }
   }

   return success;
}
