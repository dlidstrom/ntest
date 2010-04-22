// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "SGBase.h"
#include "ggsstream.h"

#ifdef _WIN32
#include <crtdbg.h>
#endif

CSGBase::CSGBase(ggsstream* apgs,const string& asLogin) {
	sLogin=asLogin;
	pgs=apgs;
}

CSGBase::~CSGBase() {
}

void CSGBase::Parse(const string& sRaw, const string& sFrom, istream& is) {
	QSSERT(0);
}

ggsstream& CSGBase::Tell() const {
	(*pgs) << "t " << sLogin << " ";
	return *pgs;
}

void CSGBase::SendInitial(int iLevel) {
	if (iLevel>0)
		Tell() << "rated +\n";
	else
		Tell() << "open 1\n";

	Tell() << "match\n"
		<< flush;
}

CSGMatch* CSGBase::PMatch(const string& idm) {
	map<string,CSGMatch>::iterator i=idToMatch.find(idm);
	if (i==idToMatch.end())
		return NULL;
	else
		return &((*i).second);
}

#include "OsObjects.h"
#include "CksObjects.h"
#include "AmsObjects.h"
#include "SG_T.h"

template class CSG<COsRules>;
template class CSG<CAmsRules>;
template class CSG<CCksRules>;
