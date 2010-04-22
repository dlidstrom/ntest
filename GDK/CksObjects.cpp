// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "types.h"
#include "CksObjects.h"
#include <strstream>
#include <iomanip>
#include <time.h>
#include <ctype.h>

using namespace std;

const char* CCksRules::sLogin = "/cks";
const string CCksRules::sGameName = "Checkers";

///////////////////////////////////////////
// CCksBoardType
///////////////////////////////////////////

void CCksBoardType::In(istream& is) {
	Clear();

	// Othello specific right now
	is >> nWidth;
	switch(nWidth) {
	case 4:
	case 6:
	case 8:
	case 10:
	case 12:
	case 14:
		nOcto=0;
		break;
	case 88:
		nWidth=10;
		nOcto=2;
		break;
	default:
		nWidth=8;
		nOcto=0;
		is.setstate(ios::failbit);
		break;
	}
	nHeight=nWidth;
}

void CCksBoardType::Out(ostream& os) const {
	os << nOcto?88:nWidth;
}

string CCksBoardType::StringGet() const {
	ostrstream os;
	Out(os);
	os << '\0';
	string sResult(os.str());
	os.rdbuf()->freeze(0);
	return sResult;
}

void CCksBoardType::StringSet(const string& sBoardType) {
	istrstream is(sBoardType.c_str());
	In(is);
}

///////////////////////////////////////////
// CCksMatchType
///////////////////////////////////////////

void CCksMatchType::Clear() {
	cColor='?';
	dKomiValue=0;
	fAnti=fHasKomiValue=fRand=false;
	nRandDiscs=0;
}

void CCksMatchType::In(istream& is) {
	Clear();
	fHasKomiValue=false;

	bool fOK=false;	// if we get a board type, clear EOFs if we peek too far

	is >> ws;
	while (isalnum(is.peek())) {
		char c=char(is.peek());
		if (isdigit(c)) {
			bt.In(is);
			fOK=true;
		}
		else{
			is >> c;
			switch(tolower(c)) {
			case 'k':
				QSSERT(cColor=='?');
				cColor='k';
				// check for komi value in e.g. match history
				c=char(is.peek());
				if (isdigit(c) || c=='+' || c=='-' || c=='.') {
					is >> dKomiValue;
					fHasKomiValue=true;
				}
				break;
			case 's':
			case 'b':
			case 'w':
				QSSERT(cColor=='?');
				cColor=char(tolower(c));
				break;
			case 'a':
				fAnti=true;
				break;
			case 'r':
				fRand=true;
				is >> nRandDiscs;
				break;
			default:
				QSSERT(0);
			}
		}
	}
	if (fOK && is.eof() && !is.bad())
		is.clear();
}

void CCksMatchType::Out(ostream& os) const {
	if (cColor=='s')
		os << 's';
	os << bt.StringGet();
	if (cColor=='k') {
		os << 'k';
		if (fHasKomiValue)
			os<< dKomiValue;
	}
	if (fAnti)
		os << 'a';
	if (fRand)
		os << 'r' << nRandDiscs;
	if (cColor=='b' || cColor=='w')
		os << cColor;
}

double CCksMatchType::MaxResult() const {
	return bt.NPlayableSquares()*0.5;
}

string CCksMatchType::Description() const {
	ostrstream os;

	os << bt.nWidth << "x" << bt.nHeight << " ";
	if (fAnti)
		os << "Anti";
	if (fRand)
		os << "Rand";
	os << '\0';

	string sResult(os.str());
	os.rdbuf()->freeze(0);

	return sResult;

}

void CCksMatchType::GetTDOut(string& smtOut) const {
	CCksMatchType mtout(*this);

	switch(tolower(mtout.cColor)) {
	case 'w':
	case 'b':
	case '?':
		mtout.cColor='b';
	}

	ostrstream os;
	os << mtout << '\0';
	smtOut=os.str();
	os.rdbuf()->freeze(0);
}

void CCksBoard::Initialize() {
	CSGBoard<TBoardType>::Initialize();

	int x=bt.nWidth/2;
	int y=bt.nHeight/2;
	PieceSet(x, y-1,BLACK);
	PieceSet(x-1, y,BLACK);
	PieceSet(x, y,WHITE);
	PieceSet(x-1, y-1,WHITE);

	iMover=0;
}

void CCksBoard::Update(const string& sMove) {
	istrstream is(sMove.c_str());

	CSGSquare sqStart, sqEnd;
	char c;

	is >> sqStart;
	while (is.peek()=='-') {
		is >> c >> sqEnd;
		QSSERT(c=='-');
		UpdatePiece(sqStart, sqEnd);
		sqStart=sqEnd;
	}

	// king piece if needed
	char piece=PieceGet(sqEnd);
	if (!PieceToKing(piece)) {
		switch(piece) {
		case RED_MAN:
			if (sqEnd.y==bt.nHeight-1)
				piece=RED_KING;
			break;
		case WHITE_MAN:
			if (sqEnd.y==0)
				piece=WHITE_KING;
			break;
		default: QSSERT(0);
		}
		PieceSet(sqEnd, piece);
	}

	iMover=!iMover;
};

void CCksBoard::UpdatePiece(const CSGSquare& sqStart, const CSGSquare& sqEnd) {
	bool fJump;
	
	// figure out if this is a jump
	int nDiff;
	nDiff=sqEnd.x-sqStart.x;
	if (nDiff<0)
		nDiff=-nDiff;
	switch(nDiff) {
	case 1:
		fJump=false;
		break;
	case 2:
		fJump=true;
		break;
	default:
		fJump=false;	// this should never happen
		QSSERT(0);
	}

	// if this is a jump, remove the piece in the middle
	if (fJump) {
		int x,y;

		x=(sqStart.x+sqEnd.x)>>1;
		y=(sqStart.y+sqEnd.y)>>1;

		PieceSet(x,y,EMPTY);
	}

	// move the piece
	PieceSet(sqEnd, PieceGet(sqStart));
	PieceSet(sqStart, EMPTY);
}

bool CCksBoard::GameOver() const {
	return GetStartSquares(iMover).empty();
}

int CCksBoard::PieceToPlayer(char c) {
	switch(c) {
	case RED_MAN:
	case RED_KING:
		return 0;
	case WHITE_MAN:
	case WHITE_KING:
		return 1;
	default:
		return -1;
	}
}

int CCksBoard::PieceToKing(char c) {
	switch(c) {
	case RED_MAN:
	case WHITE_MAN:
		return 0;
	case RED_KING:
	case WHITE_KING:
		return 1;
	default:
		return -1;
	}
}

// preconditions: piece(x,y) is iPlayer's color, this routine does not check
//	dx and dy must be +/- 1
bool CCksBoard::HasJumpSquareDirection(int x, int y, int dx, int dy, int iPlayer) const {
	int iOpponent=1-iPlayer;

	return (PieceToPlayer(PieceGet(x+dx, y+dy))==iOpponent) &&
		PieceGet(x+dx+dx, y+dy+dy)==EMPTY;
}

// returns true if iPlayer has a jump from this square,. Returns false
//	if iPlayer doesn't have a piece on the square

bool CCksBoard::HasJumpSquare(int x, int y, int iPlayer) const {
	char c=PieceGet(x,y);
	if (PieceToPlayer(c)!=iPlayer)
		return false;
	if (PieceToKing(c)) {
		return HasJumpSquareDirection(x,y,1,1,iPlayer) ||
				HasJumpSquareDirection(x,y,-1,1,iPlayer) ||
				HasJumpSquareDirection(x,y,1,-1,iPlayer) ||
				HasJumpSquareDirection(x,y,-1,-1,iPlayer);
	}
	else {
		int dyMan=DyMan(iPlayer);
		return HasJumpSquareDirection(x,y,1,dyMan,iPlayer) ||
				HasJumpSquareDirection(x,y,-1,dyMan,iPlayer);
	}
}

void CCksBoard::PushJumpSquare(vector<CSGSquare>& sqs, const CSGSquare& sqStart,int dx, int dy) const {
	if (HasJumpSquareDirection(sqStart.x,sqStart.y,dx,dy,iMover))
		sqs.push_back(CSGSquare(sqStart.x+dx+dx, sqStart.y+dy+dy));
}

void CCksBoard::PushSlideSquare(vector<CSGSquare>& sqs, const CSGSquare& sqStart,int dx, int dy) const {
	if (PieceGet(sqStart.x+dx,sqStart.y+dy)==EMPTY)
		sqs.push_back(CSGSquare(sqStart.x+dx, sqStart.y+dy));
}

// assumes the piece belongs to the mover
vector<CSGSquare> CCksBoard::GetJumpEndSquares(const CSGSquare& sqStart) const {
	vector<CSGSquare> sqs;

	char c=PieceGet(sqStart);
	QSSERT(iMover==PieceToPlayer(c));

	if (PieceToKing(c)) {
		PushJumpSquare(sqs,sqStart,1,1);
		PushJumpSquare(sqs,sqStart,1,-1);
		PushJumpSquare(sqs,sqStart,-1,1);
		PushJumpSquare(sqs,sqStart,-1,-1);
	}
	else {
		int dyMan=DyMan(iMover);

		PushJumpSquare(sqs,sqStart,1,dyMan);
		PushJumpSquare(sqs,sqStart,-1,dyMan);
	}

	return sqs;
}

int CCksBoard::DyMan(int iPlayer) {
	return iPlayer?-1:1;
}

// assumes the piece belongs to the mover
vector<CSGSquare> CCksBoard::GetSlideEndSquares(const CSGSquare& sqStart) const {
	vector<CSGSquare> sqs;

	char c=PieceGet(sqStart);
	QSSERT(iMover==PieceToPlayer(c));

	if (PieceToKing(c)) {
		PushSlideSquare(sqs,sqStart,1,1);
		PushSlideSquare(sqs,sqStart,1,-1);
		PushSlideSquare(sqs,sqStart,-1,1);
		PushSlideSquare(sqs,sqStart,-1,-1);
	}
	else {
		int dyMan=DyMan(iMover);

		PushSlideSquare(sqs,sqStart,1,dyMan);
		PushSlideSquare(sqs,sqStart,-1,dyMan);
	}

	return sqs;
}

// returns true if iPlayer has a slide from this square,. Returns false
//	if iPlayer doesn't have a piece on the square

bool CCksBoard::HasSlideSquare(int x, int y, int iPlayer) const {
	char c=PieceGet(x,y);
	if (PieceToPlayer(c)!=iPlayer)
		return false;
	if (PieceToKing(c)) {
		return PieceGet(x+1,y+1)==EMPTY ||
			PieceGet(x+1,y-1)==EMPTY ||
			PieceGet(x-1,y+1)==EMPTY ||
			PieceGet(x-1,y-1)==EMPTY;
	}
	else {
		int dyMan=DyMan(iPlayer);
		return PieceGet(x+1,y+dyMan)==EMPTY ||
			PieceGet(x-1,y+dyMan)==EMPTY;
	}
}

vector<CSGSquare> CCksBoard::GetStartSquares(int iPlayer) const {
	bool fJump;

	return GetStartSquares(iPlayer, fJump);
}

vector<CSGSquare> CCksBoard::GetStartSquares(int iPlayer, bool& fJump) const {
	int x, y;
	vector<CSGSquare> sqs;

	// check for jumps
	for (x=0; x<bt.nWidth; x++) {
		for (y=0; y<bt.nHeight; y++) {
			if (HasJumpSquare(x,y,iPlayer))
				sqs.push_back(CSGSquare(x,y));
		}
	}

	if (!sqs.empty()) {
		fJump=true;
		return sqs;
	}

	// check for slides
	for (x=0; x<bt.nWidth; x++) {
		for (y=0; y<bt.nHeight; y++) {
			if (HasSlideSquare(x,y,iPlayer))
				sqs.push_back(CSGSquare(x,y));
		}
	}

	fJump=false;
	return sqs;
}

string CCksBoard::GetForcedMove(int& iType) const {
	/* not working, needs to take pieces off the board as they are jumped

	iType=1;	// never have a forced pass in checkers!
	string sResult;
	bool fJump;

	vector<CSGSquare> sqs = GetStartSquares(iMover, fJump);
	QSSERT(!sqs.empty());

	if (sqs.size()==1) {
		CSGSquare sqStart=sqs[0];

		if (fJump) {
			sResult=sqStart;
			while ((sqs=GetJumpEndSquares(sqStart)).size()==1) {
				sResult+="-";
				sqStart=sqs[0];
				sResult+=sqStart;
			}
			if (sqs.size()>1)
				sResult="";
		}
		else {
			sqs=GetSlideEndSquares(sqStart);
			QSSERT(!sqs.empty());
			if (sqs.size()==1) {
				sResult=sqStart;
				sResult+="-";
				sResult+=sqs[0];
			}
		}
	}

	return sResult;
	*/
	return "";
}

void CCksBoard::GetPieceCounts(int& nBlack, int& nWhite, int& nEmpty) const {
	nBlack=nWhite=nEmpty=0;
	int i;
	for (i=0; i<bt.NTotalSquares(); i++) {
		switch(sBoard[i]) {
		case BLACK: nBlack++; break;
		case WHITE: nWhite++; break;
		case EMPTY: nEmpty++; break;
		case DUMMY: break;
		default: QSSERT(0);
		}
	}
}

int CCksBoard::NetBlackSquares() const {
	int nBlack, nWhite, nEmpty;
	GetPieceCounts(nBlack, nWhite, nEmpty);
	return nBlack-nWhite;
}

int CCksBoard::Result(bool fAnti) const {
	int nBlack, nWhite, nEmpty, nNet;
	GetPieceCounts(nBlack, nWhite, nEmpty);
	nNet=nBlack-nWhite;
	if (nNet<0)
		nNet-=nEmpty;
	else if (nNet>0)
		nNet+=nEmpty;

	return fAnti?-nNet:nNet;
}

///////////////////////////////////////////
// CCksGame
///////////////////////////////////////////

CSGGameBase* CCksGame::NewCopy() const {
	return new CCksGame(*this);
}

const string& CSGGame<CCksPosition, CCksMatchType>::GameName() const{
	return CCksRules::sGameName;
}
