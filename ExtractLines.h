// $Id$
// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.
//! @file   ExtractLines.h
//! Brief.
//! @author Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date   2010-04-06 11:59
//! @ingroup
//!

#if !defined(EXTRACTLINES_H__20100406T1159)
#define EXTRACTLINES_H__20100406T1159

#include "Fwd.h"

//!
//! @author  Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date    2010-04-06 11:59
//! @ingroup
//! Brief.
//!
int ExtractLines(int argc,
                 char** argv,
                 const char* submode,
                 const CComputerDefaults& cd1,
                 const CComputerDefaults& cd2,
                 int nGames);

#endif
