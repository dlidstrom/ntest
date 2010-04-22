// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "types.h"
#include "OsObjects.h"

#include <strstream>
#include <iomanip>
#include <time.h>
#include <ctype.h>

using namespace std;

const char* COsRules::sLogin = "/os";
const string COsRules::sGameName = "Reversi";

/*
void COsMove::In(istream& is) {
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

void COsMove::Out(ostream& os) const {
	if (fPass)
		os << "PA";
	else
		os << (char)(y+'A') << x+1;
}

void COsMove::SetIOS(int iosmove) {
	fPass=false;
	if (iosmove<0)
		iosmove=-iosmove;
	x=(iosmove%10)-1;
	y=(iosmove/10)-1;
}

bool COsMove::operator==(const COsMove& b) const {
	if (fPass || b.fPass)
		return fPass && b.fPass;
	else
		return x==b.x && y==b.y;
}

bool COsMove::operator<(const COsMove& b) const {
	if (fPass!=b.fPass)
		return fPass;
	else if (x!=b.x)
		return x<b.x;
	else
		return y<b.y;
}



void COsRatingType::In(istream& is) {
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

void COsRatingType::Out(ostream& os) const {
	os << bt;
	if (fAnti)
		os << 'a';
	if (fRand)
		os << 'r';
}
*/

///////////////////////////////////////////
// COsBoardType
///////////////////////////////////////////

void COsBoardType::In(istream& is) {
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

void COsBoardType::Out(ostream& os) const {
	os << (nOcto?88:nWidth);
}

string COsBoardType::StringGet() const {
	ostrstream os;
	Out(os);
	os << '\0';
	string sResult(os.str());
	os.rdbuf()->freeze(0);
	return sResult;
}

void COsBoardType::StringSet(const string& sBoardType) {
	istrstream is(sBoardType.c_str());
	In(is);
}

///////////////////////////////////////////
// COsMatchType
///////////////////////////////////////////

void COsMatchType::Clear() {
	cColor='?';
	dKomiValue=0;
	fAnti=fHasKomiValue=fRand=false;
	nRandDiscs=0;
}

// set the color type, if we already had one raise an exception
void COsMatchType::NewColor(char c, const string& smt) {
	if (cColor!='?')
		throw CError(0x10001, smt+" is not a valid match type");
	else
		cColor=c;
}

void COsMatchType::In(istream& isBig) {
	Clear();
	fHasKomiValue=false;

	string smt;

	isBig >> ws >> smt;

	istrstream is(smt.c_str());

	while (isalnum(is.peek())) {
		char c=char(is.peek());
		if (isdigit(c)) {
			bt.In(is);
		}
		else{
			is >> c;
			c=char(tolower(c));
			switch(c) {
			case 'k': {
				// check for komi value in e.g. match history
				char c2=char(is.peek());
				if (isdigit(c2) || c2=='+' || c2=='-' || c2=='.') {
					is >> dKomiValue;
					fHasKomiValue=true;
				}
					  }
				// fall through to other colors
			case 's':
			case 'b':
			case 'w':
				NewColor(char(c), smt);
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
}

double COsMatchType::MaxResult() const {
	return bt.NPlayableSquares();
}

string COsMatchType::Description() const {
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

void COsMatchType::Out(ostream& os) const {
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


void COsMatchType::GetTDOut(string& smtOut) const {
	COsMatchType mtout(*this);

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

int COsMatchType::Validate() const {
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
// COsBoard
////////////////////////////////////

void COsBoard::Initialize() {
	CSGBoard<TBoardType>::Initialize();

	int x=bt.nWidth/2;
	int y=bt.nHeight/2;
	PieceSet(x, y-1,BLACK);
	PieceSet(x-1, y,BLACK);
	PieceSet(x, y,WHITE);
	PieceSet(x-1, y-1,WHITE);

	iMover=0;
}

void COsBoard::Update(const string& sMove) {
	if (toupper(sMove[0])!='P') {
		int x,y;
		CSGSquare(sMove).XYGet(x,y);
		UpdateXY(x,y);
	}

	iMover=!iMover;
};

void COsBoard::UpdateXY(int x, int y) {
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

int COsBoard::NPass() const {
	if (HasLegalMove(iMover))
		return 0;
	else if(HasLegalMove(!iMover))
		return 1;
	else
		return 2;
}

bool COsBoard::GameOver() const {
	return !HasLegalMove(iMover) && !HasLegalMove(!iMover);
}

bool COsBoard::HasLegalMove(int iPlayer) const {
	int x, y;

	for (x=0; x<bt.nWidth; x++) {
		for (y=0; y<bt.nHeight; y++)
			if (IsMoveLegal(x,y,iPlayer))
				return true;
	}
	return false;
}

bool COsBoard::IsMoveLegal(const string& sMove) const {
	if (toupper(sMove[0])!='P') {
		int x,y;
		CSGSquare(sMove).XYGet(x,y);
		return IsMoveLegal(x,y)!=0;
	}
	else
		return !HasLegalMove();
}

vector<CSGSquare> COsBoard::GetMoves(bool fMover) const {
	int x, y;
	vector<CSGSquare> mvs;

	for (x=0; x<bt.nWidth; x++) {
		for (y=0; y<bt.nHeight; y++)
			if (IsMoveLegal(x,y,fMover?iMover:!iMover))
				mvs.push_back(CSGSquare(x,y));
	}

	return mvs;
}

string COsBoard::GetForcedMove(int& iType) const {

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

int COsBoard::IsMoveLegal(int x, int y, int iPlayer) const {
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

int COsBoard::UpdateDirection(int x, int y, int dx, int dy, char cMover, char cOpponent) {
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

int COsBoard::IsMoveLegalDirection(int x, int y, int dx, int dy, char cMover, char cOpponent) const {
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


void COsBoard::GetPieceCounts(int& nBlack, int& nWhite, int& nEmpty) const {
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

int COsBoard::NetBlackSquares() const {
	int nBlack, nWhite, nEmpty;
	GetPieceCounts(nBlack, nWhite, nEmpty);
	return nBlack-nWhite;
}

int COsBoard::Result(bool fAnti) const {
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
// COsGame
////////////////////////////////////

istream& COsGame::InLogbook(istream& is) {
	char cMover;

	DefaultValues();
	pis[0].sName=pis[1].sName="logtest";
	sPlace="logbook";

	// get moves
	CSGMoveListItem mli;
	mli.tElapsed=0;
	mli.dEval=0;
	while (is >> cMover) {
		if (cMover=='+' || cMover=='-') {

			// read move
			int iMover= cMover=='-';
			QSSERT(pos.board.iMover==iMover);
			mli.mv.resize(2);
			is >> mli.mv[0] >> mli.mv[1];

			// update game and pass if needed
			Update(mli);
			if (!GameOver() && !pos.board.HasLegalMove()) {
				mli.mv="PA";
				Update(mli);
			}
		}
		else {
			QSSERT(cMover==':');
			break;
		}
	}

	// get result
	int nResult;
	is >> nResult;
	result.Set(nResult);

	// game over flag
	int n;
	is >> n;
	QSSERT(n==10);

	return is;
}

// 772942166 r idiot    64 ( 30   0   0) TravisS   0 ( 30   0   0) +34-33+43-35+24-42+52-64+23-13+41-32+53-14+25-31+51-61+15-16+63-74+62-73+65-75+66-56+76-57+67-86+46-68+47-38+26-37+58-48+36 +0
istream& COsGame::InIOS(istream& is) {
	char c;
	time_t timestamp;
	int nBlack, nWhite;

	DefaultValues();
	sPlace="IOS";

	if (is >> timestamp) {
		// time. For early games no timestamp was stored, it is 0.
		if (timestamp)
			SetTime(timestamp);

		// game end type
		is >> c;
		switch(c) {
		case 'e':
			result.status=COsResult::kNormalEnd;
			break;
		case 'r':
			result.status=COsResult::kResigned;
			break;
		case 't':
			result.status=COsResult::kTimeout;
			break;
		}

		is >> pis[0].sName;
		is >> nBlack;
		posStart.cks[0].InIOS(is);

		is >> pis[1].sName;
		is >> nWhite;
		posStart.cks[1].InIOS(is);

		pos=posStart;

		// get moves
		CSGMoveListItem mli;
		mli.tElapsed=0;
		mli.dEval=0;
		int iosmove;

		// read move code. move code 0 means game is over
		while ((is >> iosmove) && iosmove) {
			// positive moves are black, negative are white
			QSSERT(pos.board.iMover==iosmove<0);
			CSGSquare sq;
			sq.IOSSet(iosmove);
			mli.mv=sq.StringGet();
			Update(mli);

			// pass if needed
			if (!GameOver() && !pos.board.HasLegalMove()) {
				mli.mv="PA";
				Update(mli);
			}
		}

		// calculate result. Might not be equal to the result
		//	on the board if one player resigned.
		result.dResult=nBlack-nWhite;
	}

	return is;
}

void COsGame::DefaultValues() {
	Clear();
	mt.bt.nWidth=mt.bt.nHeight=8;
	mt.bt.nOcto=0;
	posStart.board.bt=mt.bt;
	posStart.board.Initialize();

	pis[0].dRating=0;
	pis[1].dRating=0;

	posStart.cks[0].tCurrent=900;
	posStart.cks[0].iTimeout=0;
	posStart.cks[0].tGrace=120;
	posStart.cks[0].tIncrement=0;
	posStart.cks[1]=posStart.cks[0];
	pos=posStart;
}

istream& COsGame::InNtestMoveList(istream& is) {
	CSGMoveListItem mli;
	int nsq;
	int c;

	is.unsetf(ios::skipws);

	mli.dEval=mli.tElapsed=0;
	is >> ws;

	while (!GameOver() && (EOF!=(c=is.get()))) {
		// pass if needed
		if (!pos.board.HasLegalMove()) {
			mli.mv="PA";
			Update(mli);
		}
		nsq=c-' ';
		mli.mv=CSGSquare(nsq%8, nsq/8);
		Update(mli);

	}

	return is;
}

// -booklet  +10 booklet  EK23:=M5LTJF+S]\U^4R>,ON67GIZAB9-*[Y1#$?/"@H!8VQ)&0W_%'(XP .
istream& COsGame::InOldNtest(istream& isBig, int iVersion) {
	char c;

	string sLine;
	getline(isBig, sLine);
	if (sLine.empty())
		return isBig;
	istrstream is(sLine.c_str());

	DefaultValues();

	if (iVersion>=1) {
		is >> c;
		if (c=='-')
			sPlace="Local";
		else if (c=='+')
			sPlace="GGS/os";
		else
			QSSERT(0);

		is >> pis[0].sName;

		is >> result.dResult;
		result.status=CSGResult::kNormalEnd;

		is >> pis[1].sName;
	}
	else {
		sPlace="Local";
		pis[0].sName=pis[1].sName="";

		char nBlack, nWhite;
		is.unsetf(ios::skipws);
		is >> nBlack >> nWhite;
		result.dResult=nBlack-nWhite;
		result.status=CSGResult::kNormalEnd;
	}


	if (iVersion>=2) {
		// ignore initial board. works for non-rand games anyway.
		string sBoard;
		is >> sBoard;
		char cStart;
		is >> cStart;
	}
	
	InNtestMoveList(is);

	return isBig;
}

/*
according to instructions:

The format is as follows:

64 bytes per game:

  MoveNumber(1), GameResult(1), List of Moves(up to 60), 0-Byte(1), Flags(1)



But really, it is:

  Number of moves (1), Game Result (1), 0-Byte(1), List of Moves[60], 0-Byte(1)

Also.... the istream must be opened as binary

*/
istream& COsGame::InLogKitty(istream& is) {
	char moveNumber, gameResult, move, flags, whatsThis;

	// binary data
	is.unsetf(ios::skipws);

	Clear();
	mt.bt.nWidth=mt.bt.nHeight=8;
	mt.bt.nOcto=0;
	posStart.board.bt=mt.bt;
	posStart.board.Initialize();

	is >> moveNumber;
	if (!is)
		return is;
	sPlace="Log-Kitty";

	pis[0].sName=pis[1].sName="log-kitty";
	pis[0].dRating=pis[1].dRating=0;

	is >> gameResult;

	is >> whatsThis;

	posStart.cks[0].tCurrent=900;
	posStart.cks[0].iTimeout=0;
	posStart.cks[0].tGrace=120;
	posStart.cks[0].tIncrement=0;
	posStart.cks[1]=posStart.cks[0];
	pos=posStart;

	CSGMoveListItem mli;
	mli.dEval=mli.tElapsed=0;
	while ((is >> move) && move) {
		// Pass if needed
		if (!pos.board.HasLegalMove()) {
			mli.mv="PA";
			Update(mli);
		}

		// decode log/kitty moves
		int iMoverCheck=(unsigned char)move>>7;
		QSSERT(iMoverCheck==pos.board.iMover);
		int square=(move&0x7f)-1;
		if (square>=27)
			square+=2;
		if (square>=35)
			square+=2;
		mli.mv=CSGSquare(square&7, square>>3);
		Update(mli);
	}

	int nBlack, nWhite, nEmpty;
	pos.board.GetPieceCounts(nBlack, nWhite, nEmpty);
	// used sometimes for debugging game result
	// int nResult=pos.board.Result();
	int nNet=nBlack-nWhite;
	if (nNet!=(signed char)gameResult || moveNumber+4!=nBlack+nWhite) {
		pos.board.OutFormatted(cout);
		cout.flush();
		QSSERT(0);
	}

	// set result
	if (GameOver()) {
		result.dResult=(signed char)gameResult;
		result.status=CSGResult::kNormalEnd;
	}
	else {
		result.status=CSGResult::kUnfinished;
	}

	// remainder of moves are garbage, ignore
	while (nEmpty--) {
		is >> flags;
	}

	return is;
}

CSGGameBase* COsGame::NewCopy() const {
	return new COsGame(*this);
}

const string& CSGGame<COsPosition, COsMatchType>::GameName() const{
	return COsRules::sGameName;
}
