#ifndef timer_h
#define timer_h

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include <windows.h>

// Author: Daniel Lidström
// Date: 2001-07-08
// Implements a timer class.
// Use like this Timer<float> timer;
// or Timer<double> timer;
// Revised: 2002-06-11 [added clock()]
// Revised: 2003-08-04 [slightly rewritten]
// Revised: 2003-10-05 [removed format(), added HHMMSS class]

template<class T>
class Timer {
public:
   Timer();
   // formats time into hh:mm:ss.xxx
   //static std::string format(T t);
   // start timer
   void start();

   // returns seconds since start
   T elapsed();

   // returns seconds since last elapsed
   T last() const;
private:
   LARGE_INTEGER _start;
   LARGE_INTEGER _freq;
   T _elapsed;
};

template<class T>
Timer<T>::Timer() {
   QueryPerformanceFrequency(&_freq);
   start();
}

template<class T>
void Timer<T>::start() {
   QueryPerformanceCounter(&_start);
}

template<class T>
T Timer<T>::elapsed() {
   LARGE_INTEGER temp;
   QueryPerformanceCounter(&temp);
   _elapsed = (T)(temp.QuadPart - _start.QuadPart) / _freq.QuadPart;
   return _elapsed;
}

template<class T>
T Timer<T>::last() const {
   return _elapsed;
}

// formats time into hh:mm:ss.xxx
class HHMMSSmmm {
public:
   HHMMSSmmm() { }
   HHMMSSmmm( double time ) : t(time) { }
   std::string print() const {
      std::stringstream stream;
      stream.fill('0');
      int hours = int(t/3600);
      int min = int(t/60 - 60*hours)%60;
      int sec = int(t-3600*hours-60*min);
      int milli = int(1000*(t-3600*hours-60*min-sec));
      stream << hours << ':' << std::setw(2) << min << ':' << std::setw(2) << sec
             << '.' << std::setw(3) << milli;
      return stream.str();
   }
protected:
   double t;
};

inline std::ostream& operator << ( std::ostream& os, const HHMMSSmmm& v ) { os << v.print(); return os; }

#endif
