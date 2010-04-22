// $Id$
//! @file   Log.h
//! Brief.
//! @author Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date   2010-04-06 11:27
//! @ingroup
//!

#if !defined(LOG_H__20100406T1127)
#define LOG_H__20100406T1127

#include <ostream>

//!
//! @author  Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date    2010-04-06 11:27
//! @ingroup
//! Brief.
//!
class Log
{
   std::ostream& stream;

public:

   Log(std::ostream& stream);

   std::ostream& GetStream() const;

   template<class T>
   friend std::ostream& operator<<(Log& log, const T& s);
};

template<class T>
std::ostream& operator<<(Log& log, const T& s)
{
   return log.stream << s;
}

#endif
