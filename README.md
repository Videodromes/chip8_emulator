# CHIP-8 Emulator
My attempt at a [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) interpreter/emulator written in C. SDL2 was used for graphics.

## Dependencies
* gcc
    * For Windows, `mingw-19.0.exe` from https://nuwen.net/mingw.html provides a portable, self-contained "distro" of gcc, make, and other utilities.
* make
* SDL2
    * Windows: Extract `SDL2-devel-2.30.7-mingw.zip` to root directory. https://github.com/libsdl-org/SDL/releases/tag/release-2.30.7
    * Linux: `sudo apt-get install libsdl2-2.0-0` See https://wiki.libsdl.org/SDL2/Installation
## Building
`make`

## Usage
`.\chip8.exe`
* Currently only displays the window that SDL is drawing/rendering.

## Technical References/Resources
* [CHIP-8 Technical Reference](https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Technical-Reference)
* [CHIP-8 Instruction Set](https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set)
* [CHIP-8 Test Suite](https://github.com/Timendus/chip8-test-suite)
