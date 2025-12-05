# mo_yanxi_utility

A modern C++ utility library leveraging the latest C++20/23 features, including Modules and Concepts.

## Features

This library provides a collection of utilities organized into modules:

- **algo**: Algorithms.
- **concurrent**: Concurrency utilities (e.g., `atomic_shared_mutex`, `condition_variable_single`, `shared_queue`, `spsc_queue`).
- **container**: High-performance containers (e.g., `fixed_open_hash_map`, `circular_queue`, `array_stack`, `bitmap`).
- **dim2**: 2D geometry/math utilities.
- **math**: Mathematical functions, including constrained systems and quad trees.
- **string**: String manipulation utilities.
- **pointer**: Pointer management.
- **general**: General purpose utilities.
- **generic**: Generic programming helpers.

## Requirements

- **Compiler**: A C++ compiler with C++23 support (e.g., MSVC, Clang, GCC).
- **Build System**: [xmake](https://xmake.io/)

## Build & Test

The project uses `xmake` for building and testing.

### Build

```bash
xmake
```

### Run Tests

```bash
xmake run mo_yanxi.utility.test
```

## Usage Example

Here is a simple example demonstrating the usage of `fixed_open_hash_map`:

```cpp
import std;
import mo_yanxi.open_addr_hash_map;

int main() {
    // Define a map with unsigned key/value and -1 as the empty key sentinel
    constexpr unsigned EMPTY_KEY = static_cast<unsigned>(-1);
    mo_yanxi::fixed_open_hash_map<unsigned, unsigned, EMPTY_KEY> map;

    // Insert values
    map[1] = 100;
    map[2] = 200;

    // Access values
    std::println("Value at 1: {}", map[1]);

    // Iterate
    for (const auto& [key, value] : map) {
        std::println("Key: {}, Value: {}", key, value);
    }

    return 0;
}
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
