// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_SERVICE_GAME_H
#define GDK_SERVICE_GAME_H

#include "SGBase.h"
#include "SGObjects.h"
#include "SGMessages.h"

#if defined(_MSC_VER) && !(_MSC_VER<1300)
#define TYPENAME typename
#else
#define TYPENAME
#endif

template<class TRules>
class CSG : public CSGBase {
public:
	typedef typename TRules::TMatchType	TMatchType;
	typedef typename TRules::TBoardType	TBoardType;
	typedef typename TRules::TGame	TGame;

	typedef TYPENAME CSGRequest<TRules>	TRequest;

	typedef CMsgSGAbortRequest				TMsgAbortRequest;
	typedef CMsgSGComment					TMsgComment;
	typedef CMsgSGEnd						TMsgEnd;
	typedef CMsgSGErr						TMsgErr;
	typedef CMsgSGFatalTimeout				TMsgFatalTimeout;
	typedef CMsgSGFinger					TMsgFinger;
	typedef CMsgSGMatchDelta				TMsgMatchDelta;
	typedef CMsgSGHistory					TMsgHistory;
	typedef CMsgSGJoin<TRules>				TMsgJoin;
	typedef	CMsgSGLook<TRules>				TMsgLook;
	typedef	CMsgSGMatch						TMsgMatch;
	typedef CMsgSGMatchDelta				TMsgMatchDelta;
	typedef CMsgSGRank						TMsgRank;
	typedef CMsgSGRatingUpdate<TRules>		TMsgRatingUpdate;
	typedef CMsgSGRequestDelta<TRules>		TMsgRequestDelta;
	typedef CMsgSGStored					TMsgStored;
	typedef CMsgSGTDStart<TRules>			TMsgTDStart;
	typedef CMsgSGTimeout<TRules>			TMsgTimeout;
	typedef CMsgSGTop<TRules>				TMsgTop;
	typedef CMsgSGTrustViolation<TRules>	TMsgTrustViolation;
	typedef CMsgSGUndoRequest<TRules>		TMsgUndoRequest;
	typedef CMsgSGUnknown<TRules>			TMsgUnknown;
	typedef CMsgSGUpdate<TRules>			TMsgUpdate;
	typedef CMsgSGWatch<TRules>				TMsgWatch;
	typedef CMsgSGWatchDelta<TRules>		TMsgWatchDelta;
	typedef CMsgSGWho<TRules>				TMsgWho;

	CSG(ggsstream* apgs);
	virtual ~CSG() {};

	virtual void Parse(const string& sRaw, const string& sFrom, istream& is);

	// overridable handlers
	virtual void Handle					(const CMsg& msg);

	virtual void HandleAbortRequest		(const TMsgAbortRequest& msg);
	virtual void HandleComment			(const TMsgComment& msg);
	virtual void HandleEnd				(const TMsgEnd& msg);
	virtual void HandleErr				(const TMsgErr& msg);
	virtual void HandleFatalTimeout		(const TMsgFatalTimeout& msg);
	virtual void HandleFinger			(const TMsgFinger& msg);
	virtual void HandleGameOver			(const TMsgMatchDelta& msg, const string& idg);
	virtual void HandleHistory			(const TMsgHistory& msg);
	virtual void HandleJoin				(const TMsgJoin& msg);
	virtual void HandleLook				(const TMsgLook& msg);
	virtual void HandleMatch			(const TMsgMatch& msg);
	virtual void HandleMatchDelta		(const TMsgMatchDelta& msg);
	virtual void HandleRank				(const TMsgRank& msg);
	virtual void HandleRatingUpdate		(const TMsgRatingUpdate& msg);
	virtual void HandleRequestDelta		(const TMsgRequestDelta& msg);
	virtual void HandleStored			(const TMsgStored& msg);
	virtual void HandleTDStart			(const TMsgTDStart& msg);
	virtual void HandleTimeout			(const TMsgTimeout& msg);
	virtual void HandleTop				(const TMsgTop& msg);
	virtual void HandleTrustViolation	(const TMsgTrustViolation& msg);
	virtual void HandleUndoRequest		(const TMsgUndoRequest& msg);
	virtual void HandleUnknown			(const TMsgUnknown& msg);
	virtual void HandleUpdate			(const TMsgUpdate& msg);
	virtual void HandleWatch			(const TMsgWatch& msg);
	virtual void HandleWatchDelta		(const TMsgWatchDelta& msg);
	virtual void HandleWho				(const TMsgWho& msg);

	// Base handlers
	void BaseEnd			(const TMsgEnd& msg);
	void BaseGameOver		(const TMsgMatchDelta& msg, const string& idg);
	void BaseJoin			(const TMsgJoin& msg);
	void BaseMatch			(const TMsgMatch& msg);
	void BaseMatchDelta		(const TMsgMatchDelta& msg);
	void BaseRequestDelta	(const TMsgRequestDelta& msg);
	bool BaseUpdate			(const TMsgUpdate& msg);


	// persistent data
	map<string,TGame> idToGame;
	map<string,TRequest> idToRequest;

	virtual CSGGameBase* PGame(const string& idg);
	virtual TRequest* PRequest(const string& idr);

	int WatchEnd(const string& idm);
	virtual int ViewerUpdate(const string& idm, const CSGMoveListItem& mli);

	virtual void TDType(const string& smt, string& smtOut, string& smtDescription, double& dMaxResult) const;

protected:
	// helper function for BaseMatchDelta
	virtual void EndGame(const TMsgMatchDelta& msg, const string& idg);
};

//#include "SG_T.h"

#endif // GDK_SERVICE_GAME_H
