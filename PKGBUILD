pkgname=hotrocks
pkgver=1.0.0
pkgrel=1
pkgdesc="SDL2/OpenGL arcade asteroid shooter"
arch=('x86_64')
url="https://github.com/rabfulton/Hotrocks"
license=('MIT')
depends=(
  'sdl2'
  'sdl2_mixer'
  'sdl2_image'
  'sdl2_ttf'
  'glew'
  'libopenmpt'
  'glu'
  'libglvnd'
)
makedepends=('make' 'clang')
source=()
sha256sums=()

build() {
  cd "$startdir"
  make clean
  make
}

package() {
  cd "$startdir"

  install -d "$pkgdir/usr/share/hotrocks"
  cp -a HotRocks "$pkgdir/usr/share/hotrocks/"
  cp -a data "$pkgdir/usr/share/hotrocks/"
  cp -a mods "$pkgdir/usr/share/hotrocks/"
  install -m644 default.dat "$pkgdir/usr/share/hotrocks/default.dat"
  install -m644 highscore.dat "$pkgdir/usr/share/hotrocks/highscore.dat"

  install -d "$pkgdir/usr/bin"
  cat > "$pkgdir/usr/bin/hotrocks" << 'EOF'
#!/bin/sh
cd /usr/share/hotrocks || exit 1
exec ./HotRocks "$@"
EOF
  chmod 755 "$pkgdir/usr/bin/hotrocks"

  install -d "$pkgdir/usr/share/applications"
  install -m644 "$startdir/hotrocks.desktop" "$pkgdir/usr/share/applications/hotrocks.desktop"

  install -d "$pkgdir/usr/share/icons/hicolor/scalable/apps"
  install -m644 "$startdir/hotrocks.svg" "$pkgdir/usr/share/icons/hicolor/scalable/apps/hotrocks.svg"

  install -d "$pkgdir/usr/share/licenses/$pkgname"
  install -m644 "$startdir/LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
