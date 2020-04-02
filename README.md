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

```cpp
```

## references

- lest ( small-ish, compile-time and binary bloat )
    - <https://github.com/martinmoene/lest>
- mettle ( big-ish, compile-time bloat )
    - <https://github.com/jimporter/mettle>

## support and blame-game

magenbluten < mb [at] 64k.by >
