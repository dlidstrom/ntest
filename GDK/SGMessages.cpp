// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "SGMessages.h"

CMsgSGAbortRequest::~CMsgSGAbortRequest() {
}

CMsgSGAbortRequest::CMsgSGAbortRequest(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	string sDummy;

	is >> idg >> sLogin >> ws;
	getline(is, sDummy);

	QSSERT(sDummy=="is asking");
}

///////////////////////////////////
// CMsgSGComment
///////////////////////////////////

CMsgSGComment::~CMsgSGComment() {
}

CMsgSGComment::CMsgSGComment(const string& sMsg, const string& asFrom, istream& is, const string& aidg) : CMsg(sMsg, asFrom) {
	idg=aidg;
	string sPretext;

	getline(is, sPretext, ':');
	istrstream isPretext(sPretext.c_str());
	isPretext >> cTo >> pi;
	is >> ws;
	getline(is, sComment);
}

///////////////////////////////////
// CMsgSGEnd
///////////////////////////////////

CMsgSGEnd::~CMsgSGEnd() {
}

// /os: end .2.0 ( pamphlet vs. ant ) +46.00
CMsgSGEnd::CMsgSGEnd(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	char c;
	string s;

	is >> idg >> c;
	QSSERT(c=='(');
	is >> sPlayers[0] >> s >> sPlayers[1] >> c;
	QSSERT(s=="vs.");
	QSSERT(c==')');
	is >> result;
}

///////////////////////////////////
// CMsgSGErr
///////////////////////////////////

CMsgSGErr::~CMsgSGErr() {
}

// /os: error .25 corrupt move
// /os: watch + ERR not found: .3

CMsgSGErr::CMsgSGErr(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	string sText;
	getline(is, sText);
	if (sText=="your request doesn't fit the opponent's formula.")
		err=kErrRequestDoesntFitFormula;
	else if (sText=="board type")
		err=kErrIllegalBoardType;
	else if (sText=="rank <type> [login]")
		err=kErrBadRankMessage;
	else if (sText.find("corrupt move")!=sText.npos)
		err=kErrCorruptMove;
	else if (sText=="Your open value is too low for new matches.")
		err=kErrOpenTooLow;
	else if (sText=="rated variable mismatch.")
		err=kErrRatedMismatch;
	else if (sText=="Rated/unrated mismatch")
		err=kErrRatedMismatch;
	else if (sText=="you are not registered.")
		err=kErrUnregistered;

	else if (sText.find("illegal move") != string::npos)
		err=kErrIllegalMove;
	else if (sText.find("not found") != string::npos)
		err=kErrUserNotFound;
	else if (sText=="Player is not accepting new matches.")
		err=kErrNotAcceptingMatches;
	else if (sText=="only one color preference allowed")
		err=kErrOneColorPreference;
	else if (sText.find("watch + ERR not found:")==0 ||
			sText.find("watch - ERR not found:")==0)
		err=kErrWatchNotFound;
	else
		err=kErrUnknown;
}

///////////////////////////////////
// CMsgSGFatalTimeout
///////////////////////////////////

CMsgSGFatalTimeout::~CMsgSGFatalTimeout() {
}

CMsgSGFatalTimeout::CMsgSGFatalTimeout(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	is >> idg >> sLogin >> ws;
}


///////////////////////////////////
// CMsgSGFinger
///////////////////////////////////

CMsgSGFinger::~CMsgSGFinger() {
}

/*
/os: finger n2
|login  : n2
|dblen  : 100.0 = 1,052 / 1,052
|level  : 2
|open   : 1
|client : +
|trust  : +
|rated  : +
|bell   : -r -p -w -n -ns -nn -nt -ni -nr -nw -ta -to -tp
|vt100  : -
|hear   : +
|play[recv]   : 
|stored (-) : 0
|notify (+) : * 
|track  (-) : 
|ignore (-) : 
|request(-) :  : 
|watch  (+) : 
|accept : 
|decline: 
|Type  Rating@StDev      Inactive       AScore    Win   Draw   Loss
|10    1720.0@350.0= ----------- +@350.0  +0.0      0 [recv]     0      0
|12    1720.0@350.0= ----------- +@350.0  +0.0      0      0      0
|14    1720.0@350.0= ----------- +@350.0  +0.0      0      0      0
|4     1720.0@350.0= ----------- +@350.0  +0.0      0      0      0
|6     1720.0@350.0= ----------- +[recv]@350.0  +0.0      0      0      0
|6a    1720.0@350.0= ----------- +@350.0  +0.0      0      0      0
|6r    1720.0@350.0= ----------- +@350.0  +0.0      0      0      0
|8     1418.5@ 97.9=     00:01:30+@ 97.9 -18.8     12      1     32
|88    1720.0@[recv]350.0= ----------- +@350.0  +0.0      0      0      0
|8a    1720.0@350.0= ----------- +@350.0  +0.0      0      0      0
|8r    1720.0@350.0= ----------- +@350.0  +0.0      0      0      0
*/

CMsgSGFinger::CMsgSGFinger(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	string sLine, sKey, sValue;
	CSGFingerRating fr;

	is >> sLogin >> ws;

	// first section, key : value
	while (getline(is, sLine)) {
		if (!is)
			break;
		if (sLine.find(':')==string::npos)
			break;

		istrstream isLine(sLine.c_str());

		// get key and strip terminal spaces
		getline(isLine, sKey, ':');
		sKey.resize(1+sKey.find_last_not_of(' '));

		// get value. No value means this is the separator row
		//	between the first section and the second section
		isLine >> ws;
		getline(isLine, sValue);

		// insert (key, value) pair
		QSSERT(keyToValue.find(sKey)==keyToValue.end());
		keyToValue[sKey]=sValue;
	}

	// second section, rating info
	while (getline(is, sLine)) {
		istrstream isLine(sLine.c_str());
		isLine >> fr;
		frs.push_back(fr);
	}
}
///////////////////////////////////
// CMsgSGHistory
///////////////////////////////////

CMsgSGHistory::~CMsgSGHistory() {
}

/*
/os: history 8 nasai
|.48723   25 Mar 2001 03:28:00 1780 nasai    2144 OO7        -4.0 8
|.48724   25 Mar 2001 03:38:33 1780 nasai    2143 OO7       -64.0 8
|.48522   27 Mar 2001 07:34:08 1777 nasai    2350 booklet   -64.0 8
|.49128   27 Mar 2001 07:40:57 1776 nasai    2351 booklet   -64.0 8
|.49134   27 Mar 2001 07:50:42 1775 nasai    2352 booklet   -64.0 8
|.49135   27 Mar 2001 07:52:58 1774 nasai    2352 booklet   -64.0 8
|.49138   27 Mar 2001 07:59:14 1774 nasai    2353 booklet   -64.0 8
|.49141   27 Mar 2001 08:03:04 1773 nasai    2354 booklet   -64.0 8
*/

CMsgSGHistory::CMsgSGHistory(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	is >> n >> sLogin;
	CSGHistoryItem hi;
	while (is >> hi)
		his.push_back(hi);
	QSSERT(his.size()==n);
}

///////////////////////////////////
// CMsgSGMatch
///////////////////////////////////

CMsgSGMatch::~CMsgSGMatch() {
}

/*
/os: match 2/2
| .14       8  R 1174 YA       1503 ant+     1
|  .9   s8r20  R 2574 lynx     2570 kitty    2
*/

CMsgSGMatch::CMsgSGMatch(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	char c;

	is >> n1 >> c >> n2;
	QSSERT(c=='/');

	CSGMatch match;
	while (match.In(is))
		matches.push_back(match);

	QSSERT(matches.size()==n2);
}

///////////////////////////////////
// CMsgSGMatchDelta
///////////////////////////////////

// /os: - match .2 1833 nasai 2342 booklet 8 R nasai left
CMsgSGMatchDelta::CMsgSGMatchDelta(const string& sMsg, const string& asFrom, istream& is, bool afPlus) : CMsg(sMsg, asFrom) {
	fPlus=afPlus;
	match.InDelta(is);
	if (!fPlus)
		is >> result;
}

CMsgSGMatchDelta::~CMsgSGMatchDelta() {
}

///////////////////////////////////
// CMsgSGRank
///////////////////////////////////

CMsgSGRank::~CMsgSGRank() {
}

CMsgSGRank::CMsgSGRank(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	string sInactive, sAScore, sWin, sDraw, sLoss;
	CSGRankData rd;

	is >> rating >> sInactive >> sAScore >> sWin >> sDraw >> sLoss >> n >> ws;
	while (is >> rd) {
		rds.push_back(rd);
	}
}

///////////////////////////////////
// CMsgSGStored
///////////////////////////////////

CMsgSGStored::~CMsgSGStored() {
}

// /os: stored 2 pamphlet
// |.42590   23 Feb 2001 23:17:39 pamphlet patzer   s8r16:l
// |.42661   24 Feb 2001 18:08:41 pamphlet ant      s8r16:l

CMsgSGStored::CMsgSGStored(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	CSGStoredMatch sm;
	is >> nStored >> sLogin;
	while (is >> sm) {
		sms.push_back(sm);
	}
	QSSERT(sms.size()==nStored);
}

