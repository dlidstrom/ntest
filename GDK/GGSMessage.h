// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_GGSMESSAGE_H
#define GDK_GGSMESSAGE_H

#include "types.h"

#include <map>
#include "GGSObjects.h"

// base message class

class ggsstream;

class CMsg {
public:
	CMsg(const string& sMsg, const string& asFrom);

	string sRawText, sFrom;
};

class CMsgGGSAlias: public CMsg {
public:
	CMsgGGSAlias(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgGGSAlias();

	string sLogin;
	u4 nPermanent, nTotal;
	vector<CGGSAlias> valiases;
};

class CMsgGGSErr: public CMsg {
public:
	CMsgGGSErr(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgGGSErr();

	enum { kErrUnknown=0x8400, kErrCommandNotRecognized, kErrUserNotFound } err;
	string sParam;
};

class CMsgGGSFinger: public CMsg {
public:
	CMsgGGSFinger(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgGGSFinger();

	map<string, string> keyToValue;
};

class CMsgGGSHelp: public CMsg {
public:
	CMsgGGSHelp(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgGGSHelp();

	string sText;
};

class CMsgGGSUnknown: public CMsg {
public:
	CMsgGGSUnknown(const string& sMsg, const string& asFrom, istream& is, const string& asMsgType);
	virtual ~CMsgGGSUnknown();

	string sMsgType;
	string sText;
};

class CMsgGGSUserDelta: public CMsg {
public:
	CMsgGGSUserDelta(const string& sMsg, const string& asFrom, istream& is, bool fPlus);
	virtual ~CMsgGGSUserDelta();

	bool fPlus;
	string sLogin;
	int iLevel;
};

class CMsgGGSTell : public CMsg {
public:
	CMsgGGSTell(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgGGSTell();

	string sText;
};

class CMsgGGSWho : public CMsg {
public:
	CMsgGGSWho(const string& sMsg, const string& asFrom, istream& is);
	virtual ~CMsgGGSWho();

	u4 nUsers;
	vector<CGGSWhoUser> wus;
};

#endif // GDK_GGSMESSAGE_H
