# WSJT-X for DX Caliber

This is a modified distribution of WSJT-X 3.3.0-beta8, upstream commit
253133e, with DX Caliber revision 8 changes. The imported upstream archive
is wsjtx-3.3.0-beta8-src.tar.gz (SHA-256
62efd935eead4c66436a25b31fff23929386fe1d424025b2bc98b544fc8b7183).
See AUTHORS, COPYING, and the notices shipped with the source.

DX Caliber changes include macOS app/profile integration, local reporting,
Prepare QSO and Find Clear TX, display and log preferences, CAT Split
recovery and diagnostics, and late FT4 decoding. This is a DX Caliber
companion build and is independently versioned from DX Caliber.

## Build on Apple Silicon

Use Xcode, CMake, Ninja and pkg-config. Build the dependencies with the
upstream macOS recipes in `.github/scripts` and a shared parent directory for
the prefixes. The app deployment target is macOS 11.0 on arm64. Use the
checksum-pinned Qt and Boost sources, the controlled GNU Fortran 14.2
toolchain, and the WSJT-X macOS recipes for libusb, PortAudio, FFTW and Hamlib.
Hamlib 4.7.2 can be built with the recipe's configure options from its
distribution tarball, which already includes the configure script.

Run `Darwin/dxc/build-macos.sh /absolute/build /absolute/staging /absolute/dependencies`.
From a source archive, set `DXC_RELEASE_REVISION` to the full commit listed
with the matching release. The dependency prefixes are `qt-prefix`, `boost-static-prefix`, `fftw-prefix`,
`libusb-prefix`, `portaudio-prefix`, `hamlib-prefix` and `gfortran-prefix`.
Review the bundled libraries, static dependencies, and their source and
license notices before signing. Developer builds do not require the
publisher's signing credentials.
