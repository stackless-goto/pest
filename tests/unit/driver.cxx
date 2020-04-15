#include <exception>
#include <pest/pest.hxx>

#include <map>
#include <stdexcept>
#include <vector>

namespace {

// example from fluent c++ blog, iirc ( or was it modernes c++? )
std::vector<int> times7( std::vector<int> const& numbers ) noexcept {
  auto v = std::vector<int>{};
  std::transform( begin( numbers ), end( numbers ), std::back_inserter( v ), []( int const n ) {
    return n * 7;
  } );
  return v;
}

emptyspace::pest::suite basic( "pest test suite", []( auto& test ) {
  using namespace emptyspace::pest;

  test( "std::map<>: insert and find key", []( auto& expect ) {
    std::map<std::uint32_t, std::uint32_t> m;
    auto [x, n] = m.insert( { 23, 42 } );
    auto i = m.find( 23 );
    expect( n, equal_to( true ) );
    expect( i->first, equal_to( 23u ) );
    expect( i->second, equal_to( 42u ) );
  } );

  test( "std::vector<>: times7 failing", []( auto& expect ) {
    auto const inputs = std::vector<int>{ 3, 4, 7 };
    // can not include the failing test because output depends
    // on the build path ( using __FILE__ or __builtin_FILE etc )
    // this means we are facing a non reproducible build :/
    // expect( times7( inputs ), equal_to( { 3, 4, 7 } ) );

    // execution would continue; but the `equal_to` comparision
    // would not be executed
    expect( times7( inputs ), not_equal_to( { 3, 4, 7 } ) );
  } );

  test( "std::vector<>: times7 succeeding", []( auto& expect ) {
    auto const inputs = std::vector<int>{ 3, 4, 7 };
    expect( times7( inputs ), equal_to( { 21, 28, 49 } ) );
  } );

  test( "throws out-of-range", []( auto& expect ) {
    expect( throws<std::out_of_range>( [&]() { throw std::out_of_range(""); } ) );
  } );
} );

} // namespace

int main() {
  basic( std::clog );
  return 0;
}
