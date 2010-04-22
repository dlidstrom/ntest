// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_AMSOBJECTS_H
#define GDK_AMSOBJECTS_H

#include "types.h"

#include <time.h>
#include <string>
#include <vector>
#include <iostream>
#include <map>
using namespace std;

#include "SGObjects.h"

// basic classes that make up /os messages

/*
class CAmsMove {
public:
	// Creation
	CAmsMove();
	CAmsMove(int x, int y);
	CAmsMove(const string& sMove);

	// Modification
	void SetIOS(int iosmove);

	// I/O
	void In(istream& is);
	void Out(ostream& os) const;

	// Comparison
	bool operator==(const CAmsMove& b) const;
	bool operator<(const CAmsMove& b) const;

	bool fPass;
	int x,y;
};

inline CAmsMove::CAmsMove() {};
inline CAmsMove::CAmsMove(int ax, int ay) { x=ax; y=ay; fPass=false; }
inline istream& operator>>(istream& is, CAmsMove& mv) { mv.In(is); return is; }
inline ostream& operator<<(ostream& os, const CAmsMove &mv)  { mv.Out(os); return os; }
*/
  
class CAmsBoardType : public CSGBoardTypeBase {
public:
	virtual void In(istream& is);
	virtual void Out(ostream& os) const;

	virtual void StringSet(const string& sText);
	virtual string StringGet() const;
};

/*
class CAmsRatingType {
public:
	CAmsBoardType bt;
	bool fRand, fAnti;

	void In(istream& is);
	void Out(ostream& os) const;
};

inline istream& operator>>(istream& is, CAmsRatingType& rt) {rt.In(is); return is; }
inline ostream& operator<<(ostream& os, const CAmsRatingType& rt)  { rt.Out(os); return os;}
*/

class CAmsMatchType : public CSGMatchType<CAmsBoardType> {
public:

	CAmsMatchType() {};
	CAmsMatchType(const string& s) { StringSet(s); }
	void In(istream& is);
	void Out(ostream& os) const;

	string Description() const;

	void Clear();

	// error checking
	int	Validate() const;

	bool operator==(const CAmsMatchType& b) const;

	double MaxResult() const;

	void GetTDOut(string& smtOut) const;

protected:
	void NewColor(char c, const string& smt);	// can throw CError
};

inline istream& operator>>(istream& is, CAmsMatchType& mt) {mt.In(is); return is; }
inline ostream& operator<<(ostream& os, const CAmsMatchType& mt)  { mt.Out(os); return os;}

typedef CSGResult CAmsResult;

class CAmsBoard: public CSGBoard<CAmsBoardType> {
public:
	void Initialize();

	virtual void Update(const string& sMove);
	void UpdateXY(int x, int y);

	void GetPieceCounts(int& nBlack, int& nWhite, int& nEmpty) const;

	int NetBlackSquares() const;
	int Result(bool fAnti=false) const;

	// Legal moves etc
	bool GameOver() const;
	int NPass() const;
	vector<CSGSquare> GetMoves(bool fMover=true) const;
	bool HasLegalMove() const;

	virtual bool IsMoveLegal(const string& sMove) const;
	int IsMoveLegal(int x, int y) const;

	string GetForcedMove(int& iType) const;

protected:
	int UpdateDirection(int x, int y, int dRow, int dCol, char cMover, char cOpponent);
	int IsMoveLegalDirection(int x, int y, int dRow, int dCol, char cMover, char cOpponent) const;
	int IsMoveLegal(int x, int y, int iPlayer) const;
	bool HasLegalMove(int iPlayer) const;
};

inline istream& operator>>(istream& is, CAmsBoard& board) {board.In(is); return is; }
inline ostream& operator<<(ostream& os, const CAmsBoard& board)  { board.Out(os); return os;}
inline int CAmsBoard::IsMoveLegal(int x, int y) const { return IsMoveLegal(x, y, iMover); }
inline bool CAmsBoard::HasLegalMove() const { return HasLegalMove(iMover); }

typedef CSGPosition<CAmsBoard> CAmsPosition;

class CAmsGame : public CSGGame<CAmsPosition, CAmsMatchType> {
public:
	CAmsGame() {};
	CAmsGame(istream& is) : CSGGame<CAmsPosition, CAmsMatchType>(is) {};

	CSGGameBase* NewCopy() const;
};

class CAmsRules {
public:
	typedef CAmsGame TGame;
	typedef CAmsMatchType TMatchType;
	typedef CAmsBoardType TBoardType;
	typedef CSGBoard<TBoardType> TBoard;
	typedef CSGPosition<TBoard> TPosition;

	static const char* sLogin;
	static const string sGameName;
};

#endif //GDK_AMSOBJECTS_H
