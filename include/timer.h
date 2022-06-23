/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef TIMER_H
#define TIMER_H
#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
#include <unistd.h>
#include <sys/times.h>
#endif
#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

namespace ecolab
{
  struct Times
  {
    unsigned long counts;
    double elapsed, user, system;
    clock_t start_e, start_u, start_s;
    bool started;
    Times(): counts(0), elapsed(0), user(0), system(0), started(false) {}
  };

  /// return the static timer map. The first call to this method
  /// creates the the map, and is not threadsafe. Using the returned
  /// map directly is not threadsafe.
  inline std::map<std::string, Times>& timers() {
    static std::map<std::string, Times> _timers;
    return _timers;
  }

  /// start the named timer. This call is threadsafe in an OpenMP
  /// parallel region.
  inline void start_timer(const std::string& s)
  {
#ifdef _OPENMP
#pragma omp critical(ecolab_timers)
#endif
    {
#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
      Times& t=timers()[s];
      if (!t.started)
        {
          t.started = true;
          t.counts++;
          
          struct tms tbuf;
          t.start_e = times(&tbuf);
          t.start_u = tbuf.tms_utime;
          t.start_s = tbuf.tms_stime;
        }
#endif
    }
  }

  /// stop the named timer. This call is threadsafe in an OpenMP
  /// parallel region.
  inline void stop_timer(const std::string& s)
  {
#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
      Times& t=timers()[s];
      if (t.started)
        {
          t.started = false;

          static const double seconds=1.0/sysconf(_SC_CLK_TCK);
          std::cout<<seconds<<std::endl;
          struct tms tbuf;
          t.elapsed += (times(&tbuf)-t.start_e)*seconds;
          t.user += (tbuf.tms_utime-t.start_u)*seconds;
          t.system += (tbuf.tms_stime-t.start_s)*seconds;
        }
#endif
  }

 
  /// RAII class for timing code blocks.
  class Timer
  {
    std::string name;
  public:
    Timer(const std::string& name): name(name) {start_timer(name);}
    ~Timer() {stop_timer(name);}
  };

  struct SortElapsed
  {
    bool operator()(const std::pair<std::string, Times>& x,
                    const std::pair<std::string, Times>& y)
    {
      return x.second.elapsed < y.second.elapsed;
    }
  };

  // output the timer values, sorted in elapsed time order
  inline void print_timers()
  {
    std::vector<std::pair<std::string, Times> > times
      (timers().begin(), timers().end());
    std::sort(times.begin(), times.end(), SortElapsed());
    // use printf rather than cout, as we may wish to call this from a
    // static object destructor
    std::cout << "------------------ Times --------------------------------\n";
    for (size_t i=0; i<times.size(); ++i)
      std::cout << "Elapsed: "<<times[i].second.elapsed<<
        " User: "<<times[i].second.user<<" System: "<<times[i].second.system <<
        " Counts: "<<times[i].second.counts<<" "<<times[i].first<<std::endl;
    std::cout << "----------------------------------------------------------"<<
      std::endl;
  }
}
#endif
