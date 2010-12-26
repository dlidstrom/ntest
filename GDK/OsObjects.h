// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_OSOBJECTS_H
#define GDK_OSOBJECTS_H

#include "types.h"
#include "SGObjects.h"

#include <time.h>
#include <string>
#include <vector>
#include <iostream>
#include <map>

// basic classes that make up /os messages

/*
class COsMove {
public:
	// Creation
	COsMove();
	COsMove(int x, int y);
	COsMove(const string& sMove);

	// Modification
	void SetIOS(int iosmove);

	// I/O
	void In(istream& is);
	void Out(ostream& os) const;

	// Comparison
	bool operator==(const COsMove& b) const;
	bool operator<(const COsMove& b) const;

	bool fPass;
	int x,y;
};

inline COsMove::COsMove() {};
inline COsMove::COsMove(int ax, int ay) { x=ax; y=ay; fPass=false; }
inline istream& operator>>(istream& is, COsMove& mv) { mv.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsMove &mv)  { mv.Out(os); return os; }
*/
  
class COsBoardType : public CSGBoardTypeBase {
public:
	virtual void In(istream& is);
	virtual void Out(ostream& os) const;

	virtual void StringSet(const string& sText);
	virtual string StringGet() const;
};

/*
class COsRatingType {
public:
	COsBoardType bt;
	bool fRand, fAnti;

	void In(istream& is);
	void Out(ostream& os) const;
};

inline istream& operator>>(istream& is, COsRatingType& rt) {rt.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsRatingType& rt)  { rt.Out(os); return os;}
*/

class COsMatchType : public CSGMatchType<COsBoardType> {
public:

	COsMatchType() {};
	COsMatchType(const string& s) { StringSet(s); }
	void In(istream& is);
	void Out(ostream& os) const;

	string Description() const;
	void GetTDOut(string& smtOut) const;

	void Clear();

	// error checking
	int	Validate() const;

	bool operator==(const COsMatchType& b) const;

	double MaxResult() const;

protected:
	void NewColor(char c, const string& smt);	// can throw CError
};

inline istream& operator>>(istream& is, COsMatchType& mt) {mt.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsMatchType& mt)  { mt.Out(os); return os;}

typedef CSGResult COsResult;

class COsBoard: public CSGBoard<COsBoardType> {
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

inline istream& operator>>(istream& is, COsBoard& board) {board.In(is); return is; }
inline ostream& operator<<(ostream& os, const COsBoard& board)  { board.Out(os); return os;}
inline int COsBoard::IsMoveLegal(int x, int y) const { return IsMoveLegal(x, y, iMover); }
inline bool COsBoard::HasLegalMove() const { return HasLegalMove(iMover); }

typedef CSGPosition<COsBoard> COsPosition;

class COsGame : public CSGGame<COsPosition, COsMatchType> {
public:
	COsGame() {};
	COsGame(istream& is) : CSGGame<COsPosition, COsMatchType>(is) {};

	CSGGameBase* NewCopy() const;

	istream& InLogbook(istream& is);	// read logbook.gam
	istream& InIOS(istream& is);		// read ios game file
	istream& InOldNtest(istream& is, int iVersion);	// ntest's file format before GGF
	istream& InLogKitty(istream& is);	// read log/kitty game file

protected:
	istream& InNtestMoveList(istream& is);
	void DefaultValues();

};

class COsRules {
public:
	typedef COsGame TGame;
	typedef COsMatchType TMatchType;
	typedef COsBoardType TBoardType;
	typedef CSGBoard<TBoardType> TBoard;
	typedef CSGPosition<TBoard> TPosition;

	static const char* sLogin;
	static const string sGameName;
};

#endif //GDK_OSOBJECTS_H
