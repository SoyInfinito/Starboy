# Starboy — minimal Asteroids-like in C++ (SDL2)

This is a small starting project for an Asteroids-like game written in C++ using SDL2.

Files added:
- `CMakeLists.txt` — CMake build for the project
- `src/main.cpp` — minimal game loop, ship controls and simple asteroids

Building (recommended: vcpkg)

1) Install vcpkg (if you don't have it):

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install sdl2:x64-windows
```

2) Configure & build with CMake (adjust vcpkg toolchain path):

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -A x64
cmake --build build --config Release
.\build\Release\starboy.exe
```

Alternative (MSYS2 / pacman): install `mingw-w64-x86_64-SDL2` and build accordingly.

Controls:
- Left/Right arrows: rotate
- Up: thrust
- ESC or window close: quit

If you want, I can:
- add shooting, collisions, and asteroid splitting
- switch to SFML instead of SDL2
- set up a Git repo and commit the scaffold

---

Initial status
- **Version:** 0.1
- **Notes:** First functional prototype — the project builds with CMake/vcpkg, links SDL2 correctly, and the `starboy.exe` executable runs a minimal playable prototype (ship movement and static asteroids). This is an initial remake of Asteroids to iterate on.

If you'd like, I can push this as a new repository under your GitHub account (username `SoyInfinito`) — tell me the repository name and whether you want SSH (`git@github.com:...`) or HTTPS remote, and I'll push it (assuming your local environment has credentials/keys set up).
