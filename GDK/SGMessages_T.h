// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "types.h"

#include "SGMessages.h"
#include "ggsstream.h"

#include <strstream>
using namespace std;

///////////////////////////////////
// CMsgSGRatingUpdate
///////////////////////////////////

template <class TRules>
CMsgSGRatingUpdate<TRules>::~CMsgSGRatingUpdate() {
}

// /os: rating_update .21
// |pamphlet 1720.00 @ 350.00  +316.06 -> 2036.06 @ 254.20
// |ant      1697.62 @ 113.02   -43.23 -> 1654.39 @ 110.44


template <class TRules>
CMsgSGRatingUpdate<TRules>::CMsgSGRatingUpdate(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	string s;

	is >> idm;
	is >> sPlayers[0] >> rOlds[0] >> dDeltas[0] >> s >> rNews[0];
	QSSERT(s=="->");
	is >> sPlayers[1] >> rOlds[1] >> dDeltas[1] >> s >> rNews[1];
	QSSERT(s=="->");
}

///////////////////////////////////
// CMsgSGTDStart
///////////////////////////////////

template <class TRules>
CMsgSGTDStart<TRules>::~CMsgSGTDStart() {
}

template <class TRules>
CMsgSGTDStart<TRules>::CMsgSGTDStart(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	is >> idtd >> idm;
}

///////////////////////////////////
// CMsgSGTimeout
///////////////////////////////////

template <class TRules>
CMsgSGTimeout<TRules>::~CMsgSGTimeout() {
}

template <class TRules>
CMsgSGTimeout<TRules>::CMsgSGTimeout(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	is >> idg >> sLogin >> ws;
}


///////////////////////////////////
// CMsgSGTop
///////////////////////////////////

template <class TRules>
CMsgSGTop<TRules>::~CMsgSGTop() {
}

/*
/os: top       1480.3@353.7      Inactive       AScore    Win   Draw   Loss 1308
|    1 leaflet  2103.9@ 33.8=   1.07:08:24+@ 31.3  +4.5    551     65    395
|    2 OO7      2096.5@ 33.4=     03:39:07+@ 33.1  +7.2  18943   1432  12866
|    3 HrtBrev  [recv]2496.5@119.2= 303.07:25:27+@ 69.4  +3.5    888     83    477
|    4 piglet   2441.4@114.4= 641.23:07:55+@ 31.1  +8.7   2987    132    629
|    5 zeclipse 2534.2@137.1=2037.01:42:33+@ 48.0 +10.2    237     26     81
|    6 slarin   2489.5@132.8= 708.13:1[recv]2:33+@ 71.7 +11.1    118     11     12
|    7 hannibal 2547.2@147.3= 644.01:58:35+@ 97.8  +8.8    473     52    133
|    8 booklet  2328.2@107.3= 316.04:20:44+@ 44.6  +5.7    350     47    121
|    9 scorpion 1945.6@ 32.0=   1.07:11:41+@ 29.3 +13.6  215[recv]67   1052  11066
|   10 Baboo    2325.6@116.0=  82.22:30:32+@ 91.0  +6.5    141     11     21
|   11 turtle   2338.0@115.0= 537.15:14:12+@ 41.9  +8.2   2015    320    518
|   12 scorp+   2009.6@ 51.4=   1.07:10:25+@ 49.8 +12.6  12298    809   7658
|   [recv]13 pvs+     2321.8@118.3= 371.08:47:33+@ 62.3  +6.7    148     10     19
|   14 rapsac   2425.5@142.0=1545.01:39:42+@ 68.7  +5.5    198     21    123
|   15 eclipse  2444.5@148.5=1441.22:22:07+@ 83.0  +5.4    619     71    394
|   16 doronko  2398.8@141[recv].5= 519.07:30:15+@ 93.2  -0.3    937    175    817
*/

template <class TRules>
CMsgSGTop<TRules>::CMsgSGTop(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	string sInactive, sAScore, sWin, sDraw, sLoss;
	CSGRankData rd;

	is >> rating >> sInactive >> sAScore >> sWin >> sDraw >> sLoss >> n >> ws;
	while (is >> rd) {
		rds.push_back(rd);
	}
}

///////////////////////////////////
// CMsgSGTrustViolation
///////////////////////////////////

template <class TRules>
CMsgSGTrustViolation<TRules>::~CMsgSGTrustViolation() {
}

// /os: trust-violation .56 pamphlet (*) delta= 4 + 0.7 secs
template <class TRules>
CMsgSGTrustViolation<TRules>::CMsgSGTrustViolation(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	char c1, c2;
	string sDelta, sSecs;

	is >> idg >> sLogin >> c1 >> cColor >> c2;
	QSSERT(c1=='(');
//	QSSERT(cColor==CSGBoard::BLACK || cColor==CSGBoard::WHITE || cColor==CSGBoard::UNKNOWN);
	QSSERT(c2==')');

	is >> sDelta >> delta1 >> c1 >> delta2 >> sSecs;
	QSSERT(sDelta=="delta=");
	QSSERT(sSecs=="secs");
}

///////////////////////////////////
// CMsgSGUndoRequest
///////////////////////////////////

template <class TRules>
CMsgSGUndoRequest<TRules>::~CMsgSGUndoRequest() {
}

template <class TRules>
CMsgSGUndoRequest<TRules>::CMsgSGUndoRequest(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	string sDummy;

	is >> idg >> sLogin >> ws;
	getline(is, sDummy);

	QSSERT(sDummy=="is asking");
}

///////////////////////////////////
// CMsgSGUnknown
///////////////////////////////////

template <class TRules>
CMsgSGUnknown<TRules>::CMsgSGUnknown(const string& sMsg, const string& asFrom,
							 istream& is, const string& asMsgType) : CMsg(sMsg, asFrom) {
	sMsgType=asMsgType;
	getline(is, sText, (char)EOF);
}

template <class TRules>
CMsgSGUnknown<TRules>::~CMsgSGUnknown() {
}

///////////////////////////////////
// CMsgSGWatch
///////////////////////////////////

template <class TRules>
CMsgSGWatch<TRules>::~CMsgSGWatch() {
}

// /os: watch 1 : .41(1)
template <class TRules>
CMsgSGWatch<TRules>::CMsgSGWatch(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	char c1, c2;
	string idm;
	u4 nWatchers;

	is >> nMatches >> c1;
	QSSERT(c1==':');

	while (is >> idm >> c1 >> nWatchers >> c2) {
		QSSERT(c1=='(');
		QSSERT(c2==')');
		QSSERT(idToNWatchers.find(idm)==idToNWatchers.end());
		idToNWatchers[idm]=nWatchers;
	}
}

///////////////////////////////////
// CMsgSGWatchDelta
///////////////////////////////////

// /os:  + n3 watch .44
template <class TRules>
CMsgSGWatchDelta<TRules>::CMsgSGWatchDelta(const string& sMsg, const string& asFrom, istream& is, bool afPlus, const string& asLogin) : CMsg(sMsg, asFrom) {
	fPlus=afPlus;
	sLogin=asLogin;
	is >> idm;
}

template <class TRules>
CMsgSGWatchDelta<TRules>::~CMsgSGWatchDelta() {
}

///////////////////////////////////
// CMsgSGWho
///////////////////////////////////

template <class TRules>
CMsgSGWho<TRules>::~CMsgSGWho() {
}

/*
/os: who    26      change:    win    draw    loss         match(es)
|tit4tat  + 1145.7@301.5 ->   +14.3   -12.1   -38.4 @ 79.6 
|n2       + 1384.1@ 80.5 
|ant      + 1426.2@ 50.8 ->   +39.2    +4.2   -30.8 @ 78.5 
etc.
*/

template <class TRules>
CMsgSGWho<TRules>::CMsgSGWho(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	string s;
	is >> n;
	getline(is, s);

	CSGWhoItem wi;
	while (is >> wi) {
		wis.push_back(wi);
	}

	// This assert fails because GGS sends the wrong n
	//	QSSERT(wis.size()==n);
}

///////////////////////////////////
// CMsgSGJoin
///////////////////////////////////

template <class TRules>
CMsgSGJoin<TRules>::~CMsgSGJoin() {
}

template <class TRules>
CMsgSGJoin<TRules>::CMsgSGJoin(const string& sMsg, const string& asFrom, istream& is, CSGBase* pSg) : CMsg(sMsg, asFrom) {
	is >> idg >> ws >> game;
	game.SetService(pSg,idg);
}

///////////////////////////////////
// CMsgSGLook
///////////////////////////////////

template <class TRules>
CMsgSGLook<TRules>::~CMsgSGLook() {
}

template <class TRules>
CMsgSGLook<TRules>::CMsgSGLook(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	is >> nGames;
	QSSERT(nGames==1 || nGames==2);

	games.reserve(nGames);
	typename TRules::TGame game;
	u4 i;
	for (i=0; i<nGames; i++) {
		game.In(is);
		games.push_back(game);
	}
}

///////////////////////////////////
// CMsgSGRequestDelta
///////////////////////////////////

// /os: +  .19 1735.1 pamphlet 15:00//02:00        8 U 1345.6 ant
template <class TRules>
CMsgSGRequestDelta<TRules>::CMsgSGRequestDelta(const string& sMsg, const string& asFrom, istream& is, bool afPlus) : CMsg(sMsg, asFrom) {
	fPlus=afPlus;
	is >> idr;
	request.In(is);
}

template <class TRules>
CMsgSGRequestDelta<TRules>::~CMsgSGRequestDelta() {
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::IAmChallenged(CSGBase* pSg, bool fIncludeGeneric) const {
	return request.pis[1].sName==pSg->pgs->GetLogin();
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::IAmChallenging(CSGBase* pSg) const {
	return request.pis[0].sName==pSg->pgs->GetLogin();
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::RequireBoardSize(u4 n, CSGBase* pSg, bool fRespond) const {
	bool fOK= request.mt.bt.nWidth==n && request.mt.bt.nHeight==n;
	if (!fOK && fRespond)
		(*pSg->pgs) << "t " << request.pis[0].sName
		<< " I only play on an " << n << "x" << n << " board\n";
	return fOK;
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::RequireAnti(bool fAnti, CSGBase* pSg, bool fRespond) const {
	bool fOK=request.mt.fAnti==fAnti;
	if (!fOK && fRespond)
		(*pSg->pgs) << "t " << request.pis[0].sName << " I " << (fAnti?"only":"don't") << " play anti games\n";
	return fOK;
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::RequireColor(const char* legal, CSGBase* pSg, bool fRespond) const {
	bool fOK= strchr(legal, request.mt.cColor)!=NULL;
	if (!fOK && fRespond) {
		(*pSg->pgs) << "t " << request.pis[0].sName << " I only play with color one of: ";
		if (strchr(legal,'s')!=NULL)
			(*pSg->pgs) << "synch ";
		if (strchr(legal,'k')!=NULL)
			(*pSg->pgs) << "komi ";
		if (strchr(legal,'b')!=NULL)
			(*pSg->pgs) << "black ";
		if (strchr(legal,'w')!=NULL)
			(*pSg->pgs) << "white ";
		if (strchr(legal,'?')!=NULL)
			(*pSg->pgs) << "random ";
		(*pSg->pgs) << "\n";
	}
	return fOK;
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::RequireRand(bool fRand, CSGBase* pSg, bool fRespond) const {
	bool fOK=request.mt.fRand==fRand;
	if (!fOK && fRespond)
		(*pSg->pgs) << "t " << request.pis[0].sName << " I " << (fRand?"only":"don't") << " play rand games\n";
	return fOK;
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::RequireRated(bool fRated, CSGBase* pSg, bool fRespond) const {
	bool fOK=(request.cType!='U')==fRated;
	if (!fOK && fRespond)
		(*pSg->pgs) << "t " << request.pis[0].sName << " I " << (fRated?"only":"don't") << " play rated games\n";
	return fOK;
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::RequireEqualClocks(CSGBase* pSg, bool fRespond) const {
	bool fOK=request.cks[0]==request.cks[1];
	if (!fOK && fRespond)
		(*pSg->pgs) << "t " << request.pis[0].sName << " I only play games with equal clocks\n";
	return fOK;
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::RequireMaxOpponentClock(const CSGClock& ck, CSGBase* pSg, bool fRespond) const {
	bool fOK=request.cks[0]<=ck;
	if (!fOK && fRespond)
		(*pSg->pgs) << "t " << request.pis[0].sName << " Your clock can be at most " << ck << "\n";
	return fOK;
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::RequireMinMyClock(const CSGClock& ck, CSGBase* pSg, bool fRespond) const {
	bool fOK=request.cks[1]>=ck;
	if (!fOK && fRespond)
		(*pSg->pgs) << "t " << request.pis[0].sName << " My clock must be at least " << ck << "\n";
	return fOK;
}

template <class TRules>
bool CMsgSGRequestDelta<TRules>::RequireRandDiscs(u4 nMin, u4 nMax, CSGBase* pSg, bool fRespond) const {
	bool fOK= !request.mt.fRand || (request.mt.nRandDiscs>=nMin && request.mt.nRandDiscs<=nMax);
	if (!fOK && fRespond)
		(*pSg->pgs) << "t " << request.pis[0].sName
		<< " In a rand game, # of rand discs must be between " << nMin << " and " << nMax << "\n";
	return fOK;
}

///////////////////////////////////
// CMsgSGUpdate
///////////////////////////////////

template <class TRules>
CMsgSGUpdate<TRules>::~CMsgSGUpdate() {
}

template <class TRules>
CMsgSGUpdate<TRules>::CMsgSGUpdate(const string& sMsg, const string& asFrom, istream& is, bool afFromViewer) : CMsg(sMsg, asFrom) , fFromViewer(afFromViewer) {
	is >> idg >> mli;
}

template <class TRules>
CMsgSGUpdate<TRules>::CMsgSGUpdate() : CMsg("", "") {
}

