// $Id$
// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.
//! @file   Server.h
//! Brief.
//! @author Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date   2010-04-06 11:26
//! @ingroup
//!

#if !defined(SERVER_H__20100406T1126)
#define SERVER_H__20100406T1126

#include "Fwd.h"

//!
//! @author  Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date    2010-04-06 11:26
//! @ingroup
//! Brief.
//!
int Server(int argc,
           char** argv,
           const CComputerDefaults& cd1,
           int nGames);

#endif
