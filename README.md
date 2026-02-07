# HotRocks (Linux)

Arcade asteroid-style shooter using SDL2 + OpenGL.

## Build

### Dependencies

Install these development packages:

- `sdl2`
- `sdl2_mixer`
- `sdl2_image`
- `sdl2_ttf`
- `glew`
- `libopenmpt`
- `mesa` / OpenGL + GLU libraries
- `clang`
- `make`

### Compile

```bash
make
```

Binary output:

- `./HotRocks`

### Clean

```bash
make clean
```

## Run

Run from the project root so relative paths resolve:

```bash
./HotRocks
```

The game expects:

- assets in `./data`
- module music in `./mods`
- config in `./config.dat`

## Packaging (Arch Linux)

A `PKGBUILD` is included in the repository root.

