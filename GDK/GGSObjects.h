// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_GGSOBJECTS_H
#define GDK_GGSOBJECTS_H


#include "types.h"

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class CGGSAlias {
public:
	string sAlias, sExpansion;

	void In(istream& is);

	bool operator<(const CGGSAlias& b) const { return this<&b ; }
	bool operator==(const CGGSAlias& b) const { return this==&b; }
};

inline istream& operator>>(istream& is, CGGSAlias& alias) {alias.In(is); return is; }

class CGGSWhoUser {
public:
	string sLogin;
	char cRegistered;
	string sIdle, sOnline, sIPAddr, sHostName;

	void In(istream& is);

	bool operator<(const CGGSWhoUser& b) const;
	bool operator==(const CGGSWhoUser& b) const { return sLogin==b.sLogin; }
private:
	int RegisteredSortOrder() const;
};

inline istream& operator>>(istream& is, CGGSWhoUser& wu) { wu.In(is); return is; }

#endif	//GDK_GGSOBJECTS_H
