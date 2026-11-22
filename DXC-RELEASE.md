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

Use macOS 14 or later, Xcode Command Line Tools, Homebrew CMake, Ninja,
Qt 5, GCC 15, Boost, FFTW, PortAudio, Hamlib, and pkgconf. Run
`Darwin/dxc/build-macos.sh /absolute/build-directory /absolute/staging-directory`.
Use the dependency versions and sources listed with the matching binary
package for release reproduction. Developer ID signing is a distribution step;
developer builds do not require the publisher's signing credentials.
