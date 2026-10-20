# Better Aiming

Modernizes staff and binocular aiming with velocity controls, crosshair options, direct spell switching, and optional C-stick aim while moving.

## Features

- Independent `Original`, `Free`, and `Centered` aim modes for Fire Blaster and Freeze Blast
- `Original` keeps the original aiming behavior while still allowing speed, vertical range, and axis inversion to be adjusted
- Velocity-based aiming in `Free` and `Centered` modes with no automatic recentering when the stick is released
- Optional modern binocular controls with matched horizontal and vertical inertia
- Separate horizontal and vertical inversion for staff aiming and binoculars
- Shared flight sensitivity and axis inversion for Arwing and CloudRunner controls
- Adjustable aiming speed and vertical range
- Optional Freeze Blast crosshair
- Direct switching between Fire Blaster and Freeze Blast while either spell is active
- Optional C-stick aiming while moving
- Optional Blaster HUD Fix to prevent the HUD from briefly disappearing while using Fire Blaster or Freeze Blast

## Supported Platforms

Tested and supported:

- Windows x64
- macOS arm64 (Apple Silicon)

Automated builds are also produced for additional platforms, but they are not currently listed as supported because they have not been tested.

## Requirements

- Foxhollow 1.0.20 or newer
- A working Star Fox Adventures installation configured for Foxhollow
- Windows x64 or macOS arm64 for a tested configuration

## Installation

1. Open the [latest GitHub release](../../releases/latest).
2. Download the latest `better-aiming-<version>.zip` release archive.
3. Open Foxhollow's `mods` directory.
4. Create a folder named `com.derneonleon.better-aiming` inside the `mods` directory.
5. Extract the contents of the downloaded ZIP directly into that folder.
6. Start Foxhollow and configure Better Aiming from the mod settings.

The installed files should begin like this:

```text
mods/
└── com.derneonleon.better-aiming/
    ├── mod.json
    └── lib/
```

The release archive contains the available platform builds inside `lib/`.

## Building on Windows

Run `BUILD-WINDOWS.cmd` from the repository root.

The script uses `FOXHOLLOW_SOURCE_DIR` when it points to an existing Foxhollow checkout. If no checkout is available, it downloads Foxhollow and its required submodules into `.deps`.

Required build tools:

- Visual Studio with Desktop development with C++
- C++ Clang tools for Windows
- CMake
- Git

The finished release archive is written to `dist/better-aiming-1.2.0.zip`.

## Automated Builds

The repository includes `.github/workflows/cross-platform-build.yml` for automated cross-platform builds.

## Support

If you want to support my projects, you can donate on [Ko-fi](https://ko-fi.com/derneonleon).

## AI Disclosure

This project was created with AI assistance. AI was used during development for code generation, review, debugging, and project maintenance.

## License

CC0 1.0 Universal. See `LICENSE`.
