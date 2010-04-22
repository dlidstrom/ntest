// Copyright 2001 Chris Welty
//	All Rights Reserved

#ifndef GDK_TYPES_H
#define GDK_TYPES_H


////////////////////////////////////////////////////////////
// Machine, compiler, framework dependent defines
////////////////////////////////////////////////////////////





//////////////////////////////////////////////////////////////
// Define QSSERT(x) macro which breaks into debugger if x is false
//	QSSERT(x) should probably NOT be defined as assert(x) since
//	the errors it catches are mostly recoverable errors
//////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////
// Microsoft Visual C
//	The QSSERT(x) and QSSERT2(x,string) macros used internally
//	are normally mapped to QSSERT(x) when debugging, and
//	to qssert(__FILE__,__LINE__,string) when sending files to
//	external parties. 
//////////////////////////////////////////////////////////////

int qssert(const char* fn, int line, const char* sDescription=0);

#if _MSC_VER	
#pragma warning(disable:4786)	// otherwise get STL errors
#pragma warning(disable:4127)	// conditional expression not constant warning because of QSSERT() function

#ifdef _DEBUG
#include "crtdbg.h"				// include _ASSERT() macro
#define QSSERT(x) _ASSERT(x)
#define QSSERT2(x,str) _ASSERT(x)
#else
#ifdef LION
#define QSSERT(x) ((!(x)) && qssert(__FILE__, __LINE__))
#define QSSERT2(x,str) ((!(x)) && qssert(__FILE__, __LINE__, str))
#else
#define QSSERT(x)
#define QSSERT2(x,str)
#endif
#endif

//////////////////////////////////////////////////////////////
// Non-Microsoft Visual C++:
//////////////////////////////////////////////////////////////

#elif defined(__GNUC__)	// Gnu C compiler
#define QSSERT(x) ((!(x)) && qssert(__FILE__, __LINE__))
#define QSSERT2(x,str) ((!(x)) && qssert(__FILE__, __LINE__, str))

#else	// unknown compiler
#define QSSERT(x) ((!(x)) && qssert(__FILE__, __LINE__))
#define QSSERT2(x,str) ((!(x)) && qssert(__FILE__, __LINE__, str))

#endif

////////////////////////////////////////////////////////////
// Typedefs
////////////////////////////////////////////////////////////

typedef unsigned short u2;
typedef unsigned long u4;

////////////////////////////////////////////////////////////
// Error codes
////////////////////////////////////////////////////////////

const u4 kErrMem=0x8100;
enum {kErrGDK=0x10000,
		kErrOs=kErrGDK+0x1000, kErrOsBadMatchType,
		kErrAms=kErrGDK+0x2000, kErrAmsBadMatchType,
};

#include <iostream>
#include <string>

using namespace std;

class CError {
public:
	CError(u4 errCode, const string& sErr);

	u4 errCode;
	string sErr;

	ostream& Out(ostream& os) const;
};

inline ostream& operator<<(ostream& os, const CError& error) { return error.Out(os);}
inline CError::CError(u4 aErrCode, const string& asErr) : errCode(aErrCode), sErr(asErr) {};
inline ostream& CError::Out(ostream& os) const {
	os << "ERR ";
	os << errCode;
	os << ": ";
	os << sErr.c_str();
	return os;
}; 


#endif // GDK_TYPES_H
