
# pest ![plague-doc](https://64k.by/assets/pest.png)

antibloat & turbo simplistic header only cxx unit testing & benchmarking thingy.

implemented this because a hackable solution for unit testing was needed not
causing excessive compile times once you have more than one assertion.

- reduced sloc
- easy to understand and hack
- does not suffer from extraordinarily compile time bloat
- does not suffer from extraordinarily compiled binary bloat
- macro free

`pest` includes a `xoshiro` prng and a `zipfian distribution` helper class:

- `#include <pest/xoshiro.hxx>`
- `#include <pest/zipfian-distribution.hxx`>

## requires

- c++17
- `__builtin_FILE`, `__builtin_LINE` and `__builtin_FUNCTION` ( gcc and clang )
    - for the custom `source_location` impl :/

## examples

### unit tests ( `using emptyspace::pest` )

source file ...

```cpp
#include <pest/pest.hxx>

namespace {

emptyspace::pest::suite basic( "pest self test", []( auto& test ) {
  using namespace emptyspace::pest;

  test( "true equal_to true", []( auto& expect ) {
    expect( true, equal_to( true ) );
  } );

  test( "vector<> equal_to initializer_list<>", []( auto& expect ) {
    auto const inputs = std::vector<int>{ 3, 4, 7 };
    expect( inputs, equal_to( { 3, 4, 7 } ) );
  } );
} );

} // namespace

int main() {
  basic( std::clog );
  return 0;
}
```

... and example output

```
# assuming the example from above is in `example.cxx`
pest $ clang++10 -I. -std=c++17 example.cxx -o ex
pest $ ./ex
[suite <pest self test> | true equal_to true]
[suite <pest self test> | vector<> equal_to initializer_list<>]
[suite <pest self test> | summary]
  total assertions failed = 0
  total assertions pass = 2
  total assertions skipped = 0
  total uncaught exceptions = 0
  total tests = 2
```

### benchmarks ( `using emptyspace::pnch` )

suppose we want to benchmark the `strftime` function ...

```cpp
#include <pest/pnch.hxx>
#include <ctime>

int main() {
  emptyspace::pnch::config cfg;
  std::size_t n = 0;
  char buffer[128];
  cfg.run(
         "strftime",
         [&]() {
           auto stamp1 = std::chrono::system_clock::now();
           auto epoch = std::chrono::system_clock::to_time_t( stamp1 );
           auto gmtime = std::gmtime( &epoch );
           n += std::strftime( buffer, 64, "%Y.%m.%d:%T.", gmtime );
         } )
      .touch( n )
      .report_to( std::cerr );
      
  return n > 0;
}
```

... could lead to the following output

```
[benchmark | strftime]
  stats/total = 2.27702e+09
  stats/average = 990.007
  stats/stddev = 7.18419
```

## development & packaging

[build2](https://build2.org) is used for life cycle management. for using without `build2`
just drop the header into your project.

cloning the repository and running the test driver `test/unit/driver.cxx`

```
$ git clone https://github.com/stackless-goto/pest
$ cd pest
$ bdep init -C @clang10 cc config.cxx=clang++10
$ bdep test
test tests/unit/testscript{testscript}@../pest-clang10/pest/tests/unit/ ../pest-clang10/pest/tests/unit/exe{driver}
```

declaring `pest` as dependency in a `build2` project:

```
TODO
```

## references

similar libraries. might be better suited ...

unit test helpers

- [~martinmoene/lest](https://github.com/martinmoene/lest) ( small-ish, compile-time and binary bloat )
- [~jimporter/mettle](https://github.com/jimporter/mettle) ( big-ish, compile-time bloat )

benchmark helpers

- [~martinus/nanobench](https://github.com/martinus/nanobench)
- [~cameron314](https://github.com/cameron314/microbench)

## mirrors

- darcs :: <https://hub.darcs.net/magenbluten/pest>
- git :: <https://github.com/stackless-goto/pest>

## license

choose between `UNLICENSE` or `LICENSE` freely. `zipfian` and `xoshiro` are derived work and
therefore -- as indicated in the corresponding source files -- `MIT` licensed.

## support and blame-game

magenbluten < mb [at] 64k.by > :: <https://64k.by>
