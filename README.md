# Pactoy

A test game for The Toy Programming Langauge.

## Raylib for RPi

```bash
#why does raylib fall back to SDL?
cmake -B build -DPLATFORM=SDL -DOPENGL_VERSION=Software
cmake --build build
cd build && sudo make install
```