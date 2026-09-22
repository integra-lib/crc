# crc

CRC-8/NRSC-5, CRC-16/CCITT-FALSE and CRC-32/ISO-HDLC, bit-by-bit and constexpr.

Part of [integra-lib](https://gitlab.integrasources.com/internal-projects/integra-lib).
Header-only C++20, no exceptions, no RTTI.

## Use it

```bash
git submodule add ../crc.git external/integra/crc
```

```cmake
add_subdirectory(external/integra/crc)
target_link_libraries(app PRIVATE Integra::crc)
```

```cpp
#include <integra/crc.hpp>
```

## Develop it

```bash
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
```

Tests are built only when this repository is the top-level project, so a consumer
never builds them. Style and pipeline come from the `ci-shared` submodule; run
`git submodule update --init` before `pre-commit`.
