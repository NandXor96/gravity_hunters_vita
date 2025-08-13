# Gravity Hunters PS Vita Game

Blablabla

## Build (PS Vita)

Install VITASDK (set `VITASDK` env var) and SDL2 for Vita.

```
make vita
```
Artifacts (.vpk) end up in `build/vita`.

## Build for Development (Desktop / PC)

Dependencies: SDL2 development libraries.

```
make pc
./build/pc/newton-combat
```