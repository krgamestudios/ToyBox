# Pactoy

A test game for The Toy Programming Langauge. Requires raylib and Toy by default.

See https://toylang.com/ for details.

## Raylib for RPi

```bash
#why does raylib fall back to SDL?
cmake -B build -DPLATFORM=SDL -DOPENGL_VERSION=Software
cmake --build build
cd build && sudo make install
```