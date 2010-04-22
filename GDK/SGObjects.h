// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_SGObjects_H
#define GDK_SGObjects_H

// templatized classes for GGS

#define GG_TEMPLATES 1

#include "types.h"

#include <iostream>
#include <vector>
#include <string>
#include <time.h>

class CIdGame {
public:
	CIdGame() {};
	CIdGame(const string& idg);
	operator string() const;

	bool FSynch() const { return !idmg.empty(); }
	int NIdmg() const;	// returns idmg in numeric format if possible, 0 otherwise

	string idm;		// id of match, e.g. .32
	string idmg;	// id of game in match, e.g. .0 or .1 for synchro games, "" for non-synchro
};

class CSGSquare {
public:
	CSGSquare();
	CSGSquare(const string& s);
	CSGSquare(int ax, int ay);

	operator string() const;

	void In(istream& is);
	void Out(ostream& os) const;
	void StringSet(const string& s);
	string StringGet() const;
	void XYSet(int x, int y);
	void XYGet(int& x, int& y) const;

	// special for reading Othello files
	void IOSSet(int iosmove);

	// Comparison
	bool operator==(const CSGSquare& b) const {return x==b.x&& y==b.y;};
	bool operator<(const CSGSquare& b) const {return (x==b.x)?(y<b.y):(x<b.x);}

	int x,y;
};

inline istream& operator>>(istream& is, CSGSquare& sq) { sq.In(is); return is; }
inline ostream& operator<<(ostream& os, const CSGSquare &sq)  { sq.Out(os); return os; }
inline CSGSquare::operator string() const { return StringGet(); }

class CSGMove: public string {
public:
	istream& In(istream& is);

	CSGMove() {};
	CSGMove(const char* b) : string(b) {};
	CSGMove(string& b) : string(b) {};

	CSGMove& operator=(const char* b) {string::operator=(b); return *this; };
	CSGMove& operator=(const string& b) {string::operator=(b); return *this; };

};

inline istream& operator>>(istream& is, CSGMove& mv) { mv.In(is); return is; }

typedef string CSGRatingType;

class CSGBoardTypeBase {
public:
	virtual ~CSGBoardTypeBase() {};

	int nWidth, nHeight, nOcto;

	int NTotalSquares() const;
	int NPlayableSquares() const;
	int NRandDiscsMin() const;
	int NRandDiscsMax() const;
	bool DummyCorner(int x, int y) const;

	virtual void In(istream& is) = 0;
	virtual void Out(ostream& os) const = 0;

	bool operator==(const CSGBoardTypeBase& b) const;

	string StringGet() const;
	void StringSet(const string& sBoardType);

	string Description() const;

	void Clear();
};

class CSGMatchTypeBase {
public:
	virtual ~CSGMatchTypeBase() {};

	bool fAnti, fRand;
	int nRandDiscs;
	char cColor;
	bool fHasKomiValue;
	double dKomiValue;

	virtual void In(istream& is) = 0;
	virtual void Out(ostream& os) const = 0;

	virtual void StringSet(const string& s);

	operator string() const;

	virtual const CSGBoardTypeBase& BT() const = 0;

	enum {
		kErrRandUnbalanced=0x8500,
		kErrTooFewRandDiscs, kErrTooManyRandDiscs };
};

template<class aTBoardType>
class CSGMatchType : public CSGMatchTypeBase {
public:
	virtual ~CSGMatchType() {};

	typedef aTBoardType TBoardType;

	TBoardType bt;

	virtual string StringGet() const;
	virtual void StringSet(const string& sMatchType);

	string Description() const;
	int Validate() const;
	void Clear();

	const CSGBoardTypeBase& BT() const {return bt; };

	bool operator==(const CSGMatchType& b) const;

	double MaxResult() const;
};

class CSGMoveListItem {
public:
	CSGMove mv;
	double dEval, tElapsed;

	void In(istream& is);
	void Out(ostream& os) const;

	bool operator<(const CSGMoveListItem& b) const { return this < &b; }
	bool operator==(const CSGMoveListItem& b) const { return this==&b; }
};

inline istream& operator>>(istream& is, CSGMoveListItem& mli) { mli.In(is); return is;}
inline ostream& operator<<(ostream& os, const CSGMoveListItem& mli)  { mli.Out(os); return os;}

class CSGClock {
public:
	CSGClock();
	CSGClock(double tCurrent, double tIncrement=0, double tGrace=120, int aiTimeout=0);

	void Update(double tElapsed, bool fIncludeIncrement=true);

	// I/O
	void In(istream& is);
	void In(const char* s);
	void Out(ostream& os) const;
	istream& InIOS(istream& is);

	void Clear();

	bool operator==(const CSGClock& b) const;
	bool operator!=(const CSGClock& b) const;
	bool operator<=(const CSGClock& b) const;
	bool operator>=(const CSGClock& b) const;
	bool operator<(const CSGClock& b) const;
	bool operator>(const CSGClock& b) const;

	bool EqualsToNearestSecond(const CSGClock& b) const;

	double tCurrent, tIncrement, tGrace;
	int iTimeout;

protected:
	static double ReadTime(istream& is);
	static void WriteTime(ostream& os, int nSeconds);
};

inline istream& operator>>(istream& is, CSGClock& ck) { ck.In(is); return is;}
inline ostream& operator<<(ostream& os, const CSGClock& ck)  { ck.Out(os); return os;}

class CSGPlayerInfo {
public:
	void In(istream& is);

	void Clear();

	string sName;
	double dRating;
};

inline istream& operator>>(istream& is, CSGPlayerInfo& pi) { pi.In(is); return is;}

class CSGResult {
public:
	double dResult;
	enum TStatus { kBye, kUnstarted, kUnfinished, kAwaitingResult, kAdjourned, kAborted, 
					kNormalEnd, kTimeout, kResigned, kAgreedScore} status;

	void In(istream& is);
	void Out(ostream& os) const;

	void Set(double dResult, TStatus status=kNormalEnd);
	void Clear();

	bool HasScore() const;
	bool GameOver() const;

	bool operator<(const CSGResult& b) const { return this<&b; }
	bool operator==(const CSGResult& b) const { return this==&b; }
};

inline istream& operator>>(istream& is, CSGResult& result) {result.In(is); return is; }
inline ostream& operator<<(ostream& os, const CSGResult& result)  { result.Out(os); return os;}

class CSGMoveList : public vector<CSGMoveListItem> {
public:
	void Update(const CSGMoveListItem& mli);
};

class CSGMatch {
public:
	virtual ~CSGMatch() {};

	istream& InDelta(istream& is);
	istream& In(istream& is);

	string idm;
	CSGPlayerInfo pis[2];
	string sMatchType;
	char cRated;	// U = unrated, R=rated, S=stored, T=tournament
	int nObservers;	// not in matchDelta msgs

	bool IsPlaying(const string& sLogin) const;

	bool operator<(const CSGMatch& b) const { return idm<b.idm; };
	bool operator==(const CSGMatch& b) const { return idm==b.idm; };
};

inline istream& operator>>(istream& is, CSGMatch& match) {match.In(is); return is; }

// /os: +  .21 1906.0 n3       01:00:00//02:00       8b S 1382.1 n2       .64399
template <class TRules>
class CSGRequest {
public:
	void In(istream& is);

	CSGPlayerInfo pis[2];
	CSGClock cks[2];

	typename TRules::TMatchType mt;
  
	char cType;	// U=unrated, R=rated, S=stored
	string idsm;
};

// 24 Feb 2001 18:08:41
class CSGDateTime {
public:
	int day;
	string sMonth;
	int year;
	string sTime;

	void In(istream& is);
	string Text() const;
};

inline istream& operator>>(istream& is, CSGDateTime& dt) {dt.In(is); return is; }

class CSGStoredMatch {
public:
	string idsm;
	string sPlayers[2];
	string sMatchType;
	CSGDateTime dt;

	void In(istream& is);

	bool operator<(const CSGStoredMatch& b) const { return this<&b; };
	bool operator==(const CSGStoredMatch& b) const { return this==&b; };
};

inline istream& operator>>(istream& is, CSGStoredMatch& sg) {sg.In(is); return is; }

class CSGRating {
public:
	double dRating, dSD;

	void In(istream& is);
	void Out(ostream& os) const;
	double AdjustedRating() const;
};

inline istream& operator>>(istream& is, CSGRating& r) {r.In(is); return is; }
inline ostream& operator<<(ostream& os, const CSGRating &r)  { r.Out(os); return os; }


class CSGRatingData {
public:
	CSGRating rating;
	string sInactive;
	double d1, d2;	// what are these?
	int nWins, nDraws, nLosses;

	void In(istream& is);
};

inline istream& operator>>(istream& is, CSGRatingData& rd) {rd.In(is); return is; }

//     1 leaflet  2103.9@ 33.8=   1.07:08:24+@ 31.3  +4.5    551     65    395
class CSGRankData {
public:
	int iRank;
	CSGRatingData rd;
	string sLogin;
	bool fMe;

	void In(istream& is);

	bool operator<(const CSGRankData& b) const { return iRank<b.iRank; };
	bool operator==(const CSGRankData& b) const { return iRank==b.iRank; };
};

inline istream& operator>>(istream& is, CSGRankData& rd) {rd.In(is); return is; }

class CSGFingerRating {
public:
	CSGRatingType rt;
	CSGRatingData rd;

	void In(istream& is);

	bool operator<(const CSGFingerRating& b) const { return this<&b; };
	bool operator==(const CSGFingerRating& b) const { return this==&b; };
};

inline istream& operator>>(istream& is, CSGFingerRating& fr) {fr.In(is); return is; }

/*
/GG: who    26      change:    win    draw    lGGs         match(es)
|tit4tat  + 1145.7@301.5 ->   +14.3   -12.1   -38.4 @ 79.6 
|n2       + 1384.1@ 80.5 
|ant      + 1426.2@ 50.8 ->   +39.2    +4.2   -30.8 @ 78.5 
etc.
*/


class CSGWhoItem {
public:
	string sLogin;
	CSGRating rating;
	double dWin, dDraw, dLoss, dSDNew;
	bool fMe;

	void In(istream& is);

	bool operator<(const CSGWhoItem& b) const;
	bool operator==(const CSGWhoItem& b) const;
};

inline istream& operator>>(istream& is, CSGWhoItem& wi) {wi.In(is); return is; }


// .48723   25 Mar 2001 03:28:00 1780 nasai    2144 OO7        -4.0 8

class CSGHistoryItem {
public:
	string idsm;
	CSGDateTime dt;
	CSGPlayerInfo pis[2];
	CSGResult result;
	string sMatchType;
	void In(istream& is);

	bool operator<(const CSGHistoryItem& b) const { return idsm<b.idsm; };
	bool operator==(const CSGHistoryItem& b) const { return idsm==b.idsm; };
};

inline istream& operator>>(istream& is, CSGHistoryItem& hi) {hi.In(is); return is; }

typedef unsigned char u1;

class CSGBoardBase {
public:
	string sBoard;
	int iMover;

	virtual int PieceCount(char c) const;
	virtual const CSGBoardTypeBase& BT() const = 0;
	virtual int Result(bool fAnti=false) const = 0;
	virtual ~CSGBoardBase();
};

template<class aTBoardType>
class CSGBoard : public CSGBoardBase {
public:
	virtual ~CSGBoard() {};

	enum { BLACK = '*', WHITE='O', EMPTY='-', DUMMY='d', UNKNOWN='?'};

	typedef aTBoardType TBoardType;

	TBoardType bt;

	virtual void Initialize()=0;
	virtual void Update(const string& sMove)=0;

	char PieceGet(int x, int y) const;
	char PieceGet(const CSGSquare& sq) const;
	void PieceSet(int x, int y, char piece);
	void PieceSet(const CSGSquare& sq, char piece);

	void In(istream& is);
	void Out(ostream& is) const;
	void OutFormatted(ostream& os) const;

	char* TextGet(char* sBoard, int& iMover, bool fTrailingNull=true) const;
	void TextSet(const char* sBoard);
	char CMover() const;

	// Legal moves etc
	virtual bool GameOver() const = 0;
	void Clear();

	const CSGBoardTypeBase& BT() const;

protected:
	int Loc(int x, int y) const;
	void OutHeader(ostream& os) const;
};

class CSGPositionBase {
public:
	virtual ~CSGPositionBase() {};

	CSGClock cks[2];
	virtual void Update(const CSGMoveListItem& mli) = 0;
	virtual void Update(const CSGMoveList& ml, u4 nMoves=100000) = 0;
	virtual void UpdateKomiSet(const CSGMoveListItem mlis[2]) = 0;

	virtual const CSGBoardBase& Board() const = 0;

	virtual void Clear() = 0;
};

template<class TBoard>
class CSGPosition : public CSGPositionBase {
public:
	virtual ~CSGPosition() {};

	TBoard board;
	void Update(const CSGMoveListItem& mli);
	void Update(const CSGMoveList& ml, u4 nMoves=100000);
	void UpdateKomiSet(const CSGMoveListItem mlis[2]);

	const CSGBoardBase& Board() const;

	void Clear();
};

class CSGBase;

class CSGGameBase {
public:
	// basic info
	virtual ~CSGGameBase() {};

	string sPlace, sDateTime;
	CSGPlayerInfo pis[2];
	CSGMoveList ml;
	CSGMoveListItem mlisKomi[2];
	CSGResult result;

	// live games only
	bool fSynchronized;

	// creation/destruction
	static CSGGameBase* NewFromStream(istream & is);
	static string InGameName(istream& is);
	void SetService(CSGBase* pSg, const string& idg);

	// updates
	virtual void Update(const CSGMoveListItem& mli) = 0;
	virtual void UpdateKomiSet(const CSGMoveListItem mlis[2]) = 0;

	// I/O
	void In(istream& is);

	virtual void InPartial(istream& is) = 0;
	virtual void Out(ostream& os) const = 0;

	// Creation
	void SetTime(time_t t);
	void SetCurrentTime();

	virtual void Clear() = 0;

	// Modification
	virtual void Undo() = 0;
	virtual void SetResult(const CSGResult& result);
	virtual void SetResult(const CSGResult& result, const string sName);

	// Information
	virtual bool GameOver() const = 0;
	virtual bool GameOverOnBoard() const = 0;
	virtual bool NeedsKomi() const = 0;
	virtual string GetForcedMove(int& iType) const = 0;
	virtual bool IsSynchro() const = 0;
	virtual const CSGPositionBase& Pos() const = 0;
	virtual const CSGMatchTypeBase& MT() const = 0;
	virtual bool ToMove(const string& sLogin) const;
	virtual const string& GameName() const = 0;
	virtual const int IMoverStart() const = 0;

	// info for live games
	bool IAmPlaying() const;
	bool IAmPlaying(int iMover) const;
	bool IWasPlaying() const;
	bool IsMyMove() const;
	CSGBase* PSg() const;
	const string& Idg() const;
	double TimeSinceLastMove() const;
	bool IsSynchronized() const;
	virtual void GetEstimatedTimes(CSGClock cksRemaining[2]) const = 0;
	virtual string GetLogin() const;

	// server actions
	virtual void Kill();
	virtual void ServiceComment(const string& sComment);
	int ViewerComment(const string& sComment);
	virtual int WatchEnd();
	//virtual void ViewerResign();

	virtual CSGGameBase* NewCopy() const=0;
	virtual CSGPositionBase* PosCopy(u4 iMoves) const = 0;

	// in here for MS STL only
	bool operator<(const CSGGameBase& b) const { return this<&b; };
	bool operator==(const CSGGameBase& b) const { return this==&b; };

	void SetLastMoveTime();

protected:
	CSGGameBase();

	// server tracking info
	CSGBase *pSg;
	string idg;
	string sLogin;	// for determining who I am even after server closes
	time_t tLastMove;
	clock_t clockLastMove;	// for more accurate time reporting
};

template<class aTPosition,class aTMatchType>
class CSGGame : public CSGGameBase {
public:
	CSGGame() {};
	CSGGame(istream& is);
	CSGGame(istream& is, CSGBase* apSg, const string& aidg);
	virtual ~CSGGame() {};

	typedef aTMatchType TMatchType;
	typedef aTPosition TPosition;

	TPosition posStart, pos;
	TMatchType mt;

	void Update(const CSGMoveListItem& mli);
	void UpdateKomiSet(const CSGMoveListItem mlis[2]);

	// I/O
	virtual void InPartial(istream& is);
	void Out(ostream& os) const;

	void Clear();

	// Modification
	virtual void Undo();

	// Information
	bool GameOver() const;
	virtual bool GameOverOnBoard() const;
	void GetEstimatedTimes(CSGClock cksRemaining[2]) const;
	string GetForcedMove(int& iType) const;
	bool IsSynchro() const;
	virtual const CSGPositionBase& Pos() const;
	virtual const CSGMatchTypeBase& MT() const;
	virtual const string& GameName() const;
	virtual const int IMoverStart() const { return posStart.board.iMover; };

	bool NeedsKomi() const;
	double KomiValue() const;

	void CalcPos(TPosition& pos, u4 nMoves) const;
	CSGPositionBase* PosCopy(u4 iMoves) const;

	void CalcCurrentPos();
};

template<class TPosition, class TMatchType>
inline istream& operator>>(istream& is, CSGGame<TPosition, TMatchType>& ggf) {ggf.In(is); return is; }

template<class TPosition, class TMatchType>
inline ostream& operator<<(ostream& os, const CSGGame<TPosition, TMatchType>& ggf) {ggf.Out(os); return os; }

template<class TPosition, class TMatchType>
inline CSGGame<TPosition, TMatchType>::CSGGame(istream& is) {
	InPartial(is);
}

template<class TPosition, class TMatchType>
inline CSGGame<TPosition, TMatchType>::CSGGame(istream& is, CSGBase
											   * apSg, const string& aidg)
	: CSGGameBase(apSg, aidg) {
	In(is);
}

template<class TPosition, class TMatchType>
inline string CSGGame<TPosition, TMatchType>::GetForcedMove(int& iType) const{
	return pos.board.GetForcedMove(iType);
}

#include "SGObjects_T.h"

#endif	//GDK_SGObjects_H
