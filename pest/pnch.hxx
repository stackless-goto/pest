// SPDX-License-Identifier: UNLICENSE

#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <ostream>
#include <ratio>
#include <utility>
#include <vector>

#if defined( __FreeBSD__ ) || defined( __linux__ )
#  include <pthread.h>
#  include <pthread_np.h>
#  include <sys/cpuset.h>
#  include <sys/resource.h>
#  include <sys/time.h>
#  include <sys/types.h>
#else
#  error unkown platform: need posix like environment ( currently only freebsd and linux are supported )
#endif

// @see
//   - <github.com/cameron314/microbench>
//   - <github.com/martinus/nanobench>

namespace emptyspace::pnch {
namespace detail {
#if defined( __FreeBSD__ )
struct perfc {
  rusage _rusage_begin;
  rusage _rusage_end;

  explicit perfc() {
    std::memset( &_rusage_begin, 0, sizeof( _rusage_begin ) );
    std::memset( &_rusage_end, 0, sizeof( _rusage_end ) );
  }

  static inline auto now() noexcept {
    std::atomic_thread_fence( std::memory_order_seq_cst );
    auto const t = std::chrono::steady_clock::now();
    std::atomic_thread_fence( std::memory_order_seq_cst );
    return t;
  }

  void begin() noexcept {
    if( auto rc = getrusage( RUSAGE_SELF, &_rusage_begin ); rc != 0 ) {
      std::cerr << "getrusage() failed: error=" << strerror( errno ) << std::endl;
      std::memset( &_rusage_begin, 0, sizeof( _rusage_begin ) );
    }
  }

  void end() noexcept {
    if( auto rc = getrusage( RUSAGE_SELF, &_rusage_end ); rc != 0 ) {
      std::cerr << "getrusage() failed: error=" << strerror( errno ) << std::endl;
      std::memset( &_rusage_end, 0, sizeof( _rusage_begin ) );
    }
  }

  static inline void pin( int cpu = 0x1 ) noexcept {
    cpuset_t set = CPUSET_T_INITIALIZER( cpu );
    if( auto rc = pthread_setaffinity_np( pthread_self(), sizeof( cpuset_t ), &set ); rc != 0 ) {
      std::cerr << "pthread_setaffinity_np() failed: error=" << strerror( rc ) << std::endl;
    }
  }

  void report_to( std::ostream& os, std::string_view const name, rusage const& ru ) noexcept {
    os << "  " << name << "/max resident set size = " << ru.ru_maxrss << std::endl;
    os << "  " << name << "/minor page faults = " << ru.ru_minflt << std::endl;
    os << "  " << name << "/major page faults = " << ru.ru_majflt << std::endl;
  }

  void report_to( std::ostream& os ) noexcept {
    report_to( os, "begin", _rusage_begin );
    report_to( os, "end", _rusage_end );
  }
};
#elif defined( __linux__ )
struct perfc {
  rusage _rusage_begin;
  rusage _rusage_end;

  explicit perfc() {
    std::memset( &_rusage_begin, 0, sizeof( _rusage_begin ) );
    std::memset( &_rusage_end, 0, sizeof( _rusage_end ) );
  }

  static inline auto now() noexcept {
    std::atomic_thread_fence( std::memory_order_seq_cst );
    auto const t = std::chrono::steady_clock::now();
    std::atomic_thread_fence( std::memory_order_seq_cst );
    return t;
  }

  void begin() noexcept {
    if( auto rc = getrusage( RUSAGE_SELF, &_rusage_begin ); rc != 0 ) {
      std::cerr << "getrusage() failed: error = " << strerror( errno ) << std::endl;
      std::memset( &_rusage_begin, 0, sizeof( _rusage_begin ) );
    }
  }

  void end() noexcept {
    if( auto rc = getrusage( RUSAGE_SELF, &_rusage_end ); rc != 0 ) {
      std::cerr << "getrusage() failed: error = " << strerror( errno ) << std::endl;
      std::memset( &_rusage_end, 0, sizeof( _rusage_begin ) );
    }
  }

  static inline void pin( int cpu = 0x1 ) noexcept {
    cpuset_t set = CPUSET_T_INITIALIZER( cpu );
    if( auto rc = pthread_setaffinity_np( pthread_self(), sizeof( cpuset_t ), &set ); rc != 0 ) {
      std::cerr << "pthread_setaffinity_np() failed: error = " << strerror( rc ) << std::endl;
    }
  }

  void report_to( std::ostream& os, std::string_view const name, rusage const& ru ) noexcept {
    os << "  " << name << "/max resident set size = " << ru.ru_maxrss << std::endl;
    os << "  " << name << "/minor page faults = " << ru.ru_minflt << std::endl;
    os << "  " << name << "/major page faults = " << ru.ru_majflt << std::endl;
  }

  void report_to( std::ostream& os ) noexcept {
    report_to( os, "begin", _rusage_begin );
    report_to( os, "end", _rusage_end );
  }
};
#else
#  error unknown platform: do not know how to querying performance counters
#endif

// see folly's Benchmark.h
template <typename T>
constexpr bool doNotOptimizeNeedsIndirect() {
  using Decayed = typename std::decay<T>::type;
  return ! std::is_trivially_copyable_v<Decayed> || sizeof( Decayed ) > sizeof( long ) ||
      std::is_pointer<Decayed>::value;
}

template <typename T>
typename std::enable_if<! doNotOptimizeNeedsIndirect<T>()>::type doNotOptimizeAway( T const& val ) {
  asm volatile( "" ::"r"( val ) );
}

template <typename T>
typename std::enable_if<doNotOptimizeNeedsIndirect<T>()>::type doNotOptimizeAway( T const& val ) {
  asm volatile( "" ::"m"( val ) : "memory" );
}

struct stats_t {
 private:
  double _min;
  double _max;
  double _q[3];
  double _avg;
  double _variance;

 public:
  stats_t(
      std::vector<double> const& results,
      std::uint64_t const inner_loop_cnt,
      double const offset ) noexcept {
    std::vector<double> _results = results;
    std::sort( _results.begin(), _results.end() );
    auto count = _results.size();
    auto scale = static_cast<double>( inner_loop_cnt );

    for( decltype( count ) i = 0; i < count; ++i ) {
      _results[i] /= scale;
      _results[i] -= offset;
    }

    _min = _results[0];
    _max = _results[count - 1];

    if( count == 1 ) {
      _q[0] = _q[1] = _q[2] = _results[0];
      _avg = _results[0];
      _variance = 0;
      return;
    }

    // kahan summation to reduce error
    //   - see: http://en.wikipedia.org/wiki/Kahan_summation_algorithm
    double sum = 0;
    double c = 0; // error last time around
    for( std::size_t i = 0; i != count; ++i ) {
      auto y = _results[i] - c;
      auto t = sum + y;
      c = ( t - sum ) - y;
      sum = t;
    }
    _avg = sum / static_cast<double>( count );

    // calculate unbiased (corrected) sample variance
    sum = 0;
    c = 0;
    for( std::size_t i = 0; i != count; ++i ) {
      auto y = ( _results[i] - _avg ) * ( _results[i] - _avg ) - c;
      auto t = sum + y;
      c = ( t - sum ) - y;
      sum = t;
    }
    _variance = sum / static_cast<double>( count - 1 );

    // see Method 3 here: http://en.wikipedia.org/wiki/Quartile
    _q[1] = ( count & 1 ) == 0 ? ( _results[count / 2 - 1] + _results[count / 2] ) * 0.5
                               : _results[count / 2];
    if( ( count & 1 ) == 0 ) {
      _q[0] = ( count & 3 ) == 0 ? ( _results[count / 4 - 1] + _results[count / 4] ) * 0.5
                                 : _results[count / 4];
      _q[2] = ( count & 3 ) == 0
          ? ( _results[count / 2 + count / 4 - 1] + _results[count / 2 + count / 4] ) * 0.5
          : _results[count / 2 + count / 4];
    } else if( ( count & 3 ) == 1 ) {
      _q[0] = _results[count / 4 - 1] * 0.25 + _results[count / 4] * 0.75;
      _q[2] = _results[count / 4 * 3] * 0.75 + _results[count / 4 * 3 + 1] * 0.25;
    } else { // (count & 3) == 3
      _q[0] = _results[count / 4] * 0.75 + _results[count / 4 + 1] * 0.25;
      _q[2] = _results[count / 4 * 3 + 1] * 0.25 + _results[count / 4 * 3 + 2] * 0.75;
    }
  }

  inline double min() const noexcept { return _min; }

  inline double max() const noexcept { return _max; }

  inline double range() const noexcept { return _max - _min; }

  inline double avg() const noexcept { return _avg; }

  inline double variance() const noexcept { return _variance; }

  inline double stddev() const noexcept { return std::sqrt( _variance ); }

  inline double median() const noexcept { return _q[1]; }

  inline double q1() const noexcept { return _q[0]; }

  inline double q2() const noexcept { return _q[1]; }

  inline double q3() const noexcept { return _q[2]; }

  inline double q( std::size_t which ) const noexcept {
    assert( which < 4 && which > 0 );
    return _q[which - 1];
  }
};
} // namespace detail

template <typename... Args>
void doNotOptimizeAway( Args&&... args ) {
  (void)std::initializer_list<int>{
      ( detail::doNotOptimizeAway( std::forward<Args>( args ) ), 0 )... };
}

struct config {
  detail::perfc _perfc;
  std::uint64_t _inner_loop_cnt{ 100'000 };
  std::uint32_t _outer_loop_cnt{ 23 };
  std::string _name;
  std::unique_ptr<detail::stats_t> _cached_stats;

  double _offset{ .0 };
  std::vector<double> _results;

  void reset() noexcept {
    _cached_stats.reset( nullptr );
    _offset = .0;
  }

  detail::stats_t& stats() noexcept {
    if( ! _cached_stats ) {
      _cached_stats = std::make_unique<detail::stats_t>( _results, _inner_loop_cnt, _offset );
    }
    return *_cached_stats;
  }

  double average() noexcept { return stats().avg(); }

  template <typename... Args>
  config& touch( Args&&... args ) noexcept {
    (void)std::initializer_list<int>{
        ( detail::doNotOptimizeAway( std::forward<Args>( args ) ), 0 )... };
    return *this;
  }

  config& i( std::uint64_t const inner_loop_cnt ) noexcept {
    reset();
    _results.clear();
    _inner_loop_cnt = inner_loop_cnt;
    return *this;
  }

  config& o( std::uint32_t const outer_loop_cnt ) noexcept {
    reset();
    _results.clear();
    _outer_loop_cnt = outer_loop_cnt;
    return *this;
  }

  template <typename TFunc>
  config& run( std::string_view const name, TFunc&& func ) noexcept {
    reset();
    _results.clear();
    _name = name;
    _perfc.begin();
    for( std::uint32_t i = 0; i < _outer_loop_cnt; ++i ) {
      auto start = detail::perfc::now();
      for( std::uint64_t j = 0; j < _inner_loop_cnt; ++j ) {
        func();
      }
      auto end = detail::perfc::now();
      auto delta = std::chrono::duration<double, std::nano>( end - start ).count();
      _results.push_back( delta );
    }
    _perfc.end();
    return *this;
  }

  config& offset( double const offset ) noexcept {
    reset();
    _offset = offset;
    return *this;
  }

  config& report_to( std::ostream& os, std::string_view const pre = "" ) noexcept {
    auto const sep = pre == "" ? "  stats" : "  stats/";
    auto const s = stats();
    double total = .0;
    for( auto x : _results )
      total += x;
    os << "[benchmark | " << _name << "]" << std::endl;
    os << sep << pre << "/total = " << total << std::endl;
    os << sep << pre << "/average = " << s.avg() << std::endl;
    os << sep << pre << "/stddev = " << s.stddev() << std::endl;
    if( _offset != .0 ) { os << sep << pre << "/offset = " << _offset << std::endl; }
    os << std::endl;
    return *this;
  }
};

class oneshot {
  std::string _name;
  double _delta_t;
  detail::perfc _perfc{};

 public:
  template <typename F>
  auto run( std::string_view const name, F const f ) noexcept //->
  //typename std::enable_if<noexcept( f() ), oneshot&>::type
  {
    _name = name;
    _perfc.begin();
    auto begin = detail::perfc::now();
    f();
    auto end = detail::perfc::now();
    _delta_t = std::chrono::duration<double, std::nano>( end - begin ).count();
    _perfc.end();
    return *this;
  }

  oneshot& pin( int const cpu = 0x1 ) noexcept {
    detail::perfc::pin( cpu );
    return *this;
  }

  template <typename... Args>
  auto& touch( Args&&... args ) noexcept {
    (void)std::initializer_list<int>{
        ( detail::doNotOptimizeAway( std::forward<Args>( args ) ), 0 )... };
    return *this;
  }

  auto& report_to( std::ostream& os ) noexcept {
    os << "[oneshot | " << _name << "]" << std::endl;
    if( _delta_t > 1'000'000'000.0 )
      os << "  delta_t = " << ( _delta_t / 1'000'000'000.0 ) << "s" << std::endl;
    else if( _delta_t > 1'000'000.0 )
      os << "  delta_t = " << ( _delta_t / 1'000'000.0 ) << "ms" << std::endl;
    else if( _delta_t > 1'000.0 )
      os << "  delta_t = " << ( _delta_t / 1'000.0 ) << "us" << std::endl;
    else
      os << "  delta_t = " << _delta_t << "ns" << std::endl;
    _perfc.report_to( os );
    return *this;
  }
};

} // namespace emptyspace::pnch
