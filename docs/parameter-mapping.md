# Photoshop → After Effects parameter mapping

This plugin reproduces two Photoshop *Filter Gallery* filters and their
controls. Photoshop's filters are designed around a fixed-resolution preview, so
a few options are remapped to ranges that make sense for arbitrary AE comps. The
goal everywhere: same *behaviour*, and where an exact match isn't possible, the
closest practical equivalent.

## Halftone Pattern (Filter Gallery → Sketch → Halftone Pattern)

| Photoshop control | Range (PS) | Plugin control | Range (plugin) | Notes |
|---|---|---|---|---|
| Size | 1–12 | **Size** | 1–200 px | Cell size in pixels. As a rule of thumb `plugin_size ≈ PS_size × 3`. Pixels (instead of 1–12) let it scale to any comp resolution. Auto-scales with comp downsample. |
| Contrast | 0–50 | **Contrast** | 0–100 | Doubled range. Steepens the tone→dot-size response. |
| Pattern Type | Circle / Dot / Line | **Pattern Type** | Dot / Circle / Line | Same three patterns. |
| Foreground color | (toolbox FG) | **Foreground Color** | color | Ink color. Photoshop pulls this from the toolbox; here it's an explicit control. |
| Background color | (toolbox BG) | **Background Color** | color | Paper color. |

### Extra controls (not in Photoshop, added for AE)

| Plugin control | Purpose |
|---|---|
| **Screen Angle** | Rotates the halftone screen. Photoshop's Halftone Pattern has no angle; this matches the classic print look and helps avoid moiré. |
| **Color** | `Two-Color (FG/BG)` = Photoshop-accurate two-tone. `Tint` = monochrome but keeps tonal gradation. `Original Color (CMYK)` = per-channel screening that keeps the source's color and most of its detail (requirement: *keep the source detail alive*). |
| **Tonal Detail** | Gamma on the tone response. Higher = more midtone gradation preserved. |
| **Halftone Mix** | Blends the screened result back toward the pre-halftone image. |

## Torn Edges (Filter Gallery → Sketch → Torn Edges)

| Photoshop control | Range (PS) | Plugin control | Range (plugin) | Notes |
|---|---|---|---|---|
| Image Balance | 0–50 | **Image Balance** | 0–100 | Threshold between the two regions (whole-image mode) / how much is torn away (edges mode). |
| Smoothness | 1–15 | **Smoothness** | 0–100 | Low = rough, jagged tears; high = smooth, large tears. Controls noise feature scale + transition softness. |
| Contrast | 1–25 | **Contrast** | 0–100 | Edge roughness / graininess (noise octaves & persistence). |
| Foreground color | (toolbox FG) | **Foreground Color** | color | Whole-image mode only. |
| Background color | (toolbox BG) | **Background Color** | color | Whole-image mode only. |

### Extra controls (not in Photoshop, added for AE)

| Plugin control | Purpose |
|---|---|
| **Apply To** | `Edges Only (keep detail)` tears just the layer's matte/silhouette so the interior stays fully intact — the best match for *keep the source detail alive*. `Whole Image (2-tone)` is the Photoshop-accurate two-tone torn threshold. |
| **Tear Amount** | Depth of the tearing in pixels (how far the rough edge bites into a hard matte). Auto-scales with comp downsample. |
| **Random Seed** | Changes the noise pattern; animate or vary per layer. |

## Master controls

| Plugin control | Purpose |
|---|---|
| **Preserve Original Detail** | Blends the entire effect back toward the untouched source (0 = full effect, 100 = original). |
| **Preserve Original Luminance** | Re-imposes the source's brightness on the result, so shading/detail reads through stylization. |

## Why two “detail” strategies?

Photoshop's Halftone Pattern and Torn Edges are both **two-tone** filters — by
design they discard color and most tonal detail. Requirement #2 asks for the
opposite. Rather than break Photoshop fidelity, the plugin keeps the accurate
two-tone modes **and** adds detail-preserving paths:

* Halftone → `Original Color (CMYK)` + `Tonal Detail` + `Halftone Mix`
* Torn Edges → `Edges Only` mode (tears the silhouette, keeps the picture)
* Global → `Preserve Original Detail` / `Preserve Original Luminance`
