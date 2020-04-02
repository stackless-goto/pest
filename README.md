# pest

antibloat & turbo simplistic single header c++ unit testing thingy. 

implemented this because a hackable solution for unit testing was needed not
causing excessive compile times once you have more than one assertion.

- reduced sloc
- easy to understand and hack
- does not suffer from extraordinarily compile time bloat
- does not suffer from extraordinarily compiled binary bloat
- theoretically, the output can be read back using <inihxx>
- macro free

## requires

- c++17
- `__builtin_FILE`, `__builtin_LINE` and `__builtin_FUNCTION` ( gcc and clang )
    - for the custom `source_location` impl :/

## examples

source file ...

```cpp
#include <pest/pest.hxx>

namespace {

suite basic( "pest self test", []( auto& test ) {
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
```

## development & packaging

[build2](https://build2.org) is used for life cycle management.

declaring `pest` as dependency in a `build2` project:

```
```

## references

similar libraries. might be better suited ...

- lest ( small-ish, compile-time and binary bloat )
    - <https://github.com/martinmoene/lest>
- mettle ( big-ish, compile-time bloat )
    - <https://github.com/jimporter/mettle>

## mirrors

- main @ <https://hub.darcs.net/magenbluten/pest>
- m1 @ <https://github.com/stackless-goto/pest>
- m2 @ <https://sr.ht/~magenbluten/pest>

## licencse

choose between `UNLICENSE` or `LICENSE` freely.

## support and blame-game

magenbluten < mb [at] 64k.by > | <https://64k.by>
