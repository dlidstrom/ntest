
#define NOMINMAX
// Default to Windows XP
#define _WIN32_WINNT 0x0501
#define BOOST_LIB_DIAGNOSTIC
#define _SCL_SECURE_NO_WARNINGS

#include "GDK/types.h"
#include <boost/unordered_set.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <limits>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <ctype.h>
#include <stdio.h>

#define foreach BOOST_FOREACH
