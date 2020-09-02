// SPDX-License-Identifier: BlueOak-1.0.0

#include <pest/bitmask-distribution.hxx>
#include <pest/pest.hxx>
#include <pest/pnch.hxx>
#include <pest/xoshiro.hxx>

#include <exception>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

// example from fluent c++ blog, iirc ( or was it modernes c++? )
std::vector<int> times7( std::vector<int> const& numbers ) noexcept {
  auto v = std::vector<int>{};
  std::transform( begin( numbers ), end( numbers ), std::back_inserter( v ),
                  []( int const n ) { return n * 7; } );
  return v;
}

emptyspace::pest::suite basic( "pest test suite", []( auto& test ) {
  using namespace emptyspace::pest;
  using namespace emptyspace::xoshiro;
  using namespace emptyspace::pnch;

  test( "std::map<>: insert and find key", []( auto& expect ) {
    std::map<std::uint32_t, std::uint32_t> m;
    auto [x, n] = m.insert( { 23, 42 } );
    auto i = m.find( 23 );
    expect( n, equal_to( true ) );
    expect( i->first, equal_to( 23u ) );
    expect( i->second, equal_to( 42u ) );
  } );

  test( "snytax sugar for expect( true, equal_to( true ) )", []( auto& expect ) {
    expect( true, equal_to( true ) );
    expect( true );
  } );

  test( "std::vector<>: times7 failing", []( auto& expect ) {
    auto const inputs = std::vector<int>{ 3, 4, 7 };
    // fails
    expect( times7( inputs ), equal_to( { 3, 4, 7 } ) );
    expect( times7( inputs ), not_equal_to( { 3, 4, 7 } ) );
  } );

  test( "std::vector<>: times7 succeeding", []( auto& expect ) {
    auto const inputs = std::vector<int>{ 3, 4, 7 };
    expect( times7( inputs ), equal_to( { 21, 28, 49 } ) );
  } );

  test( "throws out-of-range", []( auto& expect ) {
    expect( throws<std::out_of_range>( [&]() { throw std::out_of_range( "" ); } ) );
  } );

  test( "xoshiro with boring seed", []( auto& expect ) {
    xoshiro128starstar32 xo{ 0x2342 };
    expect( xo(), equal_to( 1566649558u ) );
  } );

  test( "benchmark nothing", []( auto& expect ) {
    emptyspace::pnch::config cfg;
    int x = 0;
    std::ostringstream os;
    cfg.i( 2 ).o( 3 ).run( "nothings", [&]() { x += 1; } ).touch( x ).report_to( os );
    expect( x, equal_to( 2 * 3 ) );
  } );

  test( "benchmark oneshot nothing", []( auto& expect ) {
    int x = 0;
    emptyspace::pnch::oneshot cfg;
    std::ostringstream os;
    cfg.run( "nothings but only once", [&]() { x += 1; } ).report_to( os );
    expect( x, equal_to( 1 ) );
  } );

  test( "hexify(std::array<std::byte,N>)", []( auto& expect ) {
    std::array<std::byte, 2> bytes{ std::byte( 0x23 ), std::byte( 0x42 ) };
    expect( hexify( bytes ), equal_to( "2342" ) );
  } );

  test( "hexify(std::byte[])", []( auto& expect ) {
    std::byte bytes[2]{ std::byte( 0x23 ), std::byte( 0x42 ) };
    expect( hexify( bytes ), equal_to( "2342" ) );
  } );

  test( "bitmask_distribution<int>{ 0, 23 }", []( auto& expect ) {
    constexpr auto SHOTS = 42;
    constexpr auto LO = 0;
    constexpr auto UP = 23;
    xoshiro128starstar32 rng{ 0x2323 };
    emptyspace::bitmask_distribution<int> gen{ LO, UP };
    for( auto i = 0; i < SHOTS; ++i ) {
      auto const x = gen( rng );
      expect( x >= LO );
      expect( x <= UP );
    }
  } );

  test( "bitmask_distribution<int>{ -23, 23 }", []( auto& expect ) {
    constexpr auto SHOTS = 42;
    constexpr auto LO = -23;
    constexpr auto UP = 23;
    xoshiro128starstar32 rng{ 0x2323 };
    emptyspace::bitmask_distribution<int> gen{ LO, UP };
    for( auto i = 0; i < SHOTS; ++i ) {
      auto const x = gen( rng );
      expect( x >= LO );
      expect( x <= UP );
    }
  } );

  test( "bitmask_distribution<int>{ -42, -23 }", []( auto& expect ) {
    constexpr auto SHOTS = 42;
    constexpr auto LO = -42;
    constexpr auto UP = -23;
    xoshiro128starstar32 rng{ 0x2323 };
    emptyspace::bitmask_distribution<int> gen{ LO, UP };
    for( auto i = 0; i < SHOTS; ++i ) {
      auto const x = gen( rng );
      expect( x >= LO );
      expect( x <= UP );
    }
  } );
} );

} // namespace

int main() {
  basic( std::clog );
  return 0;
}
