# Building Rogue 5 on macOS

1. Install Apple's Command Line Tools if needed:

       xcode-select --install

2. Open Terminal and change into this directory (the one containing this
   `Makefile` and `main.c`). The compiler flag `-fwritable-strings` is
   intentional: this 1990 source writes generated item names into padded
   string literals, which old C systems allowed.

3. Build and run:

       make
       ./rogue

Rogue requires a Terminal window of at least 80 columns by 24 rows.

The score and lock files are kept in the current directory as
`rogue.scores` and `rogue.lock`, avoiding the original system-wide
`/var/games` location.

To remove generated build files:

       make clean

## Included gameplay correction

This source includes a small correction in `move.c` for the original
slow-digestion ring bug. The half-rate hunger cases now alternate using the
game-turn counter, rather than the parity of the remaining-food value.
