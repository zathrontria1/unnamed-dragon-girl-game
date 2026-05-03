# Unnamed dragon girl game (temp name)

A work in progress homebrew game made mostly in C with some inline assembly targetting the Super Nintendo Entertainment System (SNES)/Super Famicom (SFC) hardware.

An action game with dungeon maps as stages. Limited "RPG" like features are expected/to be implemented, but not the main focus.

Game features are otherwise not yet set in stone.

**Don't expect too much quality in either code or assets as I'm still figuring out how things work for the most part.**

## Technical
- Built targetting NTSC systems, which should cover the majority of owned hardware as well as all modern emulation purposes.
- Frame rate target of 30 FPS maximum (to minimize intrusive slowdown in heavier processing, especially with current compilers)
- Targeting SNES HiROM memory map. 
- SRAM is configured to be 128KB but currently not actually used. There is a functional SRAM check, though.
- Load-once assets (e.g. static background tiles, static UI tiles, and fixed sprites) are compressed in LZ4
- Load-frequent assets (anything that can be DMA'd live) are uncompressed
- Most object sprites are dynamically allocated
- A DMA queue system for updating most smaller tiles and tilemap sections
- A secondary hard-coded DMA system for known fixed animated background sections

### To-do
- More levels, and a way to swap them
- Pre-title/startup screens
- Title screen
- Game over screen
- A proper game end condition

### Sound engine technical
A bare bones sound engine is also included, as a programming test of sorts. Source code is currently not provided for it, I might do so later.

You might want to mute the audio some other way for now if you don't want to hear the music.
- Very small footprint (2KB executable binary)
- Only the sound engine program needs to be uploaded for the initial load
- All data upload (samples, sequences, and music tempo) uploaded individually from the main CPU side
- Automatic sample voice channel allocation for both music and SFX
- Closest-to-expiry system for sound dropout
- Supports up to 64 samples definition
- Each sample has ADSR/gain, pitch, and default note length explicitly set and sequencing/one shot can reference them
- Custom note format (TODO: make an Impulse Tracker to sequence format converter)
- Up to 8 tracks for music sequences
- Music sequences are not hard locked to any single voice or sample
- Loop and restart point support

## Assets
Assets are included only in binary form (i.e. ready to be placed in the ROM image as is).

## Building and testing
Note: The project expects support for the C standard library and 32-bit floating point support.

Primarily tested with [VBCC](http://www.compilers.de/vbcc.html) and [Mesen2](https://github.com/SourMesen/Mesen2)

Out of the box, VBCC will compile and link the ROM as if it were on SlowROM. You can modify the linker configuration to take advantage of FastROM speeds.

It shouldn't happen with the modified included PVSnesLib header files now, but if you get errors with regard to undefined registers, refer to the following pages to add any missing registers:
[MMIO](https://snes.nesdev.org/wiki/MMIO_registers)
[PPU](https://snes.nesdev.org/wiki/PPU_registers)
[DMA](https://snes.nesdev.org/wiki/DMA_registers)

A work-in-progress Makefile and SCM is also provided for [Calypsi](https://www.calypsi.cc/) but is currently still not fully functional.

Other C compilers are not tested at all, and may or may not result in a functional binary.

I also am currently unable to verify functionality on actual hardware or clones.

At the moment there are no plans in making the game compile for the Game Boy Advance/Windows/other platforms.
