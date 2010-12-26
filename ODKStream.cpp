// Copyright Chris Welty
//  All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "ODKStream.h"
#include "Pos2.h"
#include "GDK/GGSMessage.h"
#include "GDK/SGMessages.h"
#include "Player.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

bool accepted(std::string name);
bool rejected(std::string name);
void init_lists();

static CNodeStats timer, timer2;
static std::vector<std::string> accept_list, reject_list;

void CODKStream::Handle(const CMsg& msg) {
   std::cout << msg.sRawText << "\n";
}

void CODKStream::HandleLogin() {
   BaseLogin();
   (*this) << "mso\n";
   ifstream infile("autosend.txt");
   string line;
   if( infile )
      while( std::getline(infile,line) )
         (*this) << line.c_str() << '\n';
   infile.close();

   flush();
   timer.Read();
}

void CODKStream::HandleUserDelta(const CMsgGGSUserDelta& msg) {
   BaseUserDelta(msg);

   //if( msg.fPlus ) { // handle coming
   //  if( msg.sLogin=="dan" )
   //    (*this) << "t dan Greetings, master.\n";
   //  flush();
   //}
}

void CODKStream::HandleTell(const CMsgGGSTell& msg) {

   const char* super_users[] = {"dan", "zebra7", "zebra12", "Reindeer", "rabbit", "HCyrano", "delorme"};
   const int N = sizeof(super_users) / sizeof(super_users[0]);

   bool from_su = false;
   for( int i=0; i<N; i++ ) {
      if( string(super_users[i])==msg.sFrom ) {
         from_su = true;
         std::cout << "REMOTE cmd:" << " " << msg.sFrom << ": " << '`' << msg.sText << "\'\n";
         if (msg.sText=="quit")
            Logout();
         else if (msg.sText==":correct all") {
            int nSearches;
            this->pComputer->book.lock()->CorrectAll(nSearches);
         }
         else if (msg.sText==":reload openings") {
            InitForcedOpenings();
            (*this) << "tell " << msg.sFrom << " issued your command: `" << msg.sText << "'\n";
         }
         else if( msg.sText=="tournament" ) {
            CPlayerComputer::DEFER_ANALYSIS ^= 1;
            (*this) << "tell " << msg.sFrom << " Tournament mode is " << (CPlayerComputer::DEFER_ANALYSIS?"on":"off") << '\n';
         }
         else if( msg.sText=="?" ) {
            (*this) << "tell " << msg.sFrom;
            // Get and display the name of the computer.
            (*this) << " " << "Computer name: " << GetComputerNameAsString() << "\\";
            (*this) << "Compiled " __DATE__ " at " __TIME__ "\\";
            (*this) << "super-user(s):";
            for( int i=0; i<N; i++ )
               (*this) << ' ' << super_users[i];
            (*this) << '\\';
            (*this) << "Commands:\\";
            (*this) << "':reload openings' will reload openings files\\";
            (*this) << "':correct all' will correct opening book (might take some time)\\";
            (*this) << "'tournament' will toggle tournament mode\\";
            (*this) << "'accept + user' will add to accept list\\";
            (*this) << "'reject + user' will add to reject list\\";
            (*this) << "Tournament mode is " << (CPlayerComputer::DEFER_ANALYSIS?"on":"off") << '\\';
            (*this) << "Accept list: " << boost::algorithm::join(accept_list, ",") << '\\';
            (*this) << "Reject list: " << boost::algorithm::join(reject_list, ",") << '\n';
         }
         else if( msg.sText=="clear-accept" ) {
            accept_list.erase(accept_list.begin(), accept_list.end());
            (*this) << "tell " << msg.sFrom << " cleared accept list\n";
         }
         else if( msg.sText=="clear-reject" ) {
            reject_list.erase(reject_list.begin(), reject_list.end());
            (*this) << "tell " << msg.sFrom << " cleared reject list\n";
         }
         else if( msg.sText.substr(0,8)=="accept +" ) {
            stringstream stream(msg.sText);
            string n;
            stream >> n;
            stream >> n;
            stream >> n;
            accept_list.push_back(n);
            (*this) << "tell " << msg.sFrom << " added " << n << " to accept list\n";
         }
         else if( msg.sText.substr(0,8)=="reject +" ) {
            stringstream stream(msg.sText);
            string n;
            stream >> n;
            stream >> n;
            stream >> n;
            reject_list.push_back(n);
            (*this) << "tell " << msg.sFrom << " added " << n << " to reject list\n";
         }
         else {
            (*this) << "tell " << msg.sFrom << " issued your command: `" << msg.sText << "'\n";
            (*this) << msg.sText << "\n";
         }
         flush();
      }
   }
   if( !from_su ) {
      std::cout << msg.sRawText << endl;
   }
}

void CODKStream::HandleUnknown(const CMsgGGSUnknown& msg) {
   std::cout << "Unknown GGS message: \n";
   Handle(msg);
}

// create a service, if the sLogin is a service type supported by this app
CSGBase* CODKStream::CreateService(const string& sUserLogin) {
   CSGBase* pService=NULL;
   if (!HasService(sLogin)) {
      if (sUserLogin=="/os") {
         pService=new CSGNovello(this);
      }
      if (pService)
         loginToPService[sUserLogin]=pService;
   }

   return pService;
}

////////////////////////////
// CSGNovello
////////////////////////////

CSGNovello::CSGNovello(ggsstream* apgs) : CSG<COsRules>(apgs) {
   init_lists();
}

void CSGNovello::HandleGameOver(const TMsgMatchDelta& msg,const string& idg) {

   static int n_lost[2] = { 0 };
   static int n_won[2]  = { 0 };
   static int n_draw[2] = { 0 };

   std::string channel = "." + pgs->GetLogin();

   if (msg.match.IsPlaying(pgs->GetLogin())) {
      //SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST);
      bool online = true;
      bool rand = msg.match.sMatchType.find('r')!=std::string::npos
         || msg.match.sMatchType.find('R')!=std::string::npos;
      bool selfGame
         = (msg.match.pis[0].sName==msg.match.pis[1].sName);

      (*pgs) << "repeat\n";
      if( msg.result.status==CSGResult::kNormalEnd && !selfGame ) {
         if( (msg.result.dResult<0 && msg.match.pis[0].sName==pgs->GetLogin()
              || msg.result.dResult>0 && msg.match.pis[1].sName==pgs->GetLogin()) ) {
            if( !rand ) {
               std::string time_string =
                  boost::lexical_cast<std::string>(boost::posix_time::second_clock::local_time());
               (*pgs) << "v lost " << time_string << "\n" << std::flush;
            }
            ++n_lost[rand?1:0];
         }
         else if( (msg.result.dResult>0 && msg.match.pis[0].sName==pgs->GetLogin()
                   || msg.result.dResult<0 && msg.match.pis[1].sName==pgs->GetLogin()) ) {
            if( !rand ) {
               std::string time_string =
                  boost::lexical_cast<std::string>(boost::posix_time::second_clock::local_time());
               (*pgs) << "v won " << time_string << "\n" << std::flush;
            }
            ++n_won[rand?1:0];
         }
         else
            n_draw[rand?1:0] ++;
      }
      (*pgs) << "tell " << channel << " W/L/D = " << n_won[0] << "/" << n_lost[0] << "/" << n_draw[0] << " in session (std).\n" << std::flush;
      (*pgs) << "tell " << channel << " W/L/D = " << n_won[1] << "/" << n_lost[1] << "/" << n_draw[1] << " in session (rand).\n" << std::flush;
      if( !rand ) {
         (*pgs) << "tell " << channel << " " << idg << " Adding game to book...\n" << std::flush;
         CNodeStats start, end;
         start.Read();
         Timer<double> timer;
         PComputer()->EndGame(idToGame[idg], timer);
         CBookPtr bp = PComputer()->book.lock();
         DrawCount dc = CountDraws(*bp, std::cout);
         end.Read();
         if( online ) {
            (*pgs) << "tell " << channel << " " << idg << " Analysis complete in " << int((end-start).Seconds()) << " seconds\n" << std::flush;
            (*pgs) << "tell " << channel << " " << dc.edmunds << " edmund nodes detected.\n" << std::flush;
            (*pgs) << "tell " << channel << " " << bp->Positions() << " positions in book.\n" << std::flush;
         }
         //! check if it is time to exit
         ifstream stop_file("exit_after_analysis.txt");
         if( stop_file ) {
            if( online )
               (*pgs) << "q\n" << std::flush;
            stop_file.close();
         }
      }
      if( online )
         (*pgs) << "gameover\n" << std::flush;
   }

   BaseGameOver(msg, idg);
}

void CSGNovello::HandleJoin(const TMsgJoin& msg) {
   if( msg.sFrom!=pgs->GetLogin() ) {
      //SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
      std::cout << pgs->GetLogin() << endl;
      std::cout << msg.sFrom << endl;
      (*pgs) << "tell /os open 0\n";
      pgs->flush();

      BaseJoin(msg);
      MakeMoveIfNeeded(msg.idg);
   }
}

void CSGNovello::HandleMatchDelta(const TMsgMatchDelta& msg) {
   if (msg.match.IsPlaying(pgs->GetLogin()) && msg.fPlus)
      PComputer()->StartMatch(msg.match);
   BaseMatchDelta(msg);
}

void CSGNovello::HandleRequestDelta(const TMsgRequestDelta& msg) {
   BaseRequestDelta(msg);
   bool fGeneric = msg.request.pis[1].sName.empty();

   string name = msg.request.pis[0].sName;
   double oppRating = msg.request.pis[0].dRating;
   double myRating = msg.request.pis[1].dRating;

   bool me = msg.request.pis[0].sName==pgs->GetLogin();

   if( me ) {
      name = msg.request.pis[1].sName;
      std::swap(oppRating, myRating);
   }

   bool decline = false;

   if( accepted(name) ) {
      if( rejected(name) ) {
         cout << "Player " << name << " is on rejected list" << endl;
         decline = true;
      }
   }
   else {
      cout << "Player " << name << " is not on accepted list" << endl;
      decline = true;
   }

   if( decline ) {
      (*pgs) << "t /os decline " << msg.idr << "\n";
   }
   else if( !me ) {
      // if Saio, only accept lower rated 1/10
      if( name.substr(0, 4)!="Saio" || oppRating>myRating || rand()%10 )
         (*pgs) << "t /os accept " << msg.idr << "\n";
   }

   pgs->flush();
}

void CSGNovello::HandleUnknown(const TMsgUnknown& msg) {
   std::cout << "Unknown /os message: ";
   Handle(msg);
}

void CSGNovello::HandleUpdate(const TMsgUpdate& msg) {
   BaseUpdate(msg);
   MakeMoveIfNeeded(msg.idg);
}

// helper function for join and update messages
void CSGNovello::MakeMoveIfNeeded(const string& idg) {
   COsGame* pgame=dynamic_cast<COsGame*>(PGame(idg));

   // the computer player needs to know if this is game 1 of a synch match
   //  so it can use the appropriate hashtable
   bool fSynchGame1=false;
   u4 loc = idg.find('.',1);
   if (loc!=idg.npos && loc+1 < idg.size())
      fSynchGame1= idg[loc+1]=='1';

   QSSERT(pgame);
   if (pgame!=NULL) {
      bool fMyMove=pgame->ToMove(pgs->GetLogin());
      CSGMoveListItem mli;

      if( fMyMove ) {
         (*pgs) << "repeat "
                << pgame->pos.cks[pgame->pos.board.iMover].tCurrent
                << " tell /os break\n";
         std::cout << "Will break game in " << pgame->pos.cks[pgame->pos.board.iMover].tCurrent
                   << 's' << endl;
      }
      else
         (*pgs) << "repeat\n";
      pgs->flush();

      PComputer()->Update(*pgame, fMyMove, mli);

      if (fMyMove) {
         (*pgs) << "tell /os play " << idg << " " << mli << "\n";
         (*pgs) << "tell ." << pgs->GetLogin() << " " << idg << " " << PComputer()->GetSearchStats();
         CBookData* bd = PComputer()->book.lock()->FindAnyReflection(CQPosition(pgame->pos.board).BitBoard());
         if( bd )
         {
            std::ostringstream stream;
            bd->Out(stream, false);
            (*pgs) << ' ' << stream.str();
            DrawCount dc = GetDrawCount(CQPosition(pgame->pos.board).BitBoard());
            (*pgs) << ' ' << dc.draws << "/" << dc.privateDraws << "/" << dc.edmunds;
         }
         (*pgs) << '\n';
         pgs->flush();
      }
   }
   else
      QSSERT(0);
}

bool accepted(std::string name)
{
   bool b = accept_list.empty() || std::find(accept_list.begin(), accept_list.end(), name)!=accept_list.end();
   if( b )
      std::cout << name << " is accepted" << endl;
   return b;
}

bool rejected(std::string name)
{
   bool b = std::find(reject_list.begin(), reject_list.end(), name)!=reject_list.end();
   if( b )
      std::cout << name << " is not rejected" << endl;
   return b;
}

void init_lists()
{
   ifstream in("accepted.txt");
   string name;
   if( in ) {
      while( in >> name )
         accept_list.push_back(name);
      in.close();
   }
   in.clear();
   in.open("rejected.txt");
   if( in ) {
      while( in >> name )
         reject_list.push_back(name);
      in.close();
   }
}
