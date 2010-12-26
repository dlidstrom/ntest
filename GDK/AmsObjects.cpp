// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "types.h"
#include "AmsObjects.h"

#include <strstream>
#include <iomanip>
#include <time.h>
#include <ctype.h>

using namespace std;

const char* CAmsRules::sLogin = "/ams";
const string CAmsRules::sGameName = "Amazons";

/*
void CAmsMove::In(istream& is) {
	char cCol;

	is >> cCol;
	cCol=toupper(cCol);
	fPass= cCol=='P';
	if (fPass) {
		while (isalpha(is.peek()) && is>>cCol) {};
	}
	else {
		y=cCol-'A';
		is >> x;
		x--;
	}
}

void CAmsMove::Out(ostream& os) const {
	if (fPass)
		os << "PA";
	else
		os << (char)(y+'A') << x+1;
}

void CAmsMove::SetIOS(int iosmove) {
	fPass=false;
	if (iosmove<0)
		iosmove=-iosmove;
	x=(iosmove%10)-1;
	y=(iosmove/10)-1;
}

bool CAmsMove::operator==(const CAmsMove& b) const {
	if (fPass || b.fPass)
		return fPass && b.fPass;
	else
		return x==b.x && y==b.y;
}

bool CAmsMove::operator<(const CAmsMove& b) const {
	if (fPass!=b.fPass)
		return fPass;
	else if (x!=b.x)
		return x<b.x;
	else
		return y<b.y;
}



void CAmsRatingType::In(istream& is) {
	fRand=fAnti=0;
	is >> ws;
	while (isalnum(is.peek())) {
		char c;
		c=is.peek();
		if (isdigit(c))
			is >> bt;
		else{
			is >> c;
			switch(tolower(c)) {
			case 'a':
				fAnti=true;
				break;
			case 'r':
				fRand=true;
				break;
			default:
				QSSERT(0);
			}
		}
	}
}

void CAmsRatingType::Out(ostream& os) const {
	os << bt;
	if (fAnti)
		os << 'a';
	if (fRand)
		os << 'r';
}
*/

///////////////////////////////////////////
// CAmsBoardType
///////////////////////////////////////////

void CAmsBoardType::In(istream& is) {
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

void CAmsBoardType::Out(ostream& os) const {
	os << (nOcto?88:nWidth);
}

string CAmsBoardType::StringGet() const {
	ostrstream os;
	Out(os);
	os << '\0';
	string sResult(os.str());
	os.rdbuf()->freeze(0);
	return sResult;
}

void CAmsBoardType::StringSet(const string& sBoardType) {
	istrstream is(sBoardType.c_str());
	In(is);
}

///////////////////////////////////////////
// CAmsMatchType
///////////////////////////////////////////

void CAmsMatchType::Clear() {
	cColor='?';
	dKomiValue=0;
	fAnti=fHasKomiValue=fRand=false;
	nRandDiscs=0;
}

// set the color type, if we already had one raise an exception
void CAmsMatchType::NewColor(char c, const string& smt) {
	if (cColor!='?')
		throw CError(0x10001, smt+" is not a valid amazons match type");
	else
		cColor=c;
}

void CAmsMatchType::In(istream& isBig) {
	Clear();
	fHasKomiValue=false;

	string smt;

	isBig >> ws >> smt;

	istrstream is(smt.c_str());

	while (isalnum(is.peek())) {
		char c;
		c=char(is.peek());
		if (isdigit(c)) {
			bt.In(is);
		}
		else{
			is >> c;
			c=char(tolower(c));
			switch(c) {
			case 'k': {
				// check for komi value in e.g. match history
				int c2=is.peek();
				if (isdigit(c2) || c2=='+' || c2=='-' || c2=='.') {
					is >> dKomiValue;
					fHasKomiValue=true;
				}
					  }
				// fall through to other colors
			case 's':
			case 'b':
			case 'w':
				NewColor(c, smt);
				break;
			case 'a':
				fAnti=true;
				break;
			case 'r':
				fRand=true;
				// no rand discs in amazons
				break;
			default:
				QSSERT(0);
			}
		}
	}
}

double CAmsMatchType::MaxResult() const {
	return bt.NPlayableSquares();
}

string CAmsMatchType::Description() const {
	ostrstream os;

	os << bt.nWidth << "x" << bt.nHeight << " ";
	if (fAnti)
		os << "Anti";
	switch(cColor) {
	case 's': os << "Synchro"; break;
	case 'k': os << "Komi"; break;
	}
	if (fRand)
		os << "Rand";
	os << '\0';

	string sResult(os.str());
	os.rdbuf()->freeze(0);

	return sResult;
}

void CAmsMatchType::GetTDOut(string& smtOut) const {
	CAmsMatchType mtout(*this);

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

void CAmsMatchType::Out(ostream& os) const {
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

int CAmsMatchType::Validate() const {
	if (!fRand)
		return 0;
	else if (cColor!='k' && cColor!='s')
		return kErrRandUnbalanced;
	else if (nRandDiscs<bt.NRandDiscsMin())
		return kErrTooFewRandDiscs;
	else if (nRandDiscs>bt.NRandDiscsMax())
		return kErrTooManyRandDiscs;
	else
		return 0;
}

////////////////////////////////////
// CAmsBoard
////////////////////////////////////

void CAmsBoard::Initialize() {
	CSGBoard<TBoardType>::Initialize();

	int x=bt.nWidth/2;
	int y=bt.nHeight/2;
	PieceSet(x, y-1,BLACK);
	PieceSet(x-1, y,BLACK);
	PieceSet(x, y,WHITE);
	PieceSet(x-1, y-1,WHITE);

	iMover=0;
}

void CAmsBoard::Update(const string& sMove) {
	if (toupper(sMove[0])!='P') {
		int x,y;
		CSGSquare(sMove).XYGet(x,y);
		UpdateXY(x,y);
	}

	iMover=!iMover;
};

void CAmsBoard::UpdateXY(int x, int y) {
	int dx, dy;
	char cMover, cOpponent;

	QSSERT(x<bt.nWidth && y<bt.nHeight && x>=0 && y>=0);

	if (PieceGet(x, y)!=EMPTY) {
		OutFormatted(cerr);
		QSSERT(0);
	}
	else {

		// get yors
		if (iMover) {
			cMover=WHITE;
			cOpponent=BLACK;
		}
		else {
			cMover=BLACK;
			cOpponent=WHITE;
		}

		// update board
		int nFlipped=0;
		PieceSet(x,y,cMover);
		for (dx=-1; dx<=1; dx++) {
			for (dy=-1; dy<=1; dy++) {
				if (dx || dy)
					nFlipped+=UpdateDirection(x, y, dx, dy,cMover, cOpponent);
			}
		}
		QSSERT(nFlipped);
	}
}

int CAmsBoard::NPass() const {
	if (HasLegalMove(iMover))
		return 0;
	else if(HasLegalMove(!iMover))
		return 1;
	else
		return 2;
}

bool CAmsBoard::GameOver() const {
	return !HasLegalMove(iMover) && !HasLegalMove(!iMover);
}

bool CAmsBoard::HasLegalMove(int iPlayer) const {
	int x, y;

	for (x=0; x<bt.nWidth; x++) {
		for (y=0; y<bt.nHeight; y++)
			if (IsMoveLegal(x,y,iPlayer))
				return true;
	}
	return false;
}

bool CAmsBoard::IsMoveLegal(const string& sMove) const {
	if (toupper(sMove[0])!='P') {
		int x,y;
		CSGSquare(sMove).XYGet(x,y);
		return IsMoveLegal(x,y)!=0;
	}
	else
		return !HasLegalMove();
}

vector<CSGSquare> CAmsBoard::GetMoves(bool fMover) const {
	int x, y;
	vector<CSGSquare> mvs;

	for (x=0; x<bt.nWidth; x++) {
		for (y=0; y<bt.nHeight; y++)
			if (IsMoveLegal(x,y,fMover?iMover:!iMover))
				mvs.push_back(CSGSquare(x,y));
	}

	return mvs;
}

string CAmsBoard::GetForcedMove(int& iType) const {

	vector<CSGSquare> mvs=GetMoves();

	if (mvs.empty()) {
		iType=0;
		return "PA";
	}
	else if (mvs.size()==1) {
		iType=1;
		return mvs[0];
	}
	else {
		iType=-1;
		return "";
	}
}

int CAmsBoard::IsMoveLegal(int x, int y, int iPlayer) const {
	int dx, dy;
	char cMover, cOpponent;

	if (x<0 || y<0 || x>=bt.nWidth || y>=bt.nHeight)
		return 0;

	if(PieceGet(x, y)!=EMPTY)
		return 0;

	// get yors
	if (iPlayer) {
		cMover=WHITE;
		cOpponent=BLACK;
	}
	else {
		cMover=BLACK;
		cOpponent=WHITE;
	}

	// update board
	int nFlipped=0;
	for (dx=-1; dx<=1; dx++) {
		for (dy=-1; dy<=1; dy++) {
			if (dx || dy)
				nFlipped+=IsMoveLegalDirection(x, y, dx, dy,cMover, cOpponent);
		}
	}
	return nFlipped;
};

int CAmsBoard::UpdateDirection(int x, int y, int dx, int dy, char cMover, char cOpponent) {
	int xEnd,yEnd;
	int nFlipped;
	nFlipped=0;

	for (xEnd=x+dx, yEnd=y+dy; PieceGet(xEnd,yEnd)==cOpponent; xEnd+=dx, yEnd+=dy);
	if (PieceGet(xEnd,yEnd)==cMover) {
		for (xEnd=x+dx, yEnd=y+dy; PieceGet(xEnd,yEnd)==cOpponent; xEnd+=dx, yEnd+=dy) {
			PieceSet(xEnd, yEnd,cMover);
			nFlipped++;
		}
	}

	return nFlipped;
}

int CAmsBoard::IsMoveLegalDirection(int x, int y, int dx, int dy, char cMover, char cOpponent) const {
	int xEnd,yEnd;
	int nFlipped;
	nFlipped=0;

	for (xEnd=x+dx, yEnd=y+dy; PieceGet(xEnd,yEnd)==cOpponent; xEnd+=dx, yEnd+=dy);
	if (PieceGet(xEnd,yEnd)==cMover) {
		for (xEnd=x+dx, yEnd=y+dy; PieceGet(xEnd,yEnd)==cOpponent; xEnd+=dx, yEnd+=dy) {
			nFlipped++;
		}
	}

	return nFlipped;
}


void CAmsBoard::GetPieceCounts(int& nBlack, int& nWhite, int& nEmpty) const {
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

int CAmsBoard::NetBlackSquares() const {
	int nBlack, nWhite, nEmpty;
	GetPieceCounts(nBlack, nWhite, nEmpty);
	return nBlack-nWhite;
}

int CAmsBoard::Result(bool fAnti) const {
	int nBlack, nWhite, nEmpty, nNet;
	GetPieceCounts(nBlack, nWhite, nEmpty);
	nNet=nBlack-nWhite;
	if (nNet<0)
		nNet-=nEmpty;
	else if (nNet>0)
		nNet+=nEmpty;

	return fAnti?-nNet:nNet;
}

////////////////////////////////////
// CAmsGame
////////////////////////////////////

CSGGameBase* CAmsGame::NewCopy() const {
	return new CAmsGame(*this);
}

const string& CSGGame<CAmsPosition, CAmsMatchType>::GameName() const{
	return CAmsRules::sGameName;
}
