# pest

antibloat & turbo simplistic single header c++ unit testing thingy. 

i implemented this because i wanted a hackable solution not causing
excessive compile times once you have more than one assertion.

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

```cpp
```

## references

- lest ( small-ish, compile-time and binary bloat ) <>
- mettle ( big-ish, compile-time bloat ) <>

## support and blame-game

al3x < mb [at] 64k.by >
