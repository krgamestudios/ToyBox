# ToyBox

A test game for The Toy Programming Langauge. Requires raylib, sqlite3 and Toy.

See https://toylang.com/ for details.

<p align="center">
	<image src="noai.png" alt="No AI" width="200px" />
</p>

## Raylib for RPi

Just in case you're also working on a raspberry pi.

```bash
cmake -B build -DCMAKE_BUILD_TYPE="Release" -DPLATFORM="Desktop" -DOPENGL_VERSION="2.1"
cmake --build build
cd build && sudo make install
```

# Contributors and Special Thanks

Contributions via the [GitHub mirror](https://github.com/krgamestudios/ToyBox) are welcome, but absolutely no AI contributions will be accepted.

Suyash Dwivedi - "No AI" logo [via wikimedia, CC BY-SA 4.0](https://commons.wikimedia.org/w/index.php?curid=165477595)  
