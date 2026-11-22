#!/bin/bash
set -euo pipefail
source_dir="$(cd "$(dirname "$0")/../.." && pwd)"
build_dir="${1:?Pass an absolute build directory}"
output_dir="${2:?Pass an absolute staging directory}"
brew_prefix="${DXC_BREW_PREFIX:-/opt/homebrew}"
qt="$brew_prefix/opt/qt@5"

cmake -S "$source_dir" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_Fortran_COMPILER="$brew_prefix/bin/gfortran-15" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DCMAKE_OSX_ARCHITECTURES=arm64 \
  "-DCMAKE_PREFIX_PATH=$qt;$brew_prefix" \
  -DWSJT_SKIP_MANPAGES=ON -DWSJT_SKIP_BUNDLE_FIXUP=ON \
  -DWSJT_ENABLE_TESTS=${DXC_WSJT_TESTS:-OFF} -DWSJT_GENERATE_DOCS=OFF
cmake --build "$build_dir" --target wsjtx jt9 --parallel "${DXC_BUILD_JOBS:-8}"

app="$output_dir/WSJT-X for DX Caliber.app"
test ! -e "$app"
mkdir -p "$output_dir"
ditto "$build_dir/wsjtx.app" "$app"
"$qt/bin/macdeployqt" "$app" -always-overwrite -no-strip "-executable=$app/Contents/MacOS/jt9"
mkdir -p "$app/Contents/Resources/Licenses"
for file in COPYING AUTHORS doc/BOOST-LICENSE.txt; do
  cp "$source_dir/$file" "$app/Contents/Resources/Licenses/"
done
