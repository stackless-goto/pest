// SPDX-License-Identifier: UNLICENSE

#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if __has_include( <source_location> )
#  include <source_location>
#else
namespace std {
struct source_location {
  char const* const _file;
  char const* const _func;
  int const _line;

  constexpr source_location( char const* file, char const* func, int const line )
    : _file{ file }, _func{ func }, _line{ line } {}

#  if defined( __has_builtin )
#    if __has_builtin( __builtin_FILE ) && __has_builtin( __builtin_FUNCTION ) &&                     \
        __has_builtin( __builtin_LINE )
  static inline source_location current(
      char const* file = __builtin_FILE(),
      char const* func = __builtin_FUNCTION(),
      int const line = __builtin_LINE() ) {
    return { file, func, line };
  }
#    else
  static inline source_location current(
      char const* file = "unsupported", char const* func = "unsupported", int const line = 0 ) {
    return { file, func, line };
  }
#    endif
#  else
  static inline source_location current(
      char const* file = "unsupported", char const* func = "unsupported", int const line = 0 ) {
    return { file, func, line };
  }
#  endif
};
} // namespace std
#endif

namespace emptyspace::pest {

inline std::ostream& operator<<( std::ostream& os, std::source_location const where ) noexcept {
  auto const s = std::string_view{ where._file };
  auto const n = s.rfind( '/' );

  os << ( n == std::string_view::npos ? s : std::string_view{ where._file + n + 1 } ) << ":"
     << where._line;
  return os;
}

template <typename T>
struct equal_to {
  T _expr;
  constexpr explicit equal_to( T&& v ) noexcept : _expr( std::forward<T>( v ) ) {}
};

template <typename T>
equal_to( T && ) -> equal_to<T>;

template <typename T>
equal_to( std::initializer_list<T> ) -> equal_to<std::initializer_list<T>>;

template <typename T>
struct not_equal_to {
  T _expr;
  constexpr explicit not_equal_to( T&& v ) noexcept : _expr( std::forward<T>( v ) ) {}
};

template <typename T>
not_equal_to( T && ) -> not_equal_to<T>;

template <typename T>
not_equal_to( std::initializer_list<T> ) -> not_equal_to<std::initializer_list<T>>;

template <typename T, typename U>
bool operator!=( T const& lhs, not_equal_to<U> const& rhs ) {
  return lhs != rhs._expr;
}

inline bool operator==( double const lhs, equal_to<double> const& rhs ) noexcept {
  return std::abs( rhs._expr - lhs ) <=
      std::abs( std::min( rhs._expr, lhs ) ) * std::numeric_limits<double>::epsilon();
}

template <typename T, typename U>
bool operator==( T const& lhs, equal_to<U> const& rhs ) {
  return lhs == rhs._expr;
}

template <typename T, typename U>
bool operator==( T const& lhs, equal_to<std::initializer_list<U>> const& rhs ) noexcept {
  return std::equal(
      std::begin( lhs ),
      std::end( lhs ),
      std::begin( rhs._expr ),
      std::end( rhs._expr ) );
}

template <typename T, typename U>
bool operator!=( T const& lhs, not_equal_to<std::initializer_list<U>> const& rhs ) noexcept {
  return not std::equal(
      std::begin( lhs ),
      std::end( lhs ),
      std::begin( rhs._expr ),
      std::end( rhs._expr ) );
}

inline bool operator!=( double const lhs, not_equal_to<double> const& rhs ) noexcept {
  return not (
      std::abs( rhs._expr - lhs ) <=
      std::abs( std::min( rhs._expr, lhs ) ) * std::numeric_limits<double>::epsilon() );
}

template <typename T, std::enable_if_t<! std::is_enum<T>::value, int> = 0>
std::ostream& print_to( std::ostream& os, T const& t ) {
  os << t;
  return os;
}

template <typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
std::ostream& print_to( std::ostream& os, T const t ) {
  auto const x = static_cast<typename std::underlying_type<T>::type>( t );
  os << "enum( " << x << " )";
  return os;
}

enum class result_throws { EXPECTED, UNEXPECTED, NOTHROW };

template <typename E, typename F>
result_throws throws( F const f ) {
  try {
    f();
    return result_throws::NOTHROW;
  } catch( E const& ) { //
    return result_throws::EXPECTED;
  } catch( ... ) { //
    return result_throws::UNEXPECTED;
  }
}

struct test_state {
  std::ostream& os;
  unsigned _failed{ 0 };
  unsigned _pass{ 0 };
  unsigned _uncaught_exns{ 0 };
  unsigned _tests{ 0 };
  unsigned _skipped{ 0 };

  void expect(
      result_throws const rc,
      std::source_location const where = std::source_location::current() ) noexcept {
    if( _failed > 0 ) {
      _skipped++;
      return;
    }
    switch( rc ) {
      case result_throws::NOTHROW:
        os << "  failed = " << where << std::endl;
        os << "  expected = throws" << std::endl;
        os << "  actual = did not throw" << std::endl;
        _failed++;
        break;
      case result_throws::UNEXPECTED:
        os << "  failed = " << where << std::endl;
        os << "  expected = throws known exception" << std::endl;
        os << "  actual = threw unexpected exception" << std::endl;
        _failed++;
        break;
      case result_throws::EXPECTED: _pass++; break;
    }
  }
  template <typename T, typename U>
  void expect(
      T const& lhs,
      equal_to<std::initializer_list<U>> const& rhs,
      std::source_location const where = std::source_location::current() ) noexcept {
    // we stop after the first failure, but still count
    // the number of assertions
    if( _failed > 0 ) {
      _skipped++;
      return;
    }
    try {
      if( lhs == rhs ) {
        _pass++;
      } else {
        os << "  failed = " << where << std::endl;
        os << "  assertion = equal_to" << std::endl;
        os << "  expected = { ";
        // TODO: we should apply `print_to` to each value
        std::ostream_iterator<U> it_r( os, ", " );
        std::copy( std::begin( rhs._expr ), std::end( rhs._expr ), it_r );
        os << "}" << std::endl;
        os << "  actual = { ";
        std::ostream_iterator<typename T::value_type> it_l( os, ", " );
        std::copy( std::begin( lhs ), std::end( lhs ), it_l );
        os << "}" << std::endl;
        _failed++;
      }
    } catch( ... ) {
      os << "  failed = threw exception" << std::endl;
      _failed++;
      _uncaught_exns++;
    }
  }

  template <typename T, typename U>
  void expect(
      T const& lhs,
      equal_to<U> const& rhs,
      std::source_location const where = std::source_location::current() ) noexcept {
    // we stop after the first failure, but still count
    // the number of assertions
    if( _failed > 0 ) {
      _skipped++;
      return;
    }
    try {
      if( lhs == rhs ) {
        _pass++;
      } else {
        os << "  failed = " << where << std::endl;
        os << "  assertion = equal_to" << std::endl;
        os << "  expected = ";
        print_to( os, rhs._expr ) << std::endl;
        os << "  actual = ";
        print_to( os, lhs ) << std::endl;
        _failed++;
      }
    } catch( ... ) {
      os << "  failed = threw exception" << std::endl;
      _failed++;
      _uncaught_exns++;
    }
  }

  template <typename T, typename U>
  void expect(
      T const& lhs,
      not_equal_to<std::initializer_list<U>> const& rhs,
      std::source_location const where = std::source_location::current() ) noexcept {
    // we stop after the first failure, but still count
    // the number of assertions
    if( _failed > 0 ) {
      _skipped++;
      return;
    }
    try {
      if( lhs != rhs ) {
        _pass++;
      } else {
        os << "  failed = " << where << std::endl;
        os << "  assertion = not_equal_to" << std::endl;
        os << "  expected = { ";
        // TODO: we should apply `print_to` to each value
        std::ostream_iterator<U> it_r( os, ", " );
        std::copy( std::begin( rhs._expr ), std::end( rhs._expr ), it_r );
        os << "}" << std::endl;
        os << "  actual = { ";
        std::ostream_iterator<typename T::value_type> it_l( os, ", " );
        std::copy( std::begin( lhs ), std::end( lhs ), it_l );
        os << "}" << std::endl;
        _failed++;
      }
    } catch( ... ) {
      os << "  failed = threw exception" << std::endl;
      _failed++;
      _uncaught_exns++;
    }
  }

  template <typename T, typename U>
  void expect(
      T const& lhs,
      not_equal_to<U> const& rhs,
      std::source_location const where = std::source_location::current() ) noexcept {
    // we stop after the first failure, but still count
    // the number of assertions
    if( _failed > 0 ) {
      _skipped++;
      return;
    }
    try {
      if( lhs != rhs ) {
        _pass++;
      } else {
        os << "  failed = " << where << std::endl;
        os << "  assertion = not_equal_to" << std::endl;
        os << "  expected = ";
        print_to( os, rhs._expr ) << std::endl;
        os << "  actual = ";
        print_to( os, lhs ) << std::endl;
        _failed++;
      }
    } catch( ... ) {
      os << "  failed = threw exception" << std::endl;
      _failed++;
      _uncaught_exns++;
    }
  }

  template <typename T, typename U>
  inline void operator()(
      T const& lhs,
      U const& rhs,
      std::source_location const where = std::source_location::current() ) noexcept {
    expect( lhs, rhs, where );
  }

  inline void operator()(
      result_throws const rc,
      std::source_location const where = std::source_location::current() ) noexcept {
    expect( rc, where );
  }
};

struct suite_state {
  std::string _suite;
  std::ostream& os;
  unsigned _failed{ 0 };
  unsigned _pass{ 0 };
  unsigned _uncaught_exns{ 0 };
  unsigned _tests{ 0 };
  unsigned _skipped{ 0 };

  suite_state( std::string const& suite, std::ostream& out ) : _suite{ suite }, os{ out } {}

  template <typename Closure>
  void test( std::string_view const desc, Closure clos ) noexcept {
    os << "[suite <" << _suite << "> | " << desc << "]" << std::endl;
    _tests++;
    test_state test{ os };
    try {
      clos( test );
    } catch( std::exception const& e ) {
      os << "  uncaught exception: what = " << e.what() << std::endl;
      _uncaught_exns++;
    } catch( ... ) {
      os << "  uncaught exception =" << std::endl;
      _uncaught_exns++;
    }
    _failed += test._failed;
    _pass += test._pass;
    _uncaught_exns += test._uncaught_exns;
    _skipped += test._skipped;
  }

  template <typename Closure>
  inline void operator()( std::string_view const desc, Closure clos ) noexcept {
    test( desc, clos );
  }
};

struct suite {
  std::string _name;
  std::function<void( suite_state& )> _behaviour;

  template <typename T>
  suite( std::string_view const name, T&& t ) : _name{ name }, _behaviour{ std::move( t ) } {}

  void operator()( std::ostream& os ) noexcept {
    suite_state st{ _name, os };
    try {
      _behaviour( st );
      os << "[suite <" << _name << "> | summary]" << std::endl;
      os << "  total assertions failed = " << st._failed << std::endl;
      os << "  total assertions pass = " << st._pass << std::endl;
      os << "  total assertions skipped = " << st._skipped << std::endl;
      os << "  total uncaught exceptions = " << st._uncaught_exns << std::endl;
      os << "  total tests = " << st._tests << std::endl;
    } catch( ... ) { os << "*** suite uncaught exception ***" << std::endl; }
  }
};

} // namespace emptyspace::pest
