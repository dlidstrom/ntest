// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#include "PreCompile.h"
#include "types.h"
#include "ggsstream.h"
#include "GGSMessage.h"
#include "SGBase.h"

#include <string>
#include <strstream>
#include <iostream>


ggsstream::ggsstream() : iostream(NULL) {
	fLoggedIn=false;
	psockbuf=NULL;
}

ggsstream::~ggsstream() {
	if (IsLoggedIn())
		Logout();
	if (IsConnected())
		Disconnect();
	if (psockbuf) {
		delete psockbuf;
		psockbuf=NULL;
	}

	// clear services
	DestroyAllServices();
}

int ggsstream::Connect(const string& sServer, int nPort) {
	if(IsConnected()) {
		QSSERT(0);
		return kErrConnected;
	}
	
	int err;

	psockbuf=new sockbuf();
	if (psockbuf) {
		err=psockbuf->connect(sServer, nPort);
		if (!err)
			init(psockbuf);
		else {
			delete psockbuf;
			psockbuf=NULL;
		}
	}
	else
		err=kErrMem;

	return err;
}

int ggsstream::Disconnect() {
	if (!IsConnected()) {
		QSSERT(0);
		return kErrNotConnected;
	}
	setstate(ios::eofbit);
	if (psockbuf) {
		psockbuf->disconnect();
	}
	return 0;
}

// return 0 if no error
// 1 if socket err (e.g. connection timed out)
int ggsstream::Login(const std::string& sName, const std::string& sPassword) {
	int err=0;

	if (fLoggedIn) {
		QSSERT(0);
		err = kErrLoggedIn;
	}

	// await login prompt
	if (!err) {
		sLogin=sName;
		err = await("login");
	}

	// send login, await password prompt
	if (!err) {
		(*this) << sName << "\n";
		flush();
		err = await("password");
	}

	// send password, await response
	if (!err) {
		(*this) << sPassword << "\n";
		flush();
		err = await("\n");
	}

	// check to see if password was accepted
	if (!err) {
		int c;
		c=peek();
		switch(c) {
		case ':':
			err = kErrBadPassword;
			break;
		case 'R':
			fLoggedIn=true;
			break;
		default:
			QSSERT(0);
			err = kErrUnknown;
		}
	}

	// send fake "Login" message
	if (!err) {
		string sMsg(": ~login");
		Post(sMsg);
	}
	return err;
}

int ggsstream::Logout() {
	if (fLoggedIn==false) {
		QSSERT(0);
		return kErrLoggedOut;
	}
	else {
		(*this) << "quit\n";
		flush();
		fLoggedIn=false;
		return 0;
	}
}

int ggsstream::await(const char* sAwait) {
	string sLine;
	char c;

	while (get(c)) {
		sLine+=c;
		if (strstr(sLine.c_str(), sAwait))
			return 0;
	}
	
	sockbuf* psb = dynamic_cast<sockbuf*>(rdbuf());
	if (psb)
		return psb->Err();
	else {
		QSSERT(0);
		return kErrNoStreambuf;
	}
}

const string& ggsstream::GetLogin() const {
	return sLogin;
}

int ggsstream::GetLevel() const {
	map<string,int>::const_iterator i=loginToLevel.find(sLogin);
	if (i==loginToLevel.end())
		return -1;
	else
		return (*i).second;
}
	
// process incoming data from GGS. 'is' is a socket connection.
//	strip bells and '|' at the beginning of lines. Once we have an
//	entire message(terminated by "READY" on its own line), call Parse()
void ggsstream::Process(){
	string sLine;
	static bool fHasCR=false;
	char c;

	while (get(c)) {
		switch(c) {
		case '\a':

#ifdef _WIN32
		  MessageBeep((unsigned int)-1);
#else
		  cerr << '\a';	// will this beep the speaker?
#endif
		  break;
		case '\r':
			ProcessLine(sLine);
			break;
		case '\n':
			if (!fHasCR)
				ProcessLine(sLine);
			break;
		default:
			sLine+=c;
		}
		fHasCR=(c=='\r');
	}
	string sMsg(": ~disconnect");
	Post(sMsg);
}

void ggsstream::ProcessLine(string& sLine){
	if (sLine=="READY")
		ProcessMessage();
	else {
		// services receive messages with '\\' instead of '\n' as the line separator, convert to '\n'
		u4 pos;
		for (pos=0; (pos=sLine.find('\\',pos))!=sLine.npos; )
			sLine[pos]='\n'; 

		const char* pLine = sLine.c_str();

		if (!sMsg.empty()) {
			// GGS sometimes sends 2 messages without a READY
			if (pLine[0]==':') {
				ProcessMessage();
			}
			else {
				sMsg+="\n";

				// GGS sends '|' at the beginning of all tell lines
				// except the first, strip them
				if (pLine[0]=='|')
					pLine++;
			}
		}

		sMsg+=pLine;
	}

	sLine="";
}

// Gets a message. Calls GetMsgType() to create the new message;
//	the user must delete() the message, perhaps in the
//	Post() routine

void ggsstream::ProcessMessage() {
	Post(sMsg);
	sMsg="";
}

void ggsstream::Post(string& sMsg) {
	DirectMessage(sMsg);
}

void ggsstream::DirectMessage(string& sMsg) {
	istrstream is(sMsg.c_str());
	string sFrom;

	is >> sFrom >> ws;

	try {
		if (!sFrom.empty()) {
			// direct messages end in ':', channel messages don't
			if (sFrom.end()[-1]==':')
				sFrom.resize(sFrom.size()-1);

			CSGBase* pService = PService(sFrom);
			if (pService) {
				pService->Parse(sMsg, sFrom, is);
			}
			else if (sFrom=="")
				Parse(sMsg, sFrom, is);
			else
				HandleTell(CMsgGGSTell(sMsg, sFrom, is));
		}
	}
	catch(const CError& err) {
		(*this) << "t " << sFrom << " " << err << "\n";
	}
}

void ggsstream::Parse(const string& sRaw, const string& sFrom, istream& is) {
	string sMsgType;
	is >> sMsgType >> ws;

	if (sMsgType=="alias")
		HandleAlias(CMsgGGSAlias(sRaw, sFrom, is));
	else if (sMsgType=="ERR")
		HandleErr(CMsgGGSErr(sRaw, sFrom, is));
	else if (sMsgType=="finger")
		HandleFinger(CMsgGGSFinger(sRaw, sFrom, is));
	else if (sMsgType=="help")
		HandleHelp(CMsgGGSHelp(sRaw, sFrom, is));
	else if (sMsgType=="who")
		HandleWho(CMsgGGSWho(sRaw, sFrom, is));
	else if (sMsgType=="+" || sMsgType=="-") {
		bool fPlus= sMsgType=="+";
		HandleUserDelta(CMsgGGSUserDelta(sRaw, sFrom, is, fPlus));
	}
	// dummy messages
	else if (sMsgType=="~login")
		HandleLogin();
	else if (sMsgType=="~disconnect")
		HandleDisconnect();
	else
		HandleUnknown(CMsgGGSUnknown(sRaw, sFrom, is, sMsgType));
}

const char* ggsstream::ErrText(int err) {
	switch(err) {
	case kErrBadPassword:
		return "Your password is invalid, or someone has already chosen that login";
	case kErrLoggedIn:
		return "You are already logged into GGS";
	case kErrLoggedOut:
		return "You have already logged out of GGS";
	case kErrUnknown:
		return "Unknown GGS error";
	case kErrMem:
		return "Out of memory";
	default:
		return "(No text available for this error)";
	}
}

bool ggsstream::IsConnected() const {
	return psockbuf!=NULL && psockbuf->IsConnected();
}

bool ggsstream::IsLoggedIn() const {
	return fLoggedIn;
}

void ggsstream::BaseDisconnect() {
	fLoggedIn=false;
	DestroyAllServices();
	sLogin.erase();
}

void ggsstream::BaseLogin() {
	// required commands for ODK to work:
	(*this) << "ve -ack\n"		// turn off GGS Parser comments
			<< "notify + *\n";	// tell us when /os comes up/goes down

	flush();
}

void ggsstream::BaseUserDelta(const CMsgGGSUserDelta& msg) {
	if (msg.fPlus)
		loginToLevel[msg.sLogin]=msg.iLevel;
	else{
		map<string,int>::iterator i=loginToLevel.find(msg.sLogin);
		QSSERT(i!=loginToLevel.end());
		loginToLevel.erase(i);
	}

	// send initial service update messages if needed
	if (msg.fPlus) {
		CSGBase* pService=CreateService(msg.sLogin);
		if (pService) {
			int iLevel=GetLevel();
			if (iLevel>=0)
				pService->SendInitial(iLevel);
		}
		else if (msg.sLogin==GetLogin()) {
			map<string,CSGBase*>::iterator i;
			for (i=loginToPService.begin(); i!=loginToPService.end(); i++) {
				(*i).second->SendInitial(msg.iLevel);
			}
		}
	}
	else
		DestroyService(msg.sLogin);
}

///////////////////////////////////////
// Overridable handlers
///////////////////////////////////////

void ggsstream::Handle(const CMsg& msg) {
	cout << msg.sRawText << "\n";
}

void ggsstream::HandleAlias(const CMsgGGSAlias& msg) {
	Handle(msg);
}

void ggsstream::HandleDisconnect() {
	BaseDisconnect();
}

void ggsstream::HandleErr(const CMsgGGSErr& msg) {
	Handle(msg);
}

void ggsstream::HandleFinger(const CMsgGGSFinger& msg) {
	Handle(msg);
}

void ggsstream::HandleHelp(const CMsgGGSHelp& msg) {
	Handle(msg);
}

void ggsstream::HandleLogin() {
	BaseLogin();
}

void ggsstream::HandleTell(const CMsgGGSTell& msg) {
	Handle(msg);
}

void ggsstream::HandleUnknown(const CMsgGGSUnknown& msg) {
	Handle(msg);
}

void ggsstream::HandleUserDelta(const CMsgGGSUserDelta& msg) {
	BaseUserDelta(msg);
	Handle(msg);
}

void ggsstream::HandleWho(const CMsgGGSWho& msg) {
	Handle(msg);
}

// create a service, if the sLogin is a service type supported by this app
// returns:
//	if sUserLogin is not a service, returns NULL
//	if service already exists, returns NULL
//	if fails to create service, returns NULL
//	if creates service, returns pointer to service
/*
CSGBase* ggsstream::CreateService(const string& sUserLogin) {
	CSGBase* pService=NULL;
	if (!HasService(sLogin)) {
		if (sUserLogin=="/os") {
			pService=new CServiceOs(this);
		}
		if (pService)
			loginToPService[sUserLogin]=pService;
	}
	return pService;
}
*/
bool ggsstream::HasService(const string& sUserLogin) const {
	map<string, CSGBase*>::const_iterator i=loginToPService.find(sUserLogin);

	return i!=loginToPService.end();
}

// destroy a service, if it exists
void ggsstream::DestroyService(const string& sUserLogin) {
	map<string, CSGBase*>::iterator i=loginToPService.find(sUserLogin);

	if (i!=loginToPService.end()) {
		delete (*i).second;
		loginToPService.erase(i);
	}
}

void ggsstream::DestroyAllServices() {
	map<string, CSGBase*>::iterator i;
	for (i=loginToPService.begin(); i!=loginToPService.end(); i++)
		delete (*i).second;
	loginToPService.clear();
}

CSGBase* ggsstream::PService(const string& sUserLogin) {
	map<string, CSGBase*>::iterator i=loginToPService.find(sUserLogin);

	if (i!=loginToPService.end())
		return (*i).second;
	else
		return NULL;
}
