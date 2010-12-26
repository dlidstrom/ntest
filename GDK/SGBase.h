// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_SERVICE_H
#define GDK_SERVICE_H

#include <string>

#include <istream>

#include <map>
using namespace std;

#include "SGObjects.h"

class ggsstream;

class CSGBase {
public:
	CSGBase(ggsstream* apgs, const string& asLogin);
	virtual ~CSGBase();

	virtual void Parse(const string& sRaw, const string& sFrom, istream& is);
	ggsstream& Tell() const;

	ggsstream* pgs;

	virtual void SendInitial(int iLevel);	// sent when we know user level and service logged in
	virtual int WatchEnd(const string& idm) = 0;
	virtual int ViewerUpdate(const string& idm, const CSGMoveListItem& mli) = 0;

	virtual CSGGameBase* PGame(const string& idg) = 0;
	virtual CSGMatch* PMatch(const string& idm);
	virtual void TDType(const string& smt, string& smtOut, string& smtDescription, double& dMaxResult) const =0;

	// persistent data
	map<string,CSGMatch> idToMatch;

protected:
	string sLogin;	// service login
};


#endif //GDK_SERVICE_H
