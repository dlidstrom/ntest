// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "types.h"
#include "GGSObjects.h"

#include <strstream>

using namespace std;

void CGGSAlias::In(istream& is) {
	is >> sAlias >> ws;
	getline(is, sExpansion);
}

void CGGSWhoUser::In(istream& is) {
	string sLine;
	getline(is, sLine);
	istrstream isl(sLine.c_str());
	isl >> sLogin >> cRegistered >> sIdle >> sOnline >> sIPAddr >> sHostName;
}

// used by Lion to sort players according to displayed character
int CGGSWhoUser::RegisteredSortOrder() const {
	switch(cRegistered) {
	case 's':
		return 0;
	case 'a':
		return 1;
	case 'h':
		return 2;
	case 'p':
		return 4;
	case '.':
		return 5;
	default:
		return 3;
	}
}

// funky ordering is used by Lion
bool CGGSWhoUser::operator<(const CGGSWhoUser& b) const {
	if (RegisteredSortOrder()!=b.RegisteredSortOrder())
		return RegisteredSortOrder()<b.RegisteredSortOrder();
	else
		return sLogin<b.sLogin;
}
