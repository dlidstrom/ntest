// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "SGObjects.h"
#include "OsObjects.h"
#include "CksObjects.h"
#include "SGBase.h"
#include "ggsstream.h"

#include <iomanip>
#include <string>
#include <iostream>
#include <strstream>
#include <time.h>
#include <math.h>

/////////////////////////////////////
// CIdGame
/////////////////////////////////////
CIdGame::CIdGame(const std::string& idg) {
	u4 pos;

	if (idg.empty())
		pos=idg.npos;
	else
		pos=idg.find('.',1);

	if (pos==idg.npos)
		idm=idg;
	else {
		idm.assign(idg,0,pos);
		idmg.assign(idg,pos,idg.size());
	}
}

CIdGame::operator std::string() const {
	return idm+idmg;
}

int CIdGame::NIdmg() const {
   int result;

   u4 pos=idmg.find_first_of("0123456789");
   if (pos==idmg.npos)
      result = 0;
   else {
      std::istrstream is(idmg.c_str()+pos);
      is >> result;
   }
   return result;
}

/////////////////////////////////////
// CSGBoardBase
/////////////////////////////////////

int CSGBoardBase::PieceCount(char c) const {
	string::const_iterator pc;
	int nCount;

	nCount=0;
	for (pc=sBoard.begin(); pc!=sBoard.end(); pc++)
		if (*pc==c)
			nCount++;

	return nCount;
}

CSGBoardBase::~CSGBoardBase() {
}

/////////////////////////////////////
// CSGBoardType
/////////////////////////////////////

string CSGBoardTypeBase::StringGet() const {
	ostrstream os;
	Out(os);
	os << '\0';
	string sResult(os.str());
	os.rdbuf()->freeze(0);
	return sResult;
}

void CSGBoardTypeBase::StringSet(const string& sMatchType) {
	istrstream is(sMatchType.c_str());
	In(is);
}

bool CSGBoardTypeBase::operator==(const CSGBoardTypeBase& b) const {
	return nOcto==b.nOcto && nWidth==b.nWidth && nHeight==b.nHeight;
}

void CSGBoardTypeBase::Clear() {
	nWidth=nHeight=0;
	nOcto=0;
}

int CSGBoardTypeBase::NRandDiscsMax() const {
	// formula only works for boards with nOcto<=3 and nWidth>2*(nOcto+1)
	return nWidth*nHeight-16*(nOcto+1);
}

int CSGBoardTypeBase::NRandDiscsMin() const {
	return 4;
}

int CSGBoardTypeBase::NPlayableSquares() const {
	return nWidth*nHeight-2*nOcto*(nOcto+1);
}

int CSGBoardTypeBase::NTotalSquares() const {
	return (nWidth+2)*(nHeight+2);
}

bool CSGBoardTypeBase::DummyCorner(int x, int y) const {
	if (nOcto) {
		int xMirror=nWidth-1-x;
		int yMirror=nHeight-1-y;
		
		if (xMirror<x)
			x=xMirror;
		if (yMirror<y)
			y=yMirror;

		return (x+y)<nOcto;
	}
	else
		return false;
}

string CSGBoardTypeBase::Description() const {
	ostrstream os;
	string sResult;

	os << nWidth << 'x' << nHeight;
	if (nOcto)
		os << " Octagon";
	os << '\0';
	sResult=os.str();
	os.rdbuf()->freeze(0);

	return sResult;
}

/////////////////////////////////////////
// CSGClock
/////////////////////////////////////////

CSGClock::CSGClock() {
}

CSGClock::CSGClock(double atCurrent, double atIncrement, double atGrace, int aiTimeout) {
	tCurrent=atCurrent;
	tIncrement=atIncrement;
	tGrace=atGrace;
	iTimeout=aiTimeout;
}

void CSGClock::In(istream& is) {
	string s;
	if (is >> s)
		In(s.c_str());
}

void CSGClock::In(const char* s) {
	istrstream is(s);

	Clear();

	tCurrent=ReadTime(is);

	if (is.peek()=='/') {
		is.ignore(1);
		// 0 is passed as a blank spot
		if (is.peek()=='/')
			tIncrement=0;
		else 
			tIncrement=ReadTime(is);
	}

	if (is.peek()=='/') {
		is.ignore(1);
		tGrace=ReadTime(is);
	}
}

istream& CSGClock::InIOS(istream& is) {
	char c;

	is >> c;
	QSSERT(c=='(');
	is >> tCurrent;
	tCurrent*=60;
	is >> tIncrement;
	is >> tGrace;
	tGrace*=60;
	is >> c;
	QSSERT(c==')');

	return is;
}

void CSGClock::Out(ostream& os) const {
	WriteTime(os, int(tCurrent));
	os << "/";
	if (tIncrement)
		WriteTime(os, int(tIncrement));
	os << "/";
	WriteTime(os, int(tGrace));
}

void CSGClock::Clear() {
	iTimeout=0;
	tCurrent=tIncrement=0;
	tGrace=120;
}

void CSGClock::Update(double tElapsed, bool fIncludeIncrement) {
	tCurrent-=tElapsed;

	// adjust for timeouts and grace periods
	if (tCurrent<0) {
		if (iTimeout==0) {
			tCurrent+=tGrace;
			iTimeout=1;
		}
		if (tCurrent<0) {
			tCurrent=0;
			iTimeout=2;
		}
	}
	if (fIncludeIncrement && iTimeout<2)
		tCurrent+=tIncrement;
}

double CSGClock::ReadTime(istream& is) {
	int n[4], i;
	char c;
	double t;

	// parse into numbers
	// format is days.hours:minutes:seconds
	// leading 0s can be skipped
	for (i=0; (i<4) && (is>>n[i]); ) {
		i++;
		c=char(is.peek());
		QSSERT(c!='.' || i==1);
		if (c=='.' || c==':')
			is.ignore(1);
		else
			break;
	}

	// This can fail because the stream is bad, in which case we just return 0
	if (!i)
		return 0;

	// turn it into seconds
	i--;
	t=n[i];
	if (i--) {
		t+=n[i]*60;
		if (i--) {
			t+=n[i]*60*60;
			if (i--) {
				t+=n[i]*24*60*60;
			}
		}
	}

	return t;
}

void CSGClock::WriteTime(ostream& os, int nSeconds) {

	int nMinutes=nSeconds/60;
	nSeconds%=60;
	if (nMinutes>=60) {
		int nHours=nMinutes/60;
		nMinutes%=60;
		os << nHours << ":" << setw(2) << setfill('0') << nMinutes << ":";
	}
	else
		os << nMinutes << ":";
	os << setw(2) << setfill('0') << nSeconds;

	os <<  setfill(' ');
}

bool CSGClock::operator==(const CSGClock& b) const {
	return iTimeout==b.iTimeout &&
		tCurrent==b.tCurrent &&
		tGrace==b.tGrace &&
		tIncrement==b.tIncrement;
}

bool CSGClock::operator!=(const CSGClock& b) const {
	return !(*this==b);
}

bool CSGClock::operator<=(const CSGClock& b) const {
	return iTimeout<=b.iTimeout &&
		tCurrent<=b.tCurrent &&
		tGrace<=b.tGrace &&
		tIncrement<=b.tIncrement;
}

bool CSGClock::operator>=(const CSGClock& b) const {
	return iTimeout>=b.iTimeout &&
		tCurrent>=b.tCurrent &&
		tGrace>=b.tGrace &&
		tIncrement>=b.tIncrement;
}

bool CSGClock::operator<(const CSGClock& b) const {
	return (*this)!=b &&
		iTimeout<=b.iTimeout &&
		tCurrent<=b.tCurrent &&
		tGrace<=b.tGrace &&
		tIncrement<=b.tIncrement;
}

bool CSGClock::operator>(const CSGClock& b) const {
	return (*this)!=b &&
		iTimeout>=b.iTimeout &&
		tCurrent>=b.tCurrent &&
		tGrace>=b.tGrace &&
		tIncrement>=b.tIncrement;
}

bool CSGClock::EqualsToNearestSecond(const CSGClock& b) const {
	return iTimeout==b.iTimeout &&
		(int)tCurrent==(int)b.tCurrent &&
		(int)tGrace==(int)b.tGrace &&
		(int)tIncrement==(int)b.tIncrement;
}

/////////////////////////////////////
// CSGDateTime
/////////////////////////////////////

void CSGDateTime::In(istream& is) {
	is >> day >> sMonth >> year >> sTime;
}

string CSGDateTime::Text() const {
	ostrstream os;
	os << setw(4) << year << "-" << sMonth << "-" << day << " " << sTime << '\0';
	string result(os.str());
	os.rdbuf()->freeze(0);

	return result;
}

/////////////////////////////////
// CSGFingerRating
/////////////////////////////////

void CSGFingerRating ::In(istream& is) {
	is >> rt >> rd;
}

/////////////////////////////////
// CSGGameBase
/////////////////////////////////

CSGGameBase::CSGGameBase() {
	pSg=NULL;
	fSynchronized=true;
}

CSGGameBase* CSGGameBase::NewFromStream(istream& is) {
	string sGameName=InGameName(is);
	if (sGameName=="Reversi")
		return new COsGame(is);
	else if (sGameName=="Checkers")
		return new CCksGame(is);
	else {
		QSSERT(0);
		return NULL;
	}
}

void CSGGameBase::SetTime(time_t t) {
	// sDateTime
	char sTime[30];
	strftime(sTime, 30, "%Y-%m-%d %H:%M:%S GMT", gmtime(&t));
	sDateTime=sTime;
}

void CSGGameBase::SetService(CSGBase* apSg, const string& aidg) {
	pSg=apSg;
	idg=aidg;
	sLogin=pSg->pgs->GetLogin();
}

void CSGGameBase::SetCurrentTime() {
	time_t tCurrent=time(NULL);
	SetTime(tCurrent);
}

// set the game result. sNames[0] is the name of the first player listed by /os,
//	i.e. the challenger. The game result is given from his point of view in an End
//	message for synchro games
void CSGGameBase::SetResult(const CSGResult& aresult, const string sName) {
	result=aresult;
	if (pis[0].sName!=sName)
		result.dResult=-result.dResult;
}

// MatchDelta messages, on the other hand, list the game result properly
void CSGGameBase::SetResult(const CSGResult& aresult) {
	result=aresult;
}

string CSGGameBase::InGameName(istream& is) {
	char c;
	string sGameName;

	bool fOK = (is >> c) && (c=='(') && (is >> c) && (c==';');
	if (!fOK && is)
		is.setstate(ios::failbit);

	if (is) {
		string sToken;

		getline(is, sToken, '[');
		getline(is, sGameName, ']');

		QSSERT(sToken=="GM");
		if (sGameName=="Othello")
			sGameName="Reversi";
	}

	return sGameName;
}

void CSGGameBase::In(istream& is) {
	InGameName(is);
	if (is)
		InPartial(is);
}

bool CSGGameBase::IsSynchronized() const {
	return fSynchronized;
}

bool CSGGameBase::IsMyMove() const {
	if (pSg==NULL)
		return false;
	if (GameOver())
		return false;
	else if (NeedsKomi())
		return sLogin==pis[0].sName || sLogin==pis[1].sName;
	else
		return sLogin==pis[Pos().Board().iMover].sName;
}

const string& CSGGameBase::Idg() const {
	return idg;
}

CSGBase* CSGGameBase::PSg() const {
	return pSg;
}

bool CSGGameBase::IWasPlaying() const {
	return sLogin==pis[0].sName || sLogin==pis[1].sName;
}

bool CSGGameBase::IAmPlaying() const {
	if (pSg==NULL)
		return false;
	else
		return IWasPlaying();
}

bool CSGGameBase::IAmPlaying(int iMover) const {
	if (pSg==NULL)
		return false;
	else
		return sLogin==pis[iMover].sName;
}

double CSGGameBase::TimeSinceLastMove() const {
	double tT, tC;

	// clock() time is more accurate but may roll over. If it is close to time() time
	//	send the clock() time
	tT=time(NULL)-tLastMove;
	tC=double(clock()-clockLastMove)/CLOCKS_PER_SEC;
	if (fabs(tT-tC)<2)
		return tC;
	else
		return tT;
}

void CSGGameBase::SetLastMoveTime() {
	tLastMove=time(NULL);
	clockLastMove=clock();
}

int CSGGameBase::ViewerComment(const string& sComment) {
	if (pSg) {
		pSg->Tell() << "tell " << idg << " " << sComment << "\n" << flush;
		string comment;
		comment=pSg->pgs->GetLogin();
		comment+=": ";
		comment+=sComment;
		ServiceComment(sComment);
	}
	else
		QSSERT(0);

	return 0;
}

string CSGGameBase::GetLogin() const {
	if (pSg)
		return pSg->pgs->GetLogin();
	else
		return "";
}

// viewer actions
int CSGGameBase::WatchEnd() {
	if (pSg && !IsSynchro())
		pSg->WatchEnd(idg);

	return 0;
}

void CSGGameBase::ServiceComment(const string& sComment) {
}

void CSGGameBase::Kill() {
	pSg=NULL;
}

/*
void CSGGameBase::ViewerResign() const {
	if (pSg)
		pSg->Tell() << "resign " << idg << "\n" << flush;
	else
		QSSERT(0);
}

*/

bool CSGGameBase::ToMove(const string& sLogin) const {
	if (NeedsKomi())
		return pis[0].sName==sLogin || pis[1].sName==sLogin;
	else
		return pis[Pos().Board().iMover].sName==sLogin;
}

/////////////////////////////////////////////
// CSGHistoryItem
/////////////////////////////////////////////

// .48723   25 Mar 2001 03:28:00 1780 nasai    2144 OO7        -4.0 8
void CSGHistoryItem::In(istream& is) {
	is >> idsm >> dt >> pis[0] >> pis[1] >> result >> sMatchType;
}

/////////////////////////////////////////////
// CSGMatch
/////////////////////////////////////////////

istream& CSGMatch::InDelta(istream& is) {
	is >> idm >> pis[0] >> pis[1] >> sMatchType >> cRated;
	nObservers=0;

	return is;
}

// |  .9   s8r20  R 2574 lynx     2570 kitty    2
istream& CSGMatch::In(istream& is) {
	is >> idm;
	is >> pis[0];
	is >> pis[1];
	is >> sMatchType;
	is >> cRated;
	is >> nObservers;

	return is;
}

bool CSGMatch::IsPlaying(const string& sLogin) const {
	return pis[0].sName==sLogin || pis[1].sName==sLogin;
}

/////////////////////////////////////
// CSGMatchTypeBase
/////////////////////////////////////

CSGMatchTypeBase::operator string() const {
	ostrstream os;
	Out(os);
	string s(os.str());
	os.rdbuf()->freeze(0);
	return s;
}

void CSGMatchTypeBase::StringSet(const string& s) {
	istrstream is(s.c_str());
	In(is);
}

//////////////////////////////////
// CSGMove
//////////////////////////////////

istream& CSGMove::In(istream& is) {
	erase();
	is >> ws;
	int c;
	char cStrip;

	while (c=is.peek(), c!=EOF && !iswspace(c) && c!=']' && c!='/') {
		is >> cStrip;
		operator+=(c);
	}

	return is;
}

//////////////////////////////////
// CSGMoveList
//////////////////////////////////

void CSGMoveList::Update(const CSGMoveListItem& mli) {
	push_back(mli);
}

//////////////////////////////////
// CSGMoveListItem
//////////////////////////////////

// peek without making is bad at EOF
inline int SafePeek(istream& is) {
	if (!is)
		return EOF;
	int c=is.peek();

	// ignore EOF due to peek
	if (is.fail()&&!is.bad())
		is.clear();

	return c;
}

// helper function for In
inline void IgnoreAlnum(istream& is) {
	while (isalnum(SafePeek(is)))
		is.ignore(1);
}

void CSGMoveListItem::In(istream& is) {
	// move
	is >> mv;
	if (!is)
		return;

	IgnoreAlnum(is);

	// eval
	dEval=0;
	if (SafePeek(is)=='/') {
		is.ignore(1);
		is >> dEval;

		// 0 is passed as a blank spot, so ignore errors here
		if (is.fail()&&!is.bad())
			is.clear();
	}

	tElapsed=0;
	if (SafePeek(is)=='/') {
		is.ignore(1);
		is >> tElapsed;
	}
}

void CSGMoveListItem::Out(ostream& os) const {
	int nPrecisionOld=os.precision(2);
	int fFlagsOld=os.setf(ios::fixed, ios::floatfield);

	os << mv;
	if (dEval || tElapsed) {
		os << '/';
		if (dEval)
			os << dEval;
		if (tElapsed)
			os << '/' << tElapsed;
	}

	os.flags(fFlagsOld);
	os.precision(nPrecisionOld);
}

/////////////////////////////////
// CSGPlayerInfo
/////////////////////////////////

void CSGPlayerInfo::In(istream& is) {
	is >> dRating >> sName;
}

void CSGPlayerInfo::Clear() {
	dRating=0;
	sName.erase();
}

/////////////////////////////////
// CSGRankData
/////////////////////////////////

//     1 leaflet  2103.9@ 33.8=   1.07:08:24+@ 31.3  +4.5    551     65    395
void CSGRankData::In(istream& is) {
	string s;
	string sLine;

	getline(is, sLine);
	if (!sLine.empty()) {
		istrstream isLine(sLine.c_str());

		isLine  >> iRank >> sLogin;
		isLine >> rd;
		isLine >> s;
		fMe=s=="<=";
		if (!s.empty() && !fMe) {
			QSSERT(0);
		}
	}
}

void CSGRatingData::In(istream& is) {
	char c;

	if ( is  >> rating >> c) {
		QSSERT(c=='=');
		sInactive="hello";
		getline(is, sInactive, char('+'));
		is >> c;
		QSSERT(c=='@');
		is >> d1 >> d2 >> nWins >> nDraws >> nLosses;
	}
}

/////////////////////////////////
// CSGRating
/////////////////////////////////

void CSGRating::In(istream& is) {
	char c;
	is >> dRating >> c >> dSD;
	QSSERT(c=='@');
}

void CSGRating::Out(ostream& os) const {
	int precision = os.precision(0);
	int flags = os.setf(ios::fixed, ios::floatfield);

	os << setw(4) << dRating << "@" << setw(3) << dSD;

	os.precision(precision);
	os.flags(flags);
}

double CSGRating::AdjustedRating() const {
	double coefficient=3.5; // Kevin Wong tells me this was switched from 1720./350. a while ago 
	return dRating-dSD*coefficient;
}

/////////////////////////////////////
// CSGResult
/////////////////////////////////////

// "nasai left" in matchdelta messages means game adjourned
//	"?" in other messages
void CSGResult::In(istream& is) {
	is >> ws;

	char c;
	c=char(is.peek());
	if (c=='?') {
		is >> c;
		status=kUnfinished;
		dResult=0;
	}
	else if (isalpha(c)) {
		dResult=0;
		string sResult;
		is >> sResult;
		if (sResult=="aborted")
			status=kAborted;
		else if (sResult=="bye")
			status=kBye;
		else if (sResult=="unstarted")
			status=kUnstarted;
		else
			status=kAdjourned;
	}
	else {
		is >> dResult;
		status=kNormalEnd;
		if (is) {
			c=char(is.peek());
			if (c==':') {
				is >> c >> c;
				switch(c) {
				case 'r': status=kResigned; break;
				case 't': status=kTimeout; break;
				case 'l': status=kAdjourned; break;
				default: QSSERT(0);
				}
			}
			else if ((c==-1) && !is.bad()) {
				is.clear();
			}
		}
	}
}

void CSGResult::Out(ostream& os) const {
	switch(status) {
	case kBye:
		os << "bye";
		break;
	case kUnstarted:
		os << "unstarted";
		break;
	case kUnfinished:
	case kAdjourned:
	case kAwaitingResult:
	case kAborted:
		os << '?';
		break;
	case kResigned:
	case kNormalEnd:
	case kTimeout:
	case kAgreedScore:
		os << dResult;
		if (status==kResigned)
			os << ":r";
		else if (status==kTimeout)
			os << ":t";
		break;
	default:
			QSSERT(0);
	}
}

void CSGResult::Set(double adResult, TStatus astatus) {
	dResult=adResult;
	status=astatus;
}

void CSGResult::Clear() {
	status=kUnfinished;
	dResult=0;
}

bool CSGResult::HasScore() const {
	switch(status) {
	case kBye:
	case kUnstarted:
	case kUnfinished:
	case kAdjourned:
	case kAwaitingResult:
	case kAborted:
		return false;
	case kResigned:
	case kNormalEnd:
	case kTimeout:
	case kAgreedScore:
		return true;
	default:
		QSSERT(0);
		return false;
	}
}

bool CSGResult::GameOver() const {
	switch(status) {
	case kUnstarted:
	case kUnfinished:
	case kAwaitingResult:
		return false;
	case kBye:
	case kAdjourned:
	case kResigned:
	case kNormalEnd:
	case kTimeout:
	case kAgreedScore:
	case kAborted:
		return true;
	default:
		QSSERT(0);
		return false;
	}
}

/////////////////////////////////
// CSGSquare
/////////////////////////////////

CSGSquare::CSGSquare() {
}

CSGSquare::CSGSquare(int ax, int ay) : x(ax), y(ay) {
}

CSGSquare::CSGSquare(const string& s) {
	StringSet(s);
}

void CSGSquare::IOSSet(int iosmove) {
	if (iosmove<0)
		iosmove=-iosmove;
	XYSet(iosmove%10-1, iosmove/10-1);
}

void CSGSquare::XYGet(int& ax, int& ay) const {
	ax=x;
	ay=y;
}

void CSGSquare::XYSet(int ax, int ay) {
	x=ax;
	y=ay;
}

string CSGSquare::StringGet() const {
	ostrstream os;

	Out(os);
	os << '\0';
	string sResult(os.str());
	os.rdbuf()->freeze(0);
	return sResult;
}

void CSGSquare::StringSet(const string& s) {
	istrstream is(s.c_str());
	In(is);
}

void CSGSquare::In(istream& is) {
	char c;
	is >> c;
	x=toupper(c)-'A';
	is >> y;
	y--;
}

void CSGSquare::Out(ostream& os) const {
	os << char(x+'A') << y+1;
}

/////////////////////////////////
// CSGStoredMatch
/////////////////////////////////

// |.42590   23 Feb 2001 23:17:39 pamphlet patzer   s8r16:l
void CSGStoredMatch::In(istream& is) {
	string s;

	is >> idsm >> dt >> sPlayers[0] >> sPlayers[1] >> sMatchType;
}

/////////////////////////////////
// CSGWhoItem
/////////////////////////////////

/*
/os: who    26      change:    win    draw    loss         match(es)
|tit4tat  + 1145.7@301.5 ->   +14.3   -12.1   -38.4 @ 79.6 
|n2       + 1384.1@ 80.5 
|ant      + 1426.2@ 50.8 ->   +39.2    +4.2   -30.8 @ 78.5 
etc.
*/

void CSGWhoItem::In(istream& isAll) {
	string sLine;
	getline(isAll, sLine);

	if (!sLine.empty()) {
		istrstream is(sLine.c_str());
		string s;
		char c;

		is >> sLogin >> c >> rating >> s;
		QSSERT(c=='+');
		fMe=!is;
		if (!fMe) {
			QSSERT(s=="->");
			is >> dWin >> dDraw >> dLoss >> c >> dSDNew;
			QSSERT(c=='@');
		}
	}
}

bool CSGWhoItem::operator<(const CSGWhoItem& b) const {
	double ar=rating.AdjustedRating(), br=b.rating.AdjustedRating();
	if (ar!=br)
		return ar>br;
	else
		return sLogin<b.sLogin;
};

bool CSGWhoItem::operator==(const CSGWhoItem& b) const {
	return sLogin==b.sLogin;
};
