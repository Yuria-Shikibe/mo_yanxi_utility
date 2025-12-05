# mo_yanxi_utility

A modern C++ utility library leveraging the latest C++20/23 features, including Modules and Concepts.

## Features

This library provides a collection of utilities organized into modules:

- **algo**: Algorithms.
- **concurrent**: Concurrency utilities.
- **container**: Custom container implementations.
- **dim2**: 2D geometry/math utilities.
- **math**: Mathematical functions, including constrained systems and quad trees.
- **string**: String manipulation utilities.
- **pointer**: Pointer management.
- **general**: General purpose utilities.
- **generic**: Generic programming helpers.

## Requirements

- **Build System**: [xmake](https://xmake.io/)
- **Compiler**: A C++ compiler with C++20/23 support (e.g., MSVC, GCC, Clang) that supports C++ Modules.
- **Vector Extensions**: AVX/AVX2 support is enabled by default.

## Building

To build the library:

```bash
xmake
```

To build and run tests:

```bash
xmake build mo_yanxi.utility.test
xmake run mo_yanxi.utility.test
```

## Usage

The library uses C++ Modules. You can import the modules in your code:

```cpp
import mo_yanxi.utility;
// or specific modules like
// import mo_yanxi.math;
```

## Integration with CMake

The project provides a task to generate a `CMakeLists.txt` file with enforced C++ standard:

```bash
xmake gencmake --std=23
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
