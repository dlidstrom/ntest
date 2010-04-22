// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

//////////////////////////////////////////////////////
// portable time measuring functions
// code added by Richard Delorme
//////////////////////////////////////////////////////

#include "PreCompile.h"
#include "Ticks.h"

#if defined(_WIN32)
#include <windows.h>

i8 GetTicks(void) {
   LARGE_INTEGER ticks;
   QueryPerformanceCounter(&ticks);
   return (i8)(ticks.QuadPart);
}

i8 GetTicksPerSecond(void) {
   LARGE_INTEGER ticks;
   QueryPerformanceFrequency(&ticks);
   return (i8)(ticks.QuadPart);
}

#elif defined(__unix__)

#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>

#if 0 //USE CPU elapsed time

i8 GetTicks(void) {
   struct rusage u;
   getrusage(RUSAGE_SELF, &u);
   return (i8) 1000000LL * u.ru_utime.tv_sec + u.ru_utime.tv_usec;
}

i8 GetTicksPerSecond(void) {
   return 1000000LL;
}

#else //use real time (ie clock on the wall)

i8 GetTicks(void) {
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (i8) 1000000 * tv.tv_sec + tv.tv_usec;
}

i8 GetTicksPerSecond(void) {
   return (i8) 1000000;
}

#endif // real time

#else // ANSI

i8 GetTicks(void) {
   return (i8) clock();
}

i8 GetTicksPerSecond(void) {
   return (i8) CLOCKS_PER_SEC;
}

#endif // ANSI
