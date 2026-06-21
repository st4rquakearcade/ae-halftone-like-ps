# Halftone & Torn Edges — After Effects plugin

A native After Effects effect plugin that reproduces Photoshop's **Filter
Gallery → Sketch → Halftone Pattern** and **Filter Gallery → Sketch → Torn
Edges** filters, with their adjustment options carried over to After Effects —
and with extra controls so the original source detail stays readable.

It is built against the **Adobe After Effects SDK** (C++), renders via **SmartFX**
at **8 / 16 / 32-bit** color depth, and can be applied to **any layer**
(precompose a composition to treat it as a single layer).

## Why this design

The brief had three requirements:

1. **Works on any composition.** It's a standard layer effect under
   *Effect → Stylize → Halftone & Torn Edges*. Apply it to footage, solids,
   shape/text layers, or a precomp.
2. **Keeps the source detail alive.** Photoshop's two filters are *two-tone* by
   nature, which throws away color and detail. This plugin keeps the
   Photoshop-accurate two-tone modes **and** adds detail-preserving paths:
   per-channel (CMYK) color halftone, a tonal-detail/gamma control, an
   *Edges Only* torn mode that tears just the silhouette and leaves the picture
   intact, plus global *Preserve Detail* / *Preserve Luminance* controls.
   16/32-bit rendering avoids banding.
3. **Mirrors Photoshop's options.** Every Photoshop control is present with the
   same meaning; where an exact match is impossible it's approximated as closely
   as practical. See **[docs/parameter-mapping.md](docs/parameter-mapping.md)**.

## Controls at a glance

**Halftone Pattern** — Enable · Pattern Type (Dot/Circle/Line) · Size · Contrast ·
Screen Angle · Color (Two-Color / Tint / Original CMYK) · Foreground/Background ·
Tonal Detail · Halftone Mix

**Torn Edges** — Enable · Apply To (Edges Only / Whole Image) · Image Balance ·
Smoothness · Contrast · Tear Amount · Random Seed · Foreground/Background

**Master** — Preserve Original Detail · Preserve Original Luminance

Full Photoshop↔plugin mapping: **[docs/parameter-mapping.md](docs/parameter-mapping.md)**.

## Build & install

You need the Adobe After Effects SDK (free, not redistributable). Then:

```bash
cmake -B build -DAESDK_ROOT=/path/to/AfterEffectsSDK/Examples
cmake --build build --config Release
```

Copy the resulting `HalftoneTornEdges.aex` (Windows) or
`HalftoneTornEdges.plugin` (macOS) into the After Effects *Plug-ins* folder and
restart AE. Full instructions: **[docs/build.md](docs/build.md)**.

> Note: the source is complete and ready to compile, but it has not been built
> in CI because the AE SDK and After Effects are not available in the authoring
> environment. Build locally per `docs/build.md`.

## Project layout

```
src/
  HalftoneTornEdges.h           Params, enums, version, render-param struct
  HalftoneTornEdges.cpp         Command dispatch, parameter UI, SmartFX render
  HalftoneTornEdges_Process.h   Depth-agnostic halftone + torn-edge math
  HalftoneTornEdges_Strings.*   User-facing strings
  HalftoneTornEdgesPiPL.r       PiPL resource (effect registration)
win/   HalftoneTornEdges.rc      Windows resource glue for the compiled PiPL
mac/   Info.plist                macOS .plugin bundle metadata
docs/  build.md, parameter-mapping.md
CMakeLists.txt                  Cross-platform build
```

## How the effects work (brief)

* **Halftone** — for each pixel, the rotated halftone cell it belongs to is
  found, the cell's representative tone is sampled (with Contrast + Tonal Detail
  shaping), and the dot/line/ring coverage is computed with anti-aliasing.
  *Original Color* mode runs three screens (CMY) at classic offset angles to keep
  color and detail.
* **Torn Edges** — a deterministic fbm noise field (scaled by Smoothness, made
  grainier by Contrast, seeded by Random Seed) perturbs a threshold.
  *Whole Image* applies it as a two-tone luminance split; *Edges Only* applies it
  to a box-blurred matte so the silhouette tears while the interior is preserved.

## License

See [LICENSE](LICENSE).
