# HotRocks (Linux)

Arcade asteroid-style shooter using SDL2 + OpenGL.

![Hotrocks screenshot](screenshots/screenshot.png)

## Install

### Debian / Ubuntu (`.deb`)

```bash
sudo apt install ./hotrocks_<version>_amd64.deb
```

If `apt` is unavailable:

```bash
sudo dpkg -i hotrocks_<version>_amd64.deb
sudo apt -f install
```

### Fedora / RHEL / openSUSE (`.rpm`)

```bash
sudo dnf install ./hotrocks-<version>-1.x86_64.rpm
```

If your distro uses `rpm` directly:

```bash
sudo rpm -Uvh hotrocks-<version>-1.x86_64.rpm
```

### Arch Linux (AUR)

Install from AUR:

- https://aur.archlinux.org/packages/hotrocks

## Usage

Place all your favorite mod files in:

- `/usr/share/hotrocks/mods` (packaged install)
- `./mods` (running from source tree)

You can get more module files at [The Mod Archive](https://modarchive.org/).

### Default Controls

- `P` pause

Player 1 (keyboard):
- `Left Arrow` rotate left
- `Right Arrow` rotate right
- `Up Arrow` thrust
- `Space` fire
- `Down Arrow` shield

Player 2 (keyboard):
- `A` rotate left
- `D` rotate right
- `W` thrust
- `Left Shift` fire
- `S` shield

Mouse controls (enabled by default):
- `Left Click` fire
- `Right Click` thrust
- `Middle Click` shield

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
