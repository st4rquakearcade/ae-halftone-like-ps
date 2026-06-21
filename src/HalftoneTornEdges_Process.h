/*
	HalftoneTornEdges_Process.h

	Depth-agnostic pixel processing core. The same templated routine renders
	8-bit, 16-bit and 32-bit float pixels, so detail is preserved at every
	bit depth (requirement: "keep the source detail alive").

	The two effects mirror Photoshop's Filter Gallery filters:

	  * Halftone Pattern (Sketch)  -> HTE_Halftone()
	  * Torn Edges       (Sketch)  -> HTE_Torn()

	Both are driven by an HTE_RenderParams snapshot whose fields map 1:1 to the
	Photoshop dialog controls (see docs/parameter-mapping.md).

	Coordinates
	-----------
	All sampling works in *absolute layer coordinates* (ax, ay) so the halftone
	screen and the circular ring pattern stay continuous across SmartFX render
	tiles. The HTE_Src descriptor maps an absolute coordinate to the (possibly
	offset, possibly partial) input buffer that was checked out.
*/

#pragma once

#ifndef HALFTONE_TORN_EDGES_PROCESS_H
#define HALFTONE_TORN_EDGES_PROCESS_H

#include "HalftoneTornEdges.h"
#include <cmath>

// -------------------------------------------------------------------------
// Bit-depth traits
// -------------------------------------------------------------------------

template <typename P> struct HTE_Traits;

template <> struct HTE_Traits<PF_Pixel8> {
	typedef A_u_char	Chan;
	static float Max()  { return 255.0f; }
	static bool  IsFloat() { return false; }
};
template <> struct HTE_Traits<PF_Pixel16> {
	typedef A_u_short	Chan;
	static float Max()  { return 32768.0f; }	// AE's "white" for 16bpc
	static bool  IsFloat() { return false; }
};
template <> struct HTE_Traits<PF_PixelFloat> {
	typedef PF_FpShort	Chan;
	static float Max()  { return 1.0f; }
	static bool  IsFloat() { return true; }
};

// Normalized RGBA working pixel.
struct FPix { float a, r, g, b; };

// Maps absolute layer coordinates to the checked-out input buffer.
struct HTE_Src {
	const char	*base;
	A_long		rowbytes;
	A_long		inLeft, inTop;		// input buffer's top-left in layer coords
	A_long		inW, inH;			// input buffer dimensions
	A_long		layerW, layerH;		// full layer dims (screen origin / centres)
};

// -------------------------------------------------------------------------
// Small math helpers
// -------------------------------------------------------------------------

static inline float HTE_Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
static inline float HTE_Mix(float a, float b, float t) { return a + (b - a) * t; }
static inline float HTE_Frac(float v) { return v - std::floor(v); }

static inline float HTE_Smoothstep(float e0, float e1, float x)
{
	if (e0 == e1) return x < e0 ? 0.f : 1.f;
	float t = HTE_Clamp01((x - e0) / (e1 - e0));
	return t * t * (3.f - 2.f * t);
}

static inline float HTE_Luma(const FPix &p)
{
	return 0.2126f * p.r + 0.7152f * p.g + 0.0722f * p.b;
}

// -------------------------------------------------------------------------
// Source sampling (absolute layer coordinates -> input buffer)
// -------------------------------------------------------------------------

template <typename P>
static inline FPix HTE_Read(const HTE_Src &s, int ax, int ay)
{
	int lx = ax - s.inLeft;
	int ly = ay - s.inTop;
	if (lx < 0) lx = 0; else if (lx >= s.inW) lx = s.inW - 1;
	if (ly < 0) ly = 0; else if (ly >= s.inH) ly = s.inH - 1;
	const P *p = reinterpret_cast<const P *>(s.base + (size_t)ly * s.rowbytes) + lx;
	float m = HTE_Traits<P>::Max();
	FPix out;
	out.a = (float)p->alpha / m;
	out.r = (float)p->red   / m;
	out.g = (float)p->green / m;
	out.b = (float)p->blue  / m;
	return out;
}

// Bilinear sample at an absolute (fractional) layer coordinate.
template <typename P>
static inline FPix HTE_SampleBilinear(const HTE_Src &s, float fx, float fy)
{
	float gx = fx - 0.5f, gy = fy - 0.5f;
	int x0 = (int)std::floor(gx), y0 = (int)std::floor(gy);
	float tx = gx - x0, ty = gy - y0;
	FPix p00 = HTE_Read<P>(s, x0,     y0);
	FPix p10 = HTE_Read<P>(s, x0 + 1, y0);
	FPix p01 = HTE_Read<P>(s, x0,     y0 + 1);
	FPix p11 = HTE_Read<P>(s, x0 + 1, y0 + 1);
	FPix out;
	out.a = HTE_Mix(HTE_Mix(p00.a, p10.a, tx), HTE_Mix(p01.a, p11.a, tx), ty);
	out.r = HTE_Mix(HTE_Mix(p00.r, p10.r, tx), HTE_Mix(p01.r, p11.r, tx), ty);
	out.g = HTE_Mix(HTE_Mix(p00.g, p10.g, tx), HTE_Mix(p01.g, p11.g, tx), ty);
	out.b = HTE_Mix(HTE_Mix(p00.b, p10.b, tx), HTE_Mix(p01.b, p11.b, tx), ty);
	return out;
}

template <typename P>
static inline void HTE_Write(char *base, A_long rowbytes, int x, int y, const FPix &v)
{
	P *p = reinterpret_cast<P *>(base + (size_t)y * rowbytes) + x;
	if (HTE_Traits<P>::IsFloat()) {
		p->alpha = (typename HTE_Traits<P>::Chan)v.a;	// keep float range as-is
		p->red   = (typename HTE_Traits<P>::Chan)v.r;
		p->green = (typename HTE_Traits<P>::Chan)v.g;
		p->blue  = (typename HTE_Traits<P>::Chan)v.b;
	} else {
		float m = HTE_Traits<P>::Max();
		p->alpha = (typename HTE_Traits<P>::Chan)(HTE_Clamp01(v.a) * m + 0.5f);
		p->red   = (typename HTE_Traits<P>::Chan)(HTE_Clamp01(v.r) * m + 0.5f);
		p->green = (typename HTE_Traits<P>::Chan)(HTE_Clamp01(v.g) * m + 0.5f);
		p->blue  = (typename HTE_Traits<P>::Chan)(HTE_Clamp01(v.b) * m + 0.5f);
	}
}

// -------------------------------------------------------------------------
// Deterministic value noise / fbm (drives the Torn Edges roughness)
// -------------------------------------------------------------------------

static inline float HTE_Hash(int x, int y, int seed)
{
	unsigned int h = (unsigned int)(x * 374761393 + y * 668265263 + seed * 362437);
	h = (h ^ (h >> 13)) * 1274126177u;
	h ^= (h >> 16);
	return (float)(h & 0xFFFFFF) / (float)0xFFFFFF;	// 0..1
}

static inline float HTE_ValueNoise(float x, float y, int seed)
{
	int xi = (int)std::floor(x), yi = (int)std::floor(y);
	float tx = x - xi, ty = y - yi;
	float u = tx * tx * (3.f - 2.f * tx);
	float v = ty * ty * (3.f - 2.f * ty);
	float a = HTE_Hash(xi,     yi,     seed);
	float b = HTE_Hash(xi + 1, yi,     seed);
	float c = HTE_Hash(xi,     yi + 1, seed);
	float d = HTE_Hash(xi + 1, yi + 1, seed);
	return HTE_Mix(HTE_Mix(a, b, u), HTE_Mix(c, d, u), v);
}

static inline float HTE_Fbm(float x, float y, int octaves, float gain, int seed)
{
	float sum = 0.f, amp = 0.5f, freq = 1.f, norm = 0.f;
	for (int i = 0; i < octaves; ++i) {
		sum  += amp * HTE_ValueNoise(x * freq, y * freq, seed + i * 97);
		norm += amp;
		amp  *= gain;
		freq *= 2.f;
	}
	return norm > 0.f ? sum / norm : 0.f;	// 0..1
}

// -------------------------------------------------------------------------
// Halftone tone shaping
// -------------------------------------------------------------------------

// Apply Photoshop-style "Contrast" (around mid-grey) and "Tonal Detail" gamma.
static inline float HTE_ShapeTone(float tone, const HTE_RenderParams &P)
{
	// Tonal detail: gamma keeps midtone gradation -> preserves source detail.
	if (P.halftone_tonal_detail > 0.0001f && P.halftone_tonal_detail != 1.0f)
		tone = std::pow(HTE_Clamp01(tone), 1.0f / (float)P.halftone_tonal_detail);

	// Contrast around 0.5 (PS Contrast slider 0..50 -> 0..1 here).
	float c = (float)P.halftone_contrast;
	if (c > 0.0001f) {
		float k = 1.0f + c * 6.0f;	// stronger slope as contrast rises
		tone = HTE_Clamp01(0.5f + (tone - 0.5f) * k);
	}
	return HTE_Clamp01(tone);
}

// Ink coverage for one halftone cell. Returns 0..1 (1 == full foreground ink).
static inline float HTE_CellCoverage(float tone, float localU, float localV,
									 float radialPhase, float linePhase,
									 A_long pattern, float aa)
{
	float ink = HTE_Clamp01(1.0f - tone);	// dark tone -> more ink

	if (pattern == PATTERN_DOT) {
		float dist = std::sqrt(localU * localU + localV * localV);	// cell-normalized
		float radius = std::sqrt(ink) * 0.5f * 1.41421356f;			// area-proportional
		return HTE_Smoothstep(radius + aa, radius - aa, dist);
	} else if (pattern == PATTERN_LINE) {
		float ph = HTE_Frac(linePhase);
		float dist = std::fabs(ph - 0.5f);
		float half = ink * 0.5f;
		return HTE_Smoothstep(half + aa, half - aa, dist);
	} else { // PATTERN_CIRCLE: concentric rings from the layer centre
		float ph = HTE_Frac(radialPhase);
		float dist = std::fabs(ph - 0.5f);
		float half = ink * 0.5f;
		return HTE_Smoothstep(half + aa, half - aa, dist);
	}
}

// Screen one grid for the pixel at absolute (ax,ay). channel: -1 = luma,
// 0/1/2 = R/G/B. Returns ink coverage 0..1.
template <typename P>
static float HTE_ScreenChannel(const HTE_Src &s, int ax, int ay,
							   float angle_rad, float cellSize,
							   int channel, const HTE_RenderParams &PR)
{
	float cx = s.layerW * 0.5f, cy = s.layerH * 0.5f;

	// Rotate the pixel into screen space.
	float ca = std::cos(angle_rad), sa = std::sin(angle_rad);
	float rx = (ax - cx) * ca - (ay - cy) * sa;
	float ry = (ax - cx) * sa + (ay - cy) * ca;

	// Cell index + centre (screen space).
	float cellU = std::floor(rx / cellSize);
	float cellV = std::floor(ry / cellSize);
	float centreU = (cellU + 0.5f) * cellSize;
	float centreV = (cellV + 0.5f) * cellSize;

	// Cell centre back to layer space, to sample a representative tone.
	float icx = cx + centreU * ca + centreV * sa;
	float icy = cy - centreU * sa + centreV * ca;

	FPix smp = HTE_SampleBilinear<P>(s, icx, icy);

	// "tone" is light-positive (1 = light, 0 = dark) for every grid, so dark
	// areas always become more ink.
	float tone;
	if (channel == 0)      tone = smp.r;
	else if (channel == 1) tone = smp.g;
	else if (channel == 2) tone = smp.b;
	else                   tone = HTE_Luma(smp);
	tone = HTE_ShapeTone(tone, PR);

	float invCell = 1.0f / cellSize;
	float localU = (rx - centreU) * invCell;	// [-0.5, 0.5]
	float localV = (ry - centreV) * invCell;
	// radialPhase needs a sqrt; only the concentric-ring pattern uses it.
	float radialPhase = 0.f;
	if (PR.halftone_pattern == PATTERN_CIRCLE)
		radialPhase = std::sqrt((ax - cx) * (ax - cx) + (ay - cy) * (ay - cy)) * invCell;
	float linePhase = ry * invCell;
	float aa = 0.9f * invCell;

	return HTE_CellCoverage(tone, localU, localV, radialPhase, linePhase,
							PR.halftone_pattern, aa);
}

// Full halftone for the pixel at absolute (ax,ay).
template <typename P>
static FPix HTE_Halftone(const HTE_Src &s, int ax, int ay,
						 const FPix &orig, const HTE_RenderParams &PR)
{
	FPix out = orig;
	float size = (float)PR.halftone_size;
	if (size < 1.0f) size = 1.0f;

	if (PR.halftone_colormode == COLORMODE_ORIGINAL) {
		// CMYK-like screening at classic offset angles -> keeps colour & detail.
		float base = (float)PR.halftone_angle_rad;
		float covC = HTE_ScreenChannel<P>(s, ax, ay, base + 0.2618f, size, 0, PR); // +15
		float covM = HTE_ScreenChannel<P>(s, ax, ay, base + 1.3090f, size, 1, PR); // +75
		float covY = HTE_ScreenChannel<P>(s, ax, ay, base + 0.0000f, size, 2, PR); // 0
		out.r = HTE_Clamp01(1.0f - covC);
		out.g = HTE_Clamp01(1.0f - covM);
		out.b = HTE_Clamp01(1.0f - covY);
	} else {
		float cov = HTE_ScreenChannel<P>(s, ax, ay,
										 (float)PR.halftone_angle_rad, size, -1, PR);
		if (PR.halftone_colormode == COLORMODE_TINT) {
			// Monochrome ink, but keep local luminance gradation for detail.
			float keep = HTE_Luma(orig);
			float covDetail = HTE_Clamp01(cov * 0.85f + (1.0f - keep) * 0.15f);
			out.r = HTE_Mix((float)PR.bg_r, (float)PR.fg_r, covDetail);
			out.g = HTE_Mix((float)PR.bg_g, (float)PR.fg_g, covDetail);
			out.b = HTE_Mix((float)PR.bg_b, (float)PR.fg_b, covDetail);
		} else { // COLORMODE_TWO_COLOR (Photoshop-accurate)
			out.r = HTE_Mix((float)PR.bg_r, (float)PR.fg_r, cov);
			out.g = HTE_Mix((float)PR.bg_g, (float)PR.fg_g, cov);
			out.b = HTE_Mix((float)PR.bg_b, (float)PR.fg_b, cov);
		}
	}

	// Halftone Mix: blend the screened result back toward its input.
	float mix = (float)PR.halftone_mix;
	out.r = HTE_Mix(orig.r, out.r, mix);
	out.g = HTE_Mix(orig.g, out.g, mix);
	out.b = HTE_Mix(orig.b, out.b, mix);
	out.a = orig.a;
	return out;
}

// -------------------------------------------------------------------------
// Torn Edges
// -------------------------------------------------------------------------

// Box-averaged alpha so a hard matte gains an edge ramp the tears can bite into.
template <typename P>
static float HTE_BlurAlpha(const HTE_Src &s, int ax, int ay, int radius)
{
	if (radius < 1)
		return HTE_Read<P>(s, ax, ay).a;

	int step = radius / 12; if (step < 1) step = 1;	// bound the tap count
	float sum = 0.f; int n = 0;
	for (int dy = -radius; dy <= radius; dy += step) {
		for (int dx = -radius; dx <= radius; dx += step) {
			sum += HTE_Read<P>(s, ax + dx, ay + dy).a;
			++n;
		}
	}
	return n ? sum / n : 0.f;
}

static inline float HTE_TornNoise(int ax, int ay, const HTE_RenderParams &PR)
{
	// Smoothness -> feature scale (smooth = large, rough = small features).
	float scale = HTE_Mix(6.0f, 90.0f, (float)PR.torn_smoothness);
	// Contrast -> octaves + persistence (more = grainier, rougher edge).
	int octaves = 2 + (int)std::floor((float)PR.torn_contrast * 4.0f + 0.5f);
	if (octaves > 6) octaves = 6;
	float gain = HTE_Mix(0.35f, 0.65f, (float)PR.torn_contrast);
	return HTE_Fbm((float)ax / scale, (float)ay / scale, octaves, gain, PR.torn_seed);
}

template <typename P>
static FPix HTE_Torn(const HTE_Src &s, int ax, int ay,
					 const FPix &in, const HTE_RenderParams &PR)
{
	FPix out = in;
	float n = HTE_TornNoise(ax, ay, PR);	// 0..1

	if (PR.torn_mode == TORN_MODE_WHOLE) {
		// Photoshop-accurate: 2-tone threshold with a torn (noisy) boundary.
		float L = HTE_Luma(in);
		float thr = (float)PR.torn_balance;	// Image Balance
		float rough = HTE_Mix(0.0f, 0.5f, (float)PR.torn_contrast);
		float soft  = HTE_Mix(0.005f, 0.12f, (float)PR.torn_smoothness);
		float v = L + (n - 0.5f) * 2.0f * rough;
		float t = HTE_Smoothstep(thr - soft, thr + soft, v);	// 1 -> foreground side
		out.r = HTE_Mix((float)PR.torn_bg_r, (float)PR.torn_fg_r, t);
		out.g = HTE_Mix((float)PR.torn_bg_g, (float)PR.torn_fg_g, t);
		out.b = HTE_Mix((float)PR.torn_bg_b, (float)PR.torn_fg_b, t);
		out.a = in.a;
	} else {
		// Edges Only: carve the matte's silhouette, keep interior detail.
		int radius = (int)(PR.torn_amount + 0.5f);
		if (radius < 1) radius = 1;
		float ablur = HTE_BlurAlpha<P>(s, ax, ay, radius);
		float rough = HTE_Mix(0.05f, 0.5f, (float)PR.torn_contrast);
		float soft  = HTE_Mix(0.02f, 0.25f, (float)PR.torn_smoothness);
		float bias  = HTE_Mix(-0.15f, 0.35f, (float)PR.torn_balance);
		float thr = 0.5f + (n - 0.5f) * 2.0f * rough + bias;
		float carved = HTE_Smoothstep(thr - soft, thr + soft, ablur);
		out.a = in.a * carved;	// only ever removes coverage
	}
	return out;
}

// -------------------------------------------------------------------------
// Per-pixel master routine
// -------------------------------------------------------------------------

template <typename P>
static FPix HTE_RenderPixel(const HTE_Src &s, int ax, int ay, const HTE_RenderParams &PR)
{
	FPix orig = HTE_Read<P>(s, ax, ay);
	FPix cur = orig;

	if (PR.halftone_enable)
		cur = HTE_Halftone<P>(s, ax, ay, cur, PR);

	if (PR.torn_enable)
		cur = HTE_Torn<P>(s, ax, ay, cur, PR);

	// Preserve original luminance: re-impose source brightness on the result.
	if (PR.luma_preserve) {
		float lo = HTE_Luma(orig);
		float lc = HTE_Luma(cur);
		if (lc > 0.0001f) {
			float k = lo / lc;
			cur.r = HTE_Clamp01(cur.r * k);
			cur.g = HTE_Clamp01(cur.g * k);
			cur.b = HTE_Clamp01(cur.b * k);
		}
	}

	// Master detail: blend the whole result back toward the untouched source.
	float d = (float)PR.master_detail;
	if (d > 0.0001f) {
		cur.r = HTE_Mix(cur.r, orig.r, d);
		cur.g = HTE_Mix(cur.g, orig.g, d);
		cur.b = HTE_Mix(cur.b, orig.b, d);
		cur.a = HTE_Mix(cur.a, orig.a, d);
	}
	return cur;
}

// Render the output region [outLeft,outTop, +ow,+oh) into dst.
template <typename P>
static void HTE_RenderWorld(const HTE_Src &s,
							char *dstBase, A_long dstRow,
							A_long outLeft, A_long outTop, A_long ow, A_long oh,
							const HTE_RenderParams &PR)
{
	for (int oy = 0; oy < oh; ++oy) {
		for (int ox = 0; ox < ow; ++ox) {
			int ax = outLeft + ox;
			int ay = outTop + oy;
			FPix v = HTE_RenderPixel<P>(s, ax, ay, PR);
			HTE_Write<P>(dstBase, dstRow, ox, oy, v);
		}
	}
}

#endif // HALFTONE_TORN_EDGES_PROCESS_H
