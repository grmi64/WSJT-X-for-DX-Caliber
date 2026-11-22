#!/bin/bash
set -euo pipefail

source_dir="$(cd "$(dirname "$0")/../.." && pwd)"
build_dir="${1:?Pass an absolute build directory}"
output_dir="${2:?Pass an absolute staging directory}"
deps="${3:?Pass the absolute directory containing the upstream dependency prefixes}"
deployment_target=11.0
qt="$deps/qt-prefix"
boost="$deps/boost-static-prefix"
fftw="$deps/fftw-prefix"
libusb="$deps/libusb-prefix"
portaudio="$deps/portaudio-prefix"
hamlib="$deps/hamlib-prefix"
fortran="$deps/gfortran-prefix"
triplet=aarch64-apple-darwin20.0
gcc_lib="$fortran/lib/gcc/$triplet/14.2.0"
scripts="$source_dir/.github/scripts"
sdk="$(xcrun --sdk macosx --show-sdk-path)"
homebrew_prefix="${DXC_BREW_PREFIX:-/opt/homebrew}"
app="$output_dir/WSJT-X for DX Caliber.app"

test ! -e "$app"
"$scripts/verify-macos-static-archives.sh" --target "$deployment_target" --arch arm64 --mode error \
  "$gcc_lib/libgfortran.a" "$gcc_lib/libquadmath.a" "$gcc_lib/libgcc.a" \
  "$gcc_lib/libgomp.a" "$hamlib/lib/libhamlib.a" "$boost/lib/"libboost*.a
export PKG_CONFIG_PATH="$qt/lib/pkgconfig:$boost/lib/pkgconfig:$fftw/lib/pkgconfig:$libusb/lib/pkgconfig:$portaudio/lib/pkgconfig:$hamlib/lib/pkgconfig"
export PKG_CONFIG_LIBDIR="$PKG_CONFIG_PATH"
cmake -S "$source_dir" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  "-DCMAKE_OSX_DEPLOYMENT_TARGET=$deployment_target" -DCMAKE_OSX_ARCHITECTURES=arm64 \
  "-DCMAKE_PREFIX_PATH=$qt;$boost;$fftw;$libusb;$portaudio;$hamlib" \
  "-DQt5_DIR=$qt/lib/cmake/Qt5" "-DBOOST_ROOT=$boost" \
  -DBoost_USE_STATIC_LIBS=ON -DBoost_NO_SYSTEM_PATHS=ON "-DFFTW3_ROOT_DIR=$fftw" \
  "-DCMAKE_Fortran_COMPILER=$fortran/bin/$triplet-gfortran" \
  "-DCMAKE_Fortran_FLAGS=-static-libgcc -static-libgfortran -static-libquadmath -isysroot $sdk" \
  "-DCMAKE_EXE_LINKER_FLAGS=$gcc_lib/libgfortran.a $gcc_lib/libquadmath.a $gcc_lib/libgcc.a" \
  "-DWSJT_MACOS_LIBGOMP_LIBRARY=$gcc_lib/libgomp.a" \
  "-DCMAKE_INSTALL_RPATH=$qt/lib;$fftw/lib;$libusb/lib;$portaudio/lib;$hamlib/lib" \
  -DWSJT_SKIP_MANPAGES=ON -DWSJT_SKIP_BUNDLE_FIXUP=ON \
  -DWSJT_ENABLE_TESTS=OFF -DWSJT_GENERATE_DOCS=OFF
cmake --build "$build_dir" --parallel "${DXC_BUILD_JOBS:-8}"
cmake --install "$build_dir" --prefix "$output_dir"
mv "$output_dir/wsjtx.app" "$app"

(
  cd "$build_dir"
  "$scripts/deploy-macos-app.sh" --app "$app" --workspace-prefix "$deps" \
    --qt-prefix "$qt" --boost-prefix "$boost" --fftw-prefix "$fftw" \
    --libusb-prefix "$libusb" --portaudio-prefix "$portaudio" \
    --fortran-prefix "$fortran" --fortran-original-prefix /opt/gfortran \
    --gcc-lib "$gcc_lib" --homebrew-prefix "$homebrew_prefix"
)
"$scripts/verify-macos-artifacts.sh" --target "$deployment_target" --arch arm64 "$app"
mkdir -p "$app/Contents/Resources/Licenses"
for notice in COPYING AUTHORS doc/BOOST-LICENSE.txt DXC-RELEASE.md; do
  cp "$source_dir/$notice" "$app/Contents/Resources/Licenses/"
done
