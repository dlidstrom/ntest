// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_CKS_OBJECTS_H
#define GDK_CKS_OBJECTS_H

#include "types.h"

#include <time.h>
#include <string>
#include <vector>
#include <iostream>
#include <map>
using namespace std;

#include "SGObjects.h"
  
class CCksBoardType : public CSGBoardTypeBase {
public:
	virtual void In(istream& is);
	virtual void Out(ostream& os) const;

	virtual void StringSet(const string& sText);
	virtual string StringGet() const;
};

class CCksMatchType : public CSGMatchType<CCksBoardType> {
public:
	CCksMatchType() {};
	CCksMatchType(const string& s) { StringSet(s); };

	void In(istream& is);
	void Out(ostream& os) const;

	string Description() const;

	void Clear();

	// error checking
	int	Validate() const;

	bool operator==(const CCksMatchType& b) const;

	double MaxResult() const;

	void GetTDOut(string& smtOut) const;
};

inline istream& operator>>(istream& is, CCksMatchType& mt) {mt.In(is); return is; }
inline ostream& operator<<(ostream& os, const CCksMatchType& mt)  { mt.Out(os); return os;}

typedef CSGResult CCksResult;

 class CCksBoard: public CSGBoard<CCksBoardType> {
public:
	enum { RED_MAN='b', RED_KING='B', WHITE_MAN='w', WHITE_KING='W' , EMPTY='-', DUMMY=' ', UNKNOWN='?' };

	void Initialize();

	virtual void Update(const string& sMove);
	void UpdatePiece(const CSGSquare& sqStart, const CSGSquare& sqEnd);

	void GetPieceCounts(int& nBlack, int& nWhite, int& nEmpty) const;

	int NetBlackSquares() const;
	int Result(bool fAnti=false) const;

	// Legal moves etc
	bool GameOver() const;
	vector<CSGSquare> GetStartSquares(int iPlayer) const;
	vector<CSGSquare> GetStartSquares(int iPlayer, bool& fJump) const;
	vector<CSGSquare> GetJumpEndSquares(const CSGSquare& start) const;
	vector<CSGSquare> GetSlideEndSquares(const CSGSquare& start) const;
	string GetForcedMove(int& iType) const;

	bool HasJumpSquare(int x, int y, int iPlayer) const;
	bool HasJumpSquareDirection(int x, int y, int dx, int dy, int iPlayer) const;
	bool HasSlideSquare(int x, int y, int iPlayer) const;

	static int PieceToPlayer(char c);
	static int PieceToKing(char c);

	static int DyMan(int iPlayer);

protected:
	void PushJumpSquare(vector<CSGSquare>& sqs, const CSGSquare& sqStart,int dx, int dy) const;
	void PushSlideSquare(vector<CSGSquare>& sqs, const CSGSquare& sqStart,int dx, int dy) const;
};

inline istream& operator>>(istream& is, CCksBoard& board) {board.In(is); return is; }
inline ostream& operator<<(ostream& os, const CCksBoard& board)  { board.Out(os); return os;}

typedef CSGPosition<CCksBoard> CCksPosition;

class CCksGame : public CSGGame<CCksPosition, CCksMatchType> {
public:
	CCksGame() {};
	CCksGame(istream& is) : CSGGame<CCksPosition, CCksMatchType>(is) {};

	CSGGameBase* NewCopy() const;
};

class CCksRules {
public:
	typedef CCksGame TGame;
	typedef CCksMatchType TMatchType;
	typedef CCksBoardType TBoardType;

	static const char* sLogin;
	static const string sGameName;
};


#endif //GDK_CKS_OBJECTS_H
