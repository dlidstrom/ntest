// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "SG.h"
#include "ggsstream.h"
#include <strstream>
using namespace std;

template<class TRules>
CSG<TRules>::CSG(ggsstream* apgs) : CSGBase(apgs, TRules::sLogin) {
	// required commands for ODK to work:
	(*pgs) << "tell " << sLogin << " client +\n";	// get compact messages
	(*pgs) << "tell " << sLogin << " notify + *\n";	// get game start/end messages
	(*pgs) << "tell " << sLogin << " trust +\n";	// trust Lion's time
	pgs->flush();
}

template<class TRules>
void CSG<TRules>::TDType(const string& smt, string& smtOut, string& smtDescription, double& dMaxResult) const {
	typename TRules::TMatchType mt(smt);
	mt.GetTDOut(smtOut);
	smtDescription=mt.Description() + " " + TRules::sGameName;
	dMaxResult=mt.MaxResult();
}

template<class TRules>
void CSG<TRules>::Parse(const string& sRaw, const string& sFrom, istream& is) {
	string sMsgType;
	is >> sMsgType >> ws;

	if (sMsgType[0]=='.')
		HandleComment(TMsgComment(sRaw, sFrom, is,sMsgType));
	else if (sMsgType=="abort")
		HandleAbortRequest(TMsgAbortRequest(sRaw, sFrom, is));
	else if (sMsgType=="end")
		HandleEnd(TMsgEnd(sRaw, sFrom, is));
	else if (sMsgType=="ERR")
		HandleErr(TMsgErr(sRaw, sFrom, is));
	else if (sMsgType=="fatal-timeout")
		HandleFatalTimeout(TMsgFatalTimeout(sRaw, sFrom, is));
	else if (sMsgType=="finger")
		HandleFinger(TMsgFinger(sRaw, sFrom, is));
	else if (sMsgType=="history") {
		if (is.peek()=='E')
			HandleErr(TMsgErr(sRaw, sFrom, is));
		else
			HandleHistory(TMsgHistory(sRaw, sFrom, is));
	}
	else if (sMsgType=="illegal")
		HandleErr(TMsgErr(sRaw, sFrom, is));
	else if (sMsgType=="join")
		HandleJoin(TMsgJoin(sRaw, sFrom, is, this));
	else if (sMsgType=="look")
		HandleLook(TMsgLook(sRaw, sFrom, is));
	else if (sMsgType=="match")
		HandleMatch(TMsgMatch(sRaw, sFrom, is));
	else if (sMsgType=="rank")
		HandleRank(TMsgRank(sRaw, sFrom, is));
	else if (sMsgType=="rating_update")
		HandleRatingUpdate(TMsgRatingUpdate(sRaw, sFrom, is));
	else if (sMsgType=="stored") {
		if (sRaw.find("ERR not found")!=sRaw.npos)
			HandleErr(TMsgErr(sRaw, sFrom, is));
		else
			HandleStored(TMsgStored(sRaw, sFrom, is));
	}
	else if (sMsgType=="tdstart")
		HandleTDStart(TMsgTDStart(sRaw, sFrom, is));
	else if (sMsgType=="timeout")
		HandleTimeout(TMsgTimeout(sRaw, sFrom, is));
	else if (sMsgType=="top")
		HandleTop(TMsgTop(sRaw, sFrom, is));
	else if (sMsgType=="trust-violation")
		HandleTrustViolation(TMsgTrustViolation(sRaw, sFrom, is));
	else if (sMsgType=="undo")
		HandleUndoRequest(TMsgUndoRequest(sRaw, sFrom, is));
	else if (sMsgType=="update")
		HandleUpdate(TMsgUpdate(sRaw, sFrom, is, false));
	else if (sMsgType=="watch") {
		is >> ws;
		int c=is.peek();
		if (c=='+' || c=='-')
			HandleErr(TMsgErr(sRaw, sFrom, is));
		else
			HandleWatch(TMsgWatch(sRaw, sFrom, is));
	}
	else if (sMsgType=="who")
		HandleWho(TMsgWho(sRaw, sFrom, is));
	else if (sMsgType=="+" || sMsgType=="-") {
		bool fPlus= sMsgType=="+";
		is >> ws;
		if (is.peek()=='.')
			HandleRequestDelta(TMsgRequestDelta(sRaw, sFrom, is, fPlus));
		else {
			is >> sMsgType;
			if (sMsgType=="match")
				HandleMatchDelta(TMsgMatchDelta(sRaw, sFrom, is, fPlus));
			else {
				string sLogin=sMsgType;
				is >> sMsgType;
				if (sMsgType=="watch") {
					HandleWatchDelta(TMsgWatchDelta(sRaw, sFrom, is, fPlus, sLogin));
				}
				else{
					// os: + booklet stored
					HandleUnknown(TMsgUnknown(sRaw, sFrom, is, "--"));
				}
			}
		}
	}
	else
		Handle(TMsgUnknown(sRaw, sFrom, is, sMsgType));
}

template<class TRules>
void CSG<TRules>::Handle(const CMsg& msg) {
	pgs->Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleMatchDelta	(const TMsgMatchDelta& msg) {
	BaseMatchDelta(msg);
	Handle(msg);
};

template<class TRules>
void CSG<TRules>::BaseMatchDelta	(const TMsgMatchDelta& msg) {
	map<string,CSGMatch>::iterator i=idToMatch.find(msg.match.idm);
	if (msg.fPlus) {
		QSSERT(i==idToMatch.end());
		idToMatch[msg.match.idm]=msg.match;
	}
	else {
		TMatchType mt=msg.match.sMatchType;

		if (i!=idToMatch.end()) {

			if (mt.cColor=='s') {
				EndGame(msg, msg.match.idm+".0");
				EndGame(msg, msg.match.idm+".1");
			}
			else {
				EndGame(msg, msg.match.idm);
			}
			idToMatch.erase(i);
		}
	}
}

//////////////////////////
// Overridable handlers
//////////////////////////

/*
void CSG<TRules>::Handle(const TMsg& msg) {
	pgs->Handle(msg);
}
*/

template<class TRules>
void CSG<TRules>::HandleAbortRequest(const TMsgAbortRequest& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleComment(const TMsgComment& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleEnd(const TMsgEnd& msg) {
	BaseEnd(msg);
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleErr(const TMsgErr& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleFatalTimeout(const TMsgFatalTimeout& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleFinger(const TMsgFinger& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleGameOver(const TMsgMatchDelta& msg, const string& idg) {
	BaseGameOver(msg, idg);
}

template<class TRules>
void CSG<TRules>::HandleHistory(const TMsgHistory& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleJoin(const TMsgJoin& msg) {
	BaseJoin(msg);
	Handle(msg);
}


template<class TRules>
void CSG<TRules>::HandleLook(const TMsgLook& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleMatch(const TMsgMatch& msg) {
	BaseMatch(msg);
	Handle(msg);
}

/*
void CSG<TRules>::HandleMatchDelta(const TMsgMatchDelta& msg) {
	BaseMatchDelta(msg);
	Handle(msg);
}
*/

template<class TRules>
void CSG<TRules>::HandleRank(const TMsgRank& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleRatingUpdate(const TMsgRatingUpdate& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleRequestDelta(const TMsgRequestDelta& msg) {
	BaseRequestDelta(msg);
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleStored(const TMsgStored& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleTDStart(const TMsgTDStart& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleTimeout(const TMsgTimeout& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleTop(const TMsgTop& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleTrustViolation(const TMsgTrustViolation& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleUndoRequest(const TMsgUndoRequest& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleUnknown(const TMsgUnknown& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleUpdate(const TMsgUpdate& msg) {
	BaseUpdate(msg);
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleWatch(const TMsgWatch& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleWatchDelta(const TMsgWatchDelta& msg) {
	Handle(msg);
}

template<class TRules>
void CSG<TRules>::HandleWho(const TMsgWho& msg) {
}

///////////////////////////
// Persistent data access
///////////////////////////

template<class TRules>
CSGGameBase* CSG<TRules>::PGame(const string& idg) {
	map<string, TGame>::iterator i = idToGame.find(idg);
	if (i==idToGame.end())
		return NULL;
	else
		return &((*i).second);
}

template<class TRules>
TYPENAME CSG<TRules>::TRequest* CSG<TRules>::PRequest(const string& idr) {
	map<string,TRequest>::iterator i=idToRequest.find(idr);
	if (i==idToRequest.end())
		return NULL;
	else
		return &((*i).second);
}

///////////////////////////
// Base handlers
///////////////////////////

template<class TRules>
void CSG<TRules>::BaseEnd(const TMsgEnd& msg) {
	// end messages occur at the end of some synch games
	//	to let you know the result
	TGame* pgame=dynamic_cast<TGame*>(PGame(msg.idg));
	if (pgame) {
		QSSERT(pgame->mt.cColor=='s');
		pgame->SetResult(msg.result, msg.sPlayers[0]);
	}
}

template<class TRules>
void CSG<TRules>::BaseGameOver(const TMsgMatchDelta& msg, const string& idg) {
	idToGame.erase(idg);
}

// we get a "Join" message (and the whole game is sent)
//	when we join the game,  when komi is set in a game,
//	and when a move is undone in a game.
template<class TRules>
void CSG<TRules>::BaseJoin(const TMsgJoin& msg) {
	idToGame[msg.idg]=msg.game;
}

template<class TRules>
void CSG<TRules>::BaseMatch(const TMsgMatch& msg) {
	idToMatch.clear();
	vector<CSGMatch>::const_iterator i;

	for (i=msg.matches.begin(); i!=msg.matches.end(); i++)
		idToMatch[(*i).idm]=*i;
}

// helper function for BaseMatchDelta
template<class TRules>
void CSG<TRules>::EndGame(const TMsgMatchDelta& msg, const string& idg) {
	TGame* pgame=dynamic_cast<TGame*>(PGame(idg));
	if (pgame) {
		// synch games with normal termination should have
		//	gotten an "End" Message, the result in this msg will be the
		//	match result, i.e. the average of the two game
		//	results, so we ignore it as the result for this game.
		// Other normally terminated games
		//	will have set the result upon game completion but we use the
		//	result reported by GGS in case of timeouts
		// All other games we use the GGS result from this message.
		if (pgame->result.status==CSGResult::kNormalEnd) {
			QSSERT(pgame->mt.cColor=='s');
		}
		else {
			pgame->SetResult(msg.result, msg.match.pis[0].sName);
		}
		HandleGameOver(msg, idg);
	}
}

// delete the request if we have it. We might not, e.g. if we've just logged in
template<class TRules>
void CSG<TRules>::BaseRequestDelta(const TMsgRequestDelta& msg) {
	map<string,TRequest>::iterator i=idToRequest.find(msg.idr);
	if (msg.fPlus) {
		//QSSERT(i==idToRequest.end());
		idToRequest[msg.idr]=msg.request;
	}
	else {
		if (i!=idToRequest.end())
			idToRequest.erase(i);
	}
}

template<class TRules>
bool CSG<TRules>::BaseUpdate(const TMsgUpdate& msg) {
	// update the game if it exists. Due to lag, we might still
	//	be getting updates for games we've stopped watching
	bool fSynch = false;

	map<string,TGame>::iterator i=idToGame.find(msg.idg);
	if (i!=idToGame.end()) {
		typename TRules::TGame& game = (*i).second;
		fSynch = !game.fSynchronized;
		if (game.fSynchronized) {
			game.Update(msg.mli);
			game.fSynchronized=!msg.fFromViewer;
		}
		else {
			int nMoves=game.ml.size();
			QSSERT(nMoves);
			QSSERT(!msg.fFromViewer);
			if (nMoves) {
				game.ml[nMoves-1]=msg.mli;
				game.CalcCurrentPos();
				game.fSynchronized=true;
			}
		}
	}
	return fSynch;
}

// WatchEnd
// WARNING: Does nothing in the case of synch matches
template<class TRules>
int CSG<TRules>::WatchEnd(const string& idm) {
	int nGone=idToGame.erase(idm);
	if (nGone)
		Tell() << "watch - " << idm << "\n" << flush;
	return nGone;
}

template<class TRules>
int CSG<TRules>::ViewerUpdate(const string& idg, const CSGMoveListItem& mli) {
	(*pgs) << "tell " << sLogin << " play " << idg << " " << mli << "\n" << '\0';
	pgs->flush();

	CMsgSGUpdate<TRules> msg;
	msg.fFromViewer=true;
	msg.fSynch=false;
	msg.idg=idg;
	msg.mli=mli;
	HandleUpdate(msg);

	return 0;
}
