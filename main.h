// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// othello program

#pragma once

#include "Player.h"
#include "Timing.h"

void ParseCommandLine(int argc,
                      char* argv[],
                      const char*& submode,
                      CComputerDefaults& cd,
                      int& nGames,
                      int& nSkipValue);
void ReadParameters(CComputerDefaults& cd1, CComputerDefaults& cd2);
