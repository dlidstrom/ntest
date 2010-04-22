// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include <strstream>
#include <iomanip>
using namespace std;

/////////////////////////////////////////////
// CSGMatchType
/////////////////////////////////////////////

template<class TBoardType>
string CSGMatchType<TBoardType>::StringGet() const {
	ostrstream os;
	Out(os);
	os << '\0';
	string sResult(os.str());
	os.rdbuf()->freeze(0);
	return sResult;
}

template<class TBoardType>
void CSGMatchType<TBoardType>::StringSet(const string& sMatchType) {
	istrstream is(sMatchType.c_str());
	In(is);
}

template<class TBoardType>
string CSGMatchType<TBoardType>::Description() const {
	string s;
	s=bt.Description();
	switch(cColor) {
	case 'b':
		s+=" black";
		break;
	case 'w':
		s+=" white";
		break;
	case 'k':
		s+=" komi";
		break;
	case 's':
		s+=" synch";
		break;
	}
	if (fRand) {
		char buf[18];
		s+=" rand(";
		s+=itoa(nRandDiscs, buf, 10);
		s+=")";
	}
	if (fAnti)
		s+=" anti";

	return s;
}

template<class TBoardType>
bool CSGMatchType<TBoardType>::operator==(const CSGMatchType& b) const {
	return cColor==b.cColor &&
		 fRand==b.fRand &&
		 fAnti==b.fAnti &&
		 *bt==*(b.bt) &&
		 (fRand?(nRandDiscs==b.nRandDiscs):true);
}

template<class TBoardType>
void CSGMatchType<TBoardType>::Clear() {
	fRand=fAnti=false;
	cColor='?';
	bt.Clear();
}
		
template<class TBoardType>
int CSGMatchType<TBoardType>::Validate() const {
	// rand errors
	if (fRand) {
		if (cColor!='s' && cColor!='k')
			return kErrRandUnbalanced;
		if (nRandDiscs > bt.NRandDiscsMax())
			return kErrTooManyRandDiscs;
		else if (nRandDiscs < bt.NRandDiscsMin())
			return kErrTooFewRandDiscs;
	}

	return 0;
}

template<class TBoardType>
double CSGMatchType<TBoardType>::MaxResult() const {
	return bt.NPlayableSquares();
}
/////////////////////////////////
// CSGRequest
/////////////////////////////////

// 1735.1 pamphlet 15:00//02:00        8 U 1345.6 ant
// 1906.0 n3       01:00:00//02:00       8b S 1382.1 n2       .64399
// how to parse this if clocks are different in stored match?
template<class TRules>
void CSGRequest<TRules>::In(istream& is) {
	is >> pis[0] >> cks[0] >> mt;
	is >> cType;
	is >> pis[1];

	if (cType=='S')
		is >> idsm;
	else {
		is >> cks[1];
		if (!is)
			cks[1]=cks[0];
	}
}

/////////////////////////////////
// CSGBoard
/////////////////////////////////

template <class TBoardType>
void CSGBoard<TBoardType>::Initialize() {
	int x,y;
	char c;

	sBoard.resize(bt.NTotalSquares());
	for (y=-1;y<=bt.nHeight; y++) {
		for (x=-1; x<=bt.nWidth; x++) {
			if (x==-1 || y==-1 || x==bt.nWidth || y==bt.nHeight)
				c=DUMMY;
			else if (bt.DummyCorner(x,y))
				c=DUMMY;
			else
				c=EMPTY;
			PieceSet(x,y,c);
		}
	}

	iMover=0;
}

template <class TBoardType>
int CSGBoard<TBoardType>::Loc(int x, int y) const {
	return x+y*(bt.nWidth+2);
}

template <class TBoardType>
char CSGBoard<TBoardType>::PieceGet(int x, int y) const {
	// need to adjust for the first x and yumn of dummy squares
	x++;
	y++;
	return sBoard[Loc(x,y)];
}

template <class TBoardType>
char CSGBoard<TBoardType>::PieceGet(const CSGSquare& sq) const {
	return PieceGet(sq.x, sq.y);
}

template <class TBoardType>
void CSGBoard<TBoardType>::PieceSet(int x, int y, char piece) {
	// need to adjust for the first x and yumn of dummy squares
	x++;
	y++;
	sBoard[Loc(x,y)]=piece;
}

template <class TBoardType>
void CSGBoard<TBoardType>::PieceSet(const CSGSquare& sq, char piece) {
	PieceSet(sq.x, sq.y, piece);
}

template <class TBoardType>
void CSGBoard<TBoardType>::In(istream& is) {
	string sBoardType;
	char c;
	int i, nsq;

	Clear();

	bt.In(is);
	Initialize();
	nsq=bt.NTotalSquares();
	for (i=0; i<nsq; i++) {
		// find the next non-dummy square
		while (sBoard[i]=='d' && i<nsq)
			i++;
		if (i==nsq)
			break;

		// put something there
		is >> ws >> c;
		sBoard[i]=c;
	}

	QSSERT(is);

	is >> ws >> c;

	iMover=c=='O';
}

template <class TBoardType>
void CSGBoard<TBoardType>::Out(ostream& os) const {
	os << bt.StringGet() << " ";
	int i;
	for (i=0; i<bt.NTotalSquares(); i++) {
		if (sBoard[i]!=DUMMY)
			os << sBoard[i];
	}
	os << " " << CMover();
}

template <class TBoardType>
void CSGBoard<TBoardType>::Clear() {
	bt.Clear();
	iMover=0;
	sBoard.erase();
}

template <class TBoardType>
void CSGBoard<TBoardType>::OutFormatted(ostream& os) const {
	OutHeader(os);

	int y,x;
	for (y=0; y<bt.nHeight; y++) {
		os << setw(2) << y+1;
		for (x=0; x<bt.nWidth; x++) {
			os << " " << PieceGet(x,y);
		}
		os << setw(2) << y+1 << "\n";
	}

	OutHeader(os);
	os << (iMover?"White":"Black") << " to move\n";
}

template <class TBoardType>
void CSGBoard<TBoardType>::OutHeader(ostream& os) const {
	int x;

	os << "  ";
	for (x=0; x<bt.nWidth; x++)
		os << " " << (char)(x+'A');
	os << "  \n";
}

template <class TBoardType>
char* CSGBoard<TBoardType>::TextGet(char* sBoard, int &aiMover, bool fTrailingNull) const {
	int x, y;
	char* p;

	p=sBoard;
	for (y=0; y<=bt.nWidth; y++) {
		for (x=0; x<=bt.nHeight; x++) {
			if (PieceGet(x,y)!=DUMMY) {
				*p=PieceGet(x,y);
				p++;
			}
		}
	}
	if (fTrailingNull)
		*p='\0';

	aiMover=iMover;

	QSSERT(p-sBoard==bt.NPlayableSquares());
	return sBoard;
}

template <class TBoardType>
void CSGBoard<TBoardType>::TextSet(const char* sBoard) {
	int y, x;
	const char* p;

	p=sBoard;
	for (y=0; y<=bt.nHeight; y++) {
		for (x=0; x<=bt.nWidth; x++) {
			if (PieceGet(x,y)!=DUMMY) {
				PieceSet(x,y,*p);
				p++;
			}
		}
	}
}

template <class TBoardType>
char CSGBoard<TBoardType>::CMover() const {
	return char(iMover?WHITE:BLACK);
}

template <class TBoardType>
const CSGBoardTypeBase& CSGBoard<TBoardType>::BT() const {
	return bt;
}

/////////////////////////////////////
// CSGPosition
/////////////////////////////////////

template <class TBoard>
void CSGPosition<TBoard>::Update(const CSGMoveListItem& mli) {
	cks[board.iMover].Update(mli.tElapsed);
	board.Update(mli.mv);
}

template <class TBoard>
void CSGPosition<TBoard>::Update(const CSGMoveList& ml, u4 nMoves) {
	u4 i;
	if (nMoves>ml.size())
		nMoves=ml.size();
	for (i=0; i<nMoves; i++)
		Update(ml[i]);
}

// UpdateKomiSet() is called in a komi game to set the first move
//	choices of both players.
// It updates the clock of the nonmover. The mover's clock
//	is updated by a call to Update().

template <class TBoard>
void CSGPosition<TBoard>::UpdateKomiSet(const CSGMoveListItem mlis[2]) {
	bool iMoverOpponent=!board.iMover;
	cks[iMoverOpponent].Update(mlis[iMoverOpponent].tElapsed);
}

/*
void CSGPosition<TBoard>::Calculate(const CSGGame& game, int nMoves) {
	(*this)=game.posStart;
	if (nMoves && game.mt.color=='k') {
		QSSERT(!game.NeedsKomi());
		UpdateKomiSet(game.mlisKomi);
	}
	Update(game.ml, nMoves);
}
*/

template <class TBoard>
void CSGPosition<TBoard>::Clear() {
	board.Clear();
	cks[0].Clear();
	cks[1].Clear();
}

template <class TBoard>
const CSGBoardBase& CSGPosition<TBoard>::Board() const {
	return board;
}

////////////////////////////////////
// CSGGame
////////////////////////////////////


template <class TPosition, class TMatchType>
bool CSGGame<TPosition, TMatchType>::NeedsKomi() const {
	return mt.cColor=='k' && ml.empty();
}

template <class TPosition, class TMatchType>
double CSGGame<TPosition, TMatchType>::KomiValue() const {
	if (mt.cColor=='k' && !ml.empty())
		return (mlisKomi[0].dEval+mlisKomi[1].dEval)/2;
	else
		return 0;
}

template <class TPosition, class TMatchType>
void CSGGame<TPosition, TMatchType>::InPartial(istream& is) {
	char c;
	string sToken, sData;
	bool fCheckKomiValue = false;
	double dKomiValue;
	// we keep track of the player to move so that we can imply passes
	//	for instance edax writes the files without passes.
	// The first mover color varies from game to game so keep it '\0' until we know
	char cMover='\0';

	Clear();

	// Game tokens
	while (is >> ws) {
		if (is.peek()==';')
			break;
		getline(is, sToken, '[');
		getline(is, sData, ']');
		istrstream is(sData.c_str());

		if (sToken=="GM") {
			if (sData=="Othello")
				sData="Reversi";
			QSSERT(sData==GameName());
		}
		else if (sToken=="PC")
			sPlace=sData;
		else if (sToken=="DT")
			sDateTime=sData;
		else if (sToken=="PB")
			pis[0].sName=sData;
		else if (sToken=="PW")
			pis[1].sName=sData;
		else if (sToken=="RB")
			is >> pis[0].dRating;
		else if (sToken=="RW")
			is >> pis[1].dRating;
		else if (sToken=="TI") {
			is >> posStart.cks[0];
			posStart.cks[1]=posStart.cks[0];
		}
		else if (sToken=="TB")
			is >> posStart.cks[0];
		else if (sToken=="TW")
			is >> posStart.cks[1];
		else if (sToken=="TY")
			mt.In(is);
		else if (sToken=="BO")
			posStart.board.In(is);
		else if (sToken=="B" || sToken=="W") {
			CSGMoveListItem mli;

			// imply a pass if the same player moved twice
			// this is because edax writes the files wrong
			if (cMover==sToken[0]) {
				istrstream isPass("PA");
				isPass >> mli;
				ml.Update(mli);
			}
			cMover=sToken[0];

			is >> mli;
			ml.Update(mli);
		}
		else if (sToken=="RE")
			is >> result;
		else if (sToken=="CO")
			;	// ignore comments
		else if (sToken=="KB")
			is >> mlisKomi[0];
		else if (sToken=="KW")
			is >> mlisKomi[1];
		else if (sToken=="KM") {
			is >> dKomiValue;
			fCheckKomiValue=true;
		}
		else // unknown token
			QSSERT(0);
	}
	if (fCheckKomiValue) {
		double dErr = 2*dKomiValue - mlisKomi[0].dEval - mlisKomi[1].dEval;
		QSSERT(0.01 > dErr && dErr > -0.01);
	}

	if (is) {
		is >> c;
		QSSERT(c==';');
		is >> c;
		QSSERT(c==')');
	}

	// Get the current position
	CalcCurrentPos();
}

template <class TPosition, class TMatchType>
void CSGGame<TPosition, TMatchType>::Out(ostream& os) const {
	os << "(;GM[Reversi]";
	os << "PC[" << sPlace;
	if (!sDateTime.empty())
		os << "]DT[" << sDateTime;
	os << "]PB[" << pis[0].sName;
	os << "]PW[" << pis[1].sName;
	os << "]RE[" << result;

	if (pis[1].dRating)
		os << "]RB[" << pis[0].dRating;
	if (pis[0].dRating)
		os << "]RW[" << pis[1].dRating;
	if (posStart.cks[0]==posStart.cks[1]) {
		os << "]TI[" << posStart.cks[0];
	}
	else {
		os << "]TB[" << posStart.cks[0];
		os << "]TW[" << posStart.cks[1];
	}
	os << "]TY[" << mt.StringGet();
	os << "]BO[";
	posStart.board.Out(os);
	
	// komi set moves
	if (mt.cColor=='k' && !ml.empty()) {
		os << "]KB[" << mlisKomi[0];
		os << "]KW[" << mlisKomi[1];
		os << "]KM[" << (mlisKomi[0].dEval + mlisKomi[1].dEval)/2;
	}

	// move list
	vector<CSGMoveListItem>::const_iterator pmli;
	int iMover=posStart.board.iMover;
	for (pmli=ml.begin(); pmli!=ml.end(); pmli++) {
		os << (iMover?"]W[":"]B[") << *pmli;
		iMover=!iMover;
	}

	os << "];)";
}

template <class TPosition, class TMatchType>
void CSGGame<TPosition, TMatchType>::Clear() {
	result.Clear();
	ml.clear();
	pis[0].Clear();
	pis[1].Clear();
	pos.Clear();
	posStart.Clear();
	sDateTime.erase();
	sPlace.erase();
	mt.Clear();
}

template <class TPosition, class TMatchType>
void CSGGame<TPosition, TMatchType>::Undo() {
	if (ml.size()>=2) {
		ml.resize(ml.size()-2);
		CalcCurrentPos();
	}
}

template <class TPosition, class TMatchType>
void CSGGame<TPosition, TMatchType>::CalcCurrentPos() {
	CalcPos(pos, ml.size());
}

template <class TPosition, class TMatchType>
void CSGGame<TPosition, TMatchType>::CalcPos(TPosition& posCalc, u4 nMoves) const {
  if (nMoves>ml.size())
		nMoves=ml.size();
	posCalc=posStart;
	if (mt.cColor=='k' && nMoves)
		posCalc.UpdateKomiSet(mlisKomi);
	posCalc.Update(ml, nMoves);
}

template <class TPosition, class TMatchType>
void CSGGame<TPosition, TMatchType>::Update(const CSGMoveListItem& mli) {
	pos.Update(mli);
	ml.Update(mli);
	if (GameOver()) {
		// we don't adjust for timeouts because we don't keep track of
		//	who timed out first. This is a bug; but in games coming from services
		//	it should update us with the final result later.
		//result.dResult=pos.board.Result(mt.fAnti);
		result.status=CSGResult::kAwaitingResult;
		result.dResult=pos.board.Result(mt.fAnti)-KomiValue();
	}
}

template <class TPosition, class TMatchType>
void CSGGame<TPosition, TMatchType>::UpdateKomiSet(const CSGMoveListItem mlis[2]) {
	if (NeedsKomi()) {
		pos.UpdateKomiSet(mlis);
		mlisKomi[0]=mlis[0];
		mlisKomi[1]=mlis[1];
	}
	else
		QSSERT(0);
}

template <class TPosition, class TMatchType>
bool CSGGame<TPosition, TMatchType>::GameOverOnBoard() const {
	return pos.board.GameOver();
}

template <class TPosition, class TMatchType>
bool CSGGame<TPosition, TMatchType>::GameOver() const {
	return pos.board.GameOver() || result.GameOver();
}

template <class TPosition, class TMatchType>
void CSGGame<TPosition, TMatchType>::GetEstimatedTimes(CSGClock cksRemaining[2]) const {
	int i;
	int iMover=pos.board.iMover;

	for (i=0; i<2; i++) {
		cksRemaining[i]=pos.cks[i];
		if (!GameOver() && iMover==i)
			cksRemaining[i].Update(TimeSinceLastMove(), false);
	}
}

template<class TPosition, class TMatchType>
bool CSGGame<TPosition, TMatchType>::IsSynchro() const {
	return mt.cColor=='s';
}

template<class TPosition, class TMatchType>
const CSGPositionBase& CSGGame<TPosition, TMatchType>::Pos() const {
	return pos;
}

template<class TPosition, class TMatchType>
const CSGMatchTypeBase& CSGGame<TPosition, TMatchType>::MT() const{
	return mt;
}

template<class TPosition, class TMatchType>
CSGPositionBase* CSGGame<TPosition, TMatchType>::PosCopy(u4 iMoves) const {
  if (iMoves==ml.size())
		return new TPosition(pos);
	else {
		TPosition* pPos=new TPosition(posStart);
		if (pPos)
			CalcPos(*pPos, iMoves);
		return pPos;
	}
}
