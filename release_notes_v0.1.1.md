# Starboy v0.1.1

Small feature release with in-game UI and visual improvements.

## Summary

- Adds an in-game overlay menu (Restart / Resume / Quit) with keyboard (ESC, Up/Down/Enter, R, Q) and mouse support.
- Replaces the simple triangle ship with a 7-vertex starship polygon and fixes ship nose alignment so thrust and nose point the same direction.
- Adds thrust visuals (two-layer flickering flame) while applying thrust.
- Makes `SDL2_ttf` optional at build time: the game will render a simple fallback UI if a TTF font is not available. Installing `sdl2-ttf` via `vcpkg` enables nicer text rendering.
- CMake fixes to avoid the SDL2 main-linker issue on Windows (SDL2main) so the project builds cleanly.

## Build & Run (Windows - tested)

- Build and run using the provided PowerShell helper:

  `.uild_and_run.ps1`

- Optional: install SDL2_ttf for TTF text rendering via vcpkg (x64):

  `vcpkg install sdl2-ttf:x64-windows`

- The attached asset `starboy.exe` is a Release build for Windows x64 (MSVC).

## Known Notes & Next Steps

- If the in-game text looks missing or small, install `sdl2-ttf` (see above) or bundle a TTF into `assets/` in a future update.
- Further polish (menu transitions, mouse pointer behavior, packaging for other platforms) is planned.

## Credits

Developed and released by SoyInfinito.

Enjoy!
