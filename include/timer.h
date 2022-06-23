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
    void start();
    void stop();
  };

  
  /// return the static timer map. The first call to this method
  /// creates the the map, and starts the Overall timer and is not
  /// threadsafe. Using the returned map directly is not threadsafe.
  inline std::map<std::string, Times>& timers() {
    static std::map<std::string, Times> _timers;
    return _timers;
  }

  inline void Times::start()
  {
#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
    if (started) return;
    started = true;
    counts++;
          
    struct tms tbuf;
    start_e = times(&tbuf);
    start_u = tbuf.tms_utime;
    start_s = tbuf.tms_stime;
#endif
  }

  inline void Times::stop()
  {
#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
    if (!started) return;
    started = false;

    static const double seconds=1.0/sysconf(_SC_CLK_TCK);
    struct tms tbuf;
    elapsed += (times(&tbuf)-start_e)*seconds;
    user += (tbuf.tms_utime-start_u)*seconds;
    system += (tbuf.tms_stime-start_s)*seconds;
#endif
  }
 
  /// start the named timer. This call is threadsafe in an OpenMP
  /// parallel region.
  inline void start_timer(const std::string& s)
  {
#ifdef _OPENMP
#pragma omp critical(ecolab_timers)
#endif
    {
      timers()[s].start();
    }
  }

  /// stop the named timer. This call is threadsafe in an OpenMP
  /// parallel region.
  inline void stop_timer(const std::string& s)
  {
#ifdef _OPENMP
#pragma omp critical(ecolab_timers)
#endif
    timers()[s].stop();
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
