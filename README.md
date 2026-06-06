# ToyBox

A test game for The Toy Programming Langauge. Requires raylib and Toy by default.

See https://toylang.com/ for details.

## Raylib for RPi

```bash
cmake -B build -DCMAKE_BUILD_TYPE="Release" -DPLATFORM="Desktop" -DOPENGL_VERSION="2.1"
cmake --build build
cd build && sudo make install
```

## Asset Credits

Some assets have been renamed as needed, and used with implied permissions.

Graphical assets provided by LogicMonkey, extracted from older collaborative projects:

* cartoon_plane.png
* drone.png
* dungeontiles.png
* explosion.png
* parvati.png

Other misc. assets:

* pixelcards.png created by [IvoryRed](https://ivoryred.itch.io/pixel-poker-cards), [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)