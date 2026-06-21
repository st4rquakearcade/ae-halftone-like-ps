# Building the plugin

This is a native After Effects effect plugin written against the **Adobe After
Effects SDK** (C/C++). The SDK is required to compile and cannot be
redistributed, so you download it once and point the build at it.

> Heads-up: this repository contains the complete, ready-to-compile source.
> It has **not** been compiled in CI (the AE SDK and AE itself aren't available
> in the authoring environment). Build it locally with the steps below; if the
> compiler flags any SDK-version drift (struct/suite renames between SDK
> releases), they'll be localized and easy to resolve.

## 1. Get the SDK

1. Download the After Effects SDK from
   <https://developer.adobe.com/after-effects/>.
2. Unzip it. You'll have a folder like `AfterEffectsSDK/Examples` containing
   `Headers/`, `Util/`, and `Resources/`.
3. Note that path — it's your `AESDK_ROOT`.

## 2. Build with CMake (recommended, cross-platform)

```bash
cmake -B build -DAESDK_ROOT=/path/to/AfterEffectsSDK/Examples
cmake --build build --config Release
```

Output:

* **Windows:** `HalftoneTornEdges.aex`
* **macOS:** `HalftoneTornEdges.plugin` (universal: x86_64 + arm64)

### Toolchains
* **Windows:** Visual Studio 2022 (Desktop C++). Run from a *x64 Native Tools*
  prompt so `cl.exe`/`PiPLtool` are found.
* **macOS:** Xcode + command line tools (provides `Rez`).

## 3. The PiPL resource

After Effects discovers effects through a compiled **PiPL** resource
(`src/HalftoneTornEdgesPiPL.r`). The CMake script compiles it automatically when
it can find the tools:

* **macOS** uses `Rez` to embed `HalftoneTornEdges.rsrc` in the bundle.
* **Windows** uses the SDK's `PiPLtool` to turn the `.r` into a `.rrc`, which
  `win/HalftoneTornEdges.rc` `#include`s.

If a tool isn't found, CMake prints a warning. To compile the PiPL by hand:

**macOS**
```bash
Rez -d AE_OS_MAC=1 -useDF \
    -i "$AESDK_ROOT/Headers" -i "$AESDK_ROOT/Resources" \
    src/HalftoneTornEdgesPiPL.r \
    -o HalftoneTornEdges.plugin/Contents/Resources/HalftoneTornEdges.rsrc
```

**Windows** (x64 Native Tools prompt)
```bat
cl /EP /D "MSWindows=1" /D "AE_OS_WIN=1" ^
   /I "%AESDK_ROOT%\Headers" /I "%AESDK_ROOT%\Resources" ^
   src\HalftoneTornEdgesPiPL.r > HalftoneTornEdgesPiPL.rr
PiPLtool HalftoneTornEdgesPiPL.rr win\HalftoneTornEdgesPiPL.rrc
```

If AE ever warns that the plugin's cached out-flags differ from the PiPL, let it
update them — the authoritative flags are set in `GlobalSetup()`.

## 4. Install

Copy the built `.aex` / `.plugin` into the After Effects plug-ins folder:

* **Windows:** `C:\Program Files\Adobe\Adobe After Effects <ver>\Support Files\Plug-ins\`
* **macOS:** `/Applications/Adobe After Effects <ver>/Plug-ins/`

Restart After Effects. The effect appears under **Effect → Stylize → Halftone &
Torn Edges**.

## 5. Alternative: build inside the SDK sample tree

If you prefer the SDK's own project setup, drop `src/*` into a copy of one of
the `Examples/Effect` SmartFX samples (e.g. `Skeleton`) and replace its sources
and `.r` with these. The provided `.vcxproj`/Xcode samples already wire up
PiPLtool/Rez.
