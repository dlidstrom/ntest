// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "types.h"
#include "GGSMessage.h"
#include "ggsstream.h"

#include <strstream>
#include <iomanip>
#include <ctype.h>

using namespace std;

///////////////////////////////////
// CMsg
///////////////////////////////////

CMsg::CMsg(const string& sMsg, const string& asFrom) {
	sRawText=sMsg;
	sFrom=asFrom;
}

///////////////////////////////////
// CMsgGGSAlias
///////////////////////////////////

CMsgGGSAlias::~CMsgGGSAlias() {
}

/*
: alias Gunnar 30/30
...
OR
: alias 1/1
*/
CMsgGGSAlias::CMsgGGSAlias(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom) {
	char c;
	CGGSAlias alias;
	string sLine;
	getline(is, sLine);
	istrstream isLine(sLine.c_str());

	// login, sometimes sent, sometimes not
	if (isalpha(isLine.peek()))
		isLine >> sLogin;

	// number of aliases. Sometimes sent, sometimes not.
	bool fHasNum=isdigit(isLine.peek())!=0;
	if (fHasNum) {
		isLine >> nPermanent >> c >> nTotal;
		QSSERT(c=='/');
	}
	else
		nPermanent=nTotal=0;

	while (is >> alias)
		valiases.push_back(alias);

	
	if (fHasNum)
		QSSERT(nTotal==valiases.size());
}

///////////////////////////////////
// CMsgGGSErr
///////////////////////////////////

CMsgGGSErr::~CMsgGGSErr() {
}

// : ERR user 'rated' not found.
CMsgGGSErr::CMsgGGSErr(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom)  {
	string sLine;
	getline(is, sLine);
	if (sLine.find("not recognized")!=sLine.npos)
		err=kErrCommandNotRecognized;
	else if (sLine.find("user")!=sLine.npos && sLine.find("not found")!=sLine.npos) {
		err=kErrUserNotFound;
		u4 nPos1=sLine.find_first_of('\'');
		if (nPos1!=sLine.npos) {
			u4 nPos2=sLine.find_first_of('\'',nPos1+1);
			if (nPos2!=sLine.npos) {
				sParam=sLine.substr(nPos1+1, nPos2-nPos1-1);
			}
		}
		QSSERT(!sParam.empty());
	}
	else
		err=kErrUnknown;
}

///////////////////////////////////
// CMsgGGSFinger
///////////////////////////////////

CMsgGGSFinger::~CMsgGGSFinger() {
}

CMsgGGSFinger::CMsgGGSFinger(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom)  {
	string sLine, sKey, sValue;

	is >> ws;

	// first section, key : value
	while (getline(is, sLine)) {
		if (!is)
			break;
		if (sLine.find(':')==string::npos)
			break;

		istrstream isLine(sLine.c_str());

		// get key and strip terminal spaces
		getline(isLine, sKey, ':');
		sKey.resize(1+sKey.find_last_not_of(' '));

		// get value. No value means this is the separator row
		//	between the first section and the second section
		isLine >> ws;
		getline(isLine, sValue);

		// insert (key, value) pair
		map<string,string>::iterator i=keyToValue.find(sKey);
		if (i==keyToValue.end())
			keyToValue[sKey]=sValue;
		else {
			(*i).second+="\n";
			(*i).second+=sValue;
		}
	}

}

///////////////////////////////////
// CMsgGGSHelp
///////////////////////////////////

CMsgGGSHelp::~CMsgGGSHelp() {
}

CMsgGGSHelp::CMsgGGSHelp(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom)  {
	getline(is, sText, (char)EOF);
}

///////////////////////////////////
// CMsgGGSTell
///////////////////////////////////

CMsgGGSTell::CMsgGGSTell(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom)  {
	getline(is, sText, (char)EOF);
	sFrom=asFrom;
}

CMsgGGSTell::~CMsgGGSTell() {
}

///////////////////////////////////
// CMsgGGSUnknown
///////////////////////////////////

CMsgGGSUnknown::~CMsgGGSUnknown() {
}

CMsgGGSUnknown::CMsgGGSUnknown(const string& sMsg, const string& asFrom, istream& is, const string& sType) : CMsg(sMsg, asFrom)  {
	getline(is, sText, (char)EOF);
	sMsgType=sType;
}

///////////////////////////////////
// CMsgGGSUserDelta
///////////////////////////////////

CMsgGGSUserDelta::~CMsgGGSUserDelta() {
}

CMsgGGSUserDelta::CMsgGGSUserDelta(const string& sMsg, const string& asFrom, istream& is, bool afPlus) : CMsg(sMsg, asFrom)  {
	fPlus=afPlus;
	is >> sLogin >> iLevel;
}

///////////////////////////////////
// CMsgGGSWho
///////////////////////////////////

CMsgGGSWho::~CMsgGGSWho() {
}

CMsgGGSWho::CMsgGGSWho(const string& sMsg, const string& asFrom, istream& is) : CMsg(sMsg, asFrom)  {
	CGGSWhoUser wu;

	is >> nUsers;
	is.ignore(1000, '\n');
	while (is >> wu) {
		wus.push_back(wu);
	}
	QSSERT(wus.size()==nUsers);
}
