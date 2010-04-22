
#include "PreCompile.h"
#include "Log.h"
#include <boost/date_time/posix_time/posix_time.hpp>

Log::Log(std::ostream& stream)
   : stream(stream)
{
   boost::posix_time::ptime now
      = boost::posix_time::second_clock::local_time();
   stream << now << " - ";
}

std::ostream& Log::GetStream() const
{
   return stream;
}
