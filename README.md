# ton-contest-stuff

research and code on the 2025 TON C++ contest.

## What do i need to build the project?

- `CMake`
- C++20 compiler( tested with `g++ 14.2.0` )
- `GTest`
- `pthread`( _almost all linux distros have it installed by default_ )

## How can I build the project?

### Debug build

```bash
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build .
```

### Release build

```bash
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```

## So how do I run the tests?

```bash
# if already not in build directory
cd build

# Run tests
./test_pool
```
