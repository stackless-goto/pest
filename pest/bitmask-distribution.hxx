// SPDX-License-Identifier: BlueOak-1.0.0

// @see
//   - <https://github.com/degski/uniform_int_distribution_fast/blob/master/uid_fast/uniform_int_distribution_fast.hpp>
//   - <http://www.pcg-random.org/posts/bounded-rands.html>

#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace emptyspace {

template <typename Int = int>
class bitmask_distribution;

namespace detail {

// `x` must be greater than `0`. otherwise UB.
template <typename Int>
auto leading_zeros( Int const x ) noexcept {
  if constexpr( std::is_same_v<Int, std::uint64_t> ) {
    return __builtin_clzll( x );
  } else {
    return __builtin_clz( static_cast<std::uint32_t>( x ) );
  }
}

} // namespace detail

template <typename Int>
class bitmask_distribution {
  using result_type = Int;
  using range_type = typename std::make_unsigned<result_type>::type;

  result_type _min;
  range_type _range;

 public:
  constexpr bitmask_distribution( result_type const min, result_type const max ) noexcept
    : _min{ min }, _range{ static_cast<range_type>( max - min ) } {
    assert( min < max );
  }

  template <typename Gen>
  [[nodiscard]] result_type operator()( Gen& gen ) const noexcept {
    if( _range == 0 ) { return static_cast<result_type>( gen() ); }
    // calling leading_zeros is safe because `_range != 0`
    range_type const mask = std::numeric_limits<range_type>::max() >> detail::leading_zeros( _range );
    range_type x;
    do { x = gen() & mask; } while( x > _range );
    return static_cast<result_type>( x ) + _min;
  }
};

} // namespace emptyspace
