// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_SGMessage_H
#define GDK_SGMessage_H

#include "GGSMessage.h"
#include "SGObjects.h"

//class CSG;

// /os: abort .7 n3 is asking
class CMsgSGAbortRequest: public CMsg {
public:
	CMsgSGAbortRequest(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGAbortRequest();

	string idg, sLogin;
};

// /os: .44 A 1720 n3: test from me
class CMsgSGComment : public CMsg {
public:
	CMsgSGComment(const string& sMsg, const string& asFrom, istream& is, const string& idm);
	virtual ~CMsgSGComment();

	string idg;
	char cTo;
	CSGPlayerInfo pi;
	string sComment;
};

// /os: end .2.0 ( pamphlet vs. ant ) +46.00
class CMsgSGEnd : public CMsg {
public:
	CMsgSGEnd(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGEnd();

	string idg;
	string sPlayers[2];
	CSGResult result;
};

class CMsgSGErr: public CMsg {
public:
	CMsgSGErr(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGErr();

	enum { kErrUnknown=0x8300, kErrRequestDoesntFitFormula, kErrIllegalBoardType,
			kErrBadRankMessage, kErrCorruptMove, kErrOpenTooLow, 
			kErrRatedMismatch, kErrUnregistered, kErrIllegalMove,
			kErrUserNotFound, kErrNotAcceptingMatches, kErrOneColorPreference,
			kErrWatchNotFound } err;

	string sParam;
};

class CMsgSGFatalTimeout: public CMsg {
public:
	CMsgSGFatalTimeout(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGFatalTimeout();

	string idg, sLogin;
};

class CMsgSGFinger: public CMsg {
public:
	CMsgSGFinger(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGFinger();

	string sLogin;
	map<string, string> keyToValue;
	vector<CSGFingerRating> frs;
};

class CMsgSGHistory: public CMsg {
public:
	CMsgSGHistory(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGHistory();

	u4 n;
	string sLogin;
	vector<CSGHistoryItem> his;
};

template<class TRules>
class CMsgSGJoin: public CMsg {
public:
	CMsgSGJoin(const string& sMsg, const string& asFrom, istream& is, CSGBase* pSg);
	virtual ~CMsgSGJoin();

	string idg;
	typename TRules::TGame game;
};

template<class TRules>
class CMsgSGLook: public CMsg {
public:
	CMsgSGLook(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGLook();

	u4 nGames;
	vector<typename TRules::TGame> games;
};

class CMsgSGMatch: public CMsg {
public:
	CMsgSGMatch(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGMatch();

	u4 n1, n2;
	vector<CSGMatch> matches;
};

class CMsgSGMatchDelta: public CMsg {
public:
	CMsgSGMatchDelta(const string& sMsg, const string& asFrom, istream& is, bool fPlus);
	virtual ~CMsgSGMatchDelta();

	bool fPlus;
	CSGMatch match;
	// '- match' messages only
	CSGResult result;
};

class CMsgSGRank: public CMsg {
public:
	CMsgSGRank(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGRank();

	CSGRating rating;
	u4 n;	// what is this?
	vector<CSGRankData> rds;
};

// /os: +  .19 1735.1 pamphlet 15:00//02:00        8 U 1345.6 ant
template<class TRules>
class CMsgSGRequestDelta: public CMsg {
public:
	CMsgSGRequestDelta(const string& sMsg, const string& asFrom, istream& is, bool fPlus);
	virtual ~CMsgSGRequestDelta();

	bool fPlus;
	string idr;
	CSGRequest<TRules> request;

	bool IAmChallenged(CSGBase* pSg, bool fIncludeGenericRequests=false) const;
	bool IAmChallenging(CSGBase* pSg) const;
	bool RequireBoardSize(u4 n, CSGBase* pSg, bool fRespond=true) const;
	bool RequireAnti(bool fAnti, CSGBase* pSg, bool fRespond=true) const;
	bool RequireRand(bool fRand, CSGBase* pSg, bool fRespond=true) const;
	bool RequireRated(bool fRated, CSGBase* pSg, bool fRespond=true) const;
	bool RequireColor(const char* legal, CSGBase* pSg, bool fRespond=true) const;
	bool RequireEqualClocks(CSGBase* pSg, bool fRespond=true) const;
	bool RequireMaxOpponentClock(const CSGClock& ck, CSGBase* pSg, bool fRespond=true) const;
	bool RequireMinMyClock(const CSGClock& ck, CSGBase* pSg, bool fRespond=true) const;
	bool RequireRandDiscs(u4 nMin, u4 nMax, CSGBase* pSg, bool fRespond=true) const;
};

class CMsgSGStored: public CMsg {
public:
	CMsgSGStored(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGStored();

	u4 nStored;
	string sLogin;
	vector<CSGStoredMatch> sms;
};

template<class TRules>
class CMsgSGUpdate: public CMsg {
public:
	CMsgSGUpdate(const string& sMsg, const string& asFrom, istream& is, bool afFromViewer);
	virtual ~CMsgSGUpdate();

	string idg;
	CSGMoveListItem mli;
	bool fFromViewer, fSynch;

	CMsgSGUpdate();
};

// /os: rating_update .21
// |pamphlet 1720.00 @ 350.00  +316.06 -> 2036.06 @ 254.20
// |ant      1697.62 @ 113.02   -43.23 -> 1654.39 @ 110.44

template<class TRules>
class CMsgSGRatingUpdate: public CMsg {
public:
	CMsgSGRatingUpdate(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGRatingUpdate();

	string idm;
	string sPlayers[2];
	CSGRating rOlds[2], rNews[2];
	double dDeltas[2];
};

template<class TRules>
class CMsgSGTDStart: public CMsg {
public:
	CMsgSGTDStart(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGTDStart();

	string idtd, idm;
};

template<class TRules>
class CMsgSGTimeout: public CMsg {
public:
	CMsgSGTimeout(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGTimeout();

	string idg, sLogin;
};

template<class TRules>
class CMsgSGTop: public CMsg {
public:
	CMsgSGTop(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGTop();

	CSGRating rating;
	u4 n;	// what is this?
	vector<CSGRankData> rds;
};

// /os: trust-violation .56 pamphlet (*) delta= 4 + 0.7 secs
template<class TRules>
class CMsgSGTrustViolation : public CMsg {
public:
	CMsgSGTrustViolation(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGTrustViolation();

	string idg, sLogin;
	char cColor;
	double delta1, delta2;
};

// /os: undo .7 n3 is asking
template<class TRules>
class CMsgSGUndoRequest: public CMsg {
public:
	CMsgSGUndoRequest(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGUndoRequest();

	string idg, sLogin;
};

template<class TRules>
class CMsgSGUnknown: public CMsg {
public:
	CMsgSGUnknown(const string& sMsg, const string& asFrom, istream& is, const string& asFromType);
	virtual ~CMsgSGUnknown();

	string sMsgType;
	string sText;
};


// /os: watch 1 : .41(1)
template<class TRules>
class CMsgSGWatch: public CMsg {
public:
	CMsgSGWatch(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGWatch();

	u4 nMatches;
	map<string, u4> idToNWatchers;
};

// /os:  + n3 watch .44
template<class TRules>
class CMsgSGWatchDelta: public CMsg {
public:
	CMsgSGWatchDelta(const string& sMsg, const string& asFrom, istream& is, bool fPlus, const string& sLogin);
	virtual ~CMsgSGWatchDelta();

	bool fPlus;
	string sLogin, idm;
};

/*
/os: who    26      change:    win    draw    loss         match(es)
|tit4tat  + 1145.7@301.5 ->   +14.3   -12.1   -38.4 @ 79.6 
|n2       + 1384.1@ 80.5 
|ant      + 1426.2@ 50.8 ->   +39.2    +4.2   -30.8 @ 78.5 
etc.
*/

template<class TRules>
class CMsgSGWho: public CMsg {
public:
	CMsgSGWho(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgSGWho();

	u4 n;
	vector<CSGWhoItem> wis;
};

#include "SGMessages_T.h"

#endif //GDK_SGMessage_H
