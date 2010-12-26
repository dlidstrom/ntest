// Copyright 2001 Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#ifndef GDK_GGSSTREAM_H
#define GDK_GGSSTREAM_H

#include "sockbuf.h"
#include "GGSMessage.h"
#include "SGBase.h"

#include <set>
#include <map>

class CMsg;
class CSGBase;

class ggsstream : public std::iostream {
public:
	// construction/destruction
	ggsstream();
	virtual ~ggsstream();

	// Connection, disconnection
	virtual int Connect(const std::string& sServer, int nPort);
	virtual int Disconnect();

	// login, logout
   virtual int Login(const std::string& sLogin, const std::string& sPassword);
	virtual int Logout();
	virtual const std::string& GetLogin() const;
	virtual int GetLevel() const;

	// turn stream data into messages
	virtual void Process();

	// Information
	virtual bool IsConnected() const;
	virtual bool IsLoggedIn() const;

	// return an error description
	static const char* ErrText(int err);
	enum { kErrUnknown=0x8200, kErrBadPassword,
		kErrLoggedIn, kErrLoggedOut, kErrConnected, kErrNotConnected,
		kErrNoStreambuf, kErrUserCancelled };

	// Handle messages relating to persistent data
	void BaseLogin		();
	void BaseUserDelta	(const CMsgGGSUserDelta& msg);
	void BaseDisconnect	();

	// Handle messages
	virtual void Handle				(const CMsg& msg);
	virtual void HandleAlias		(const CMsgGGSAlias& msg);
	virtual void HandleDisconnect	();
	virtual void HandleErr			(const CMsgGGSErr& msg);
	virtual void HandleFinger		(const CMsgGGSFinger& msg);
	virtual void HandleHelp			(const CMsgGGSHelp& msg);
	virtual void HandleLogin		();
	virtual void HandleTell			(const CMsgGGSTell& msg);
	virtual void HandleUnknown		(const CMsgGGSUnknown& msg);
	virtual void HandleUserDelta	(const CMsgGGSUserDelta& msg);
	virtual void HandleWho			(const CMsgGGSWho& msg);

	// persistent 
	std::map<std::string,int> loginToLevel;

	virtual void DirectMessage(std::string& sMsg);
	CSGBase* PService(const std::string& sUserLogin);
	bool HasService(const std::string& sUserLogin) const;

protected:
	virtual CSGBase* CreateService(const std::string& sUserLogin) = 0;
	virtual void DestroyService(const std::string& sUserLogin);
	virtual void DestroyAllServices();

	virtual int await(const char* sAwait);

	// turn stream data into messages
	virtual void ProcessLine(std::string& sLine);
	virtual void ProcessMessage();

	// parse messages
	virtual void Parse(const std::string& sMsg, const std::string& sFrom, std::istream& is);

	// post messages
	virtual void Post(std::string& sMsg);

	bool fLoggedIn;
	std::string sLogin;

	std::map<std::string, CSGBase*> loginToPService;

private:

	std::string sMsg;
	sockbuf *psockbuf;
};

#endif	//GDK_GGSSTREAM_H
