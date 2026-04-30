# VampireToyvivors

A test game for The Toy Programming Langauge. Requires raylib and Toy by default.

See https://toylang.com/ for details.

Note: Vampire Survivors is a simple design, so it makes sense to test Toy with it - this project isn't intended for retail.

## Raylib for RPi

```bash
cmake -B build -DCMAKE_BUILD_TYPE="Release" -DPLATFORM="Desktop" -DOPENGL_VERSION="2.1"
cmake --build build
cd build && sudo make install
```

## Asset Sources

https://www.spriters-resource.com/arcade/pacman/asset/52631/
