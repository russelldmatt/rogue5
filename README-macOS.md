# Rogue 5 from 4.3BSD — macOS build

The historical top-level Makefile was for typesetting the Rogue manual, and
both original Makefiles depended on BSD make fragments that are not included
with macOS. This tree contains a small modern POSIX/macOS build adaptation.

## Build

Open Terminal, change into this top-level directory, and run:

    xcode-select --install   # only if the command-line tools are not installed
    make
    ./rogue/rogue

Or build and start it in one step:

    make run

The Terminal window must be at least 80 columns by 24 rows.

## Files written while playing

The original program used `/var/games`, which normally is not writable by a
regular macOS user. This build stores these in `rogue/` instead:

- `rogue.scores`
- `rogue.lock` (temporary)
- saved games such as `rogue.save`

## Gameplay correction included

This package also corrects an original hunger-accounting bug in `move.c`.
With the uncorrected code, one ring of slow digestion could stop food
consumption completely because the game tested the parity of `moves_left`
instead of alternating by game turn. In this build, one slow-digestion ring
properly reduces consumption to about one food unit every two turns.

The armor damage calculation is unchanged: it already uses floating-point
arithmetic, so armor reduces damage as intended, subject to the original
per-hit truncation of fractional damage reductions.

## Why `-fwritable-strings` is used

This 1990 source writes generated scroll, wand, and ring names into padded
string literals. Early C systems permitted that. Modern systems place string
literals in read-only memory, so Clang's compatibility option is required for
this minimally invasive historical-source build.
