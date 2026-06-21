/*
	HalftoneTornEdges.h

	After Effects effect plugin that reproduces Photoshop's Filter Gallery
	"Halftone Pattern" (Sketch) and "Torn Edges" (Sketch) filters, with the
	same adjustment options, plus detail-preservation controls so the original
	source stays readable.

	Built against the Adobe After Effects SDK (AE_Effect.h et al.).
*/

#pragma once

#ifndef HALFTONE_TORN_EDGES_H
#define HALFTONE_TORN_EDGES_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels;
								// AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
	typedef unsigned short PixelType;
	#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include "HalftoneTornEdges_Strings.h"

/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	0
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1


/* Parameter indices.

   IMPORTANT: the order here must exactly match the order parameters are added
   in ParamsSetup(). Index 0 is always the implicit input layer.
*/
enum {
	HTE_INPUT = 0,

	// ----- Halftone group -----
	HTE_TOPIC_HALFTONE_START,
	HTE_HALFTONE_ENABLE,
	HTE_HALFTONE_PATTERN,		// popup: Dot / Circle / Line
	HTE_HALFTONE_SIZE,			// PS "Size" (1..12)
	HTE_HALFTONE_CONTRAST,		// PS "Contrast" (0..50)
	HTE_HALFTONE_ANGLE,			// extra: screen angle
	HTE_HALFTONE_COLORMODE,		// popup: Two-Color / Tint / Original Color
	HTE_HALFTONE_FGCOLOR,		// PS foreground color
	HTE_HALFTONE_BGCOLOR,		// PS background color
	HTE_HALFTONE_TONALDETAIL,	// detail-preservation (gamma on tonal response)
	HTE_HALFTONE_MIX,			// blend halftone result with its input (0..100)
	HTE_TOPIC_HALFTONE_END,

	// ----- Torn Edges group -----
	HTE_TOPIC_TORN_START,
	HTE_TORN_ENABLE,
	HTE_TORN_MODE,				// popup: Edges Only (keep detail) / Whole Image (PS 2-tone)
	HTE_TORN_BALANCE,			// PS "Image Balance" (0..50)
	HTE_TORN_SMOOTHNESS,		// PS "Smoothness" (1..15)
	HTE_TORN_CONTRAST,			// PS "Contrast" (1..25)  -> edge roughness
	HTE_TORN_AMOUNT,			// tear depth in pixels
	HTE_TORN_SEED,				// random seed
	HTE_TORN_FGCOLOR,			// foreground (whole-image mode)
	HTE_TORN_BGCOLOR,			// background (whole-image mode)
	HTE_TOPIC_TORN_END,

	// ----- Master -----
	HTE_MASTER_DETAIL,			// blend whole effect with original (detail preservation)
	HTE_MASTER_LUMA_PRESERVE,	// keep original luminance modulation

	HTE_NUM_PARAMS
};

/* Stable parameter disk IDs (must never change once shipped). */
enum {
	HTE_TOPIC_HALFTONE_START_DISK_ID = 1,
	HTE_HALFTONE_ENABLE_DISK_ID,
	HTE_HALFTONE_PATTERN_DISK_ID,
	HTE_HALFTONE_SIZE_DISK_ID,
	HTE_HALFTONE_CONTRAST_DISK_ID,
	HTE_HALFTONE_ANGLE_DISK_ID,
	HTE_HALFTONE_COLORMODE_DISK_ID,
	HTE_HALFTONE_FGCOLOR_DISK_ID,
	HTE_HALFTONE_BGCOLOR_DISK_ID,
	HTE_HALFTONE_TONALDETAIL_DISK_ID,
	HTE_HALFTONE_MIX_DISK_ID,
	HTE_TOPIC_HALFTONE_END_DISK_ID,

	HTE_TOPIC_TORN_START_DISK_ID,
	HTE_TORN_ENABLE_DISK_ID,
	HTE_TORN_MODE_DISK_ID,
	HTE_TORN_BALANCE_DISK_ID,
	HTE_TORN_SMOOTHNESS_DISK_ID,
	HTE_TORN_CONTRAST_DISK_ID,
	HTE_TORN_AMOUNT_DISK_ID,
	HTE_TORN_SEED_DISK_ID,
	HTE_TORN_FGCOLOR_DISK_ID,
	HTE_TORN_BGCOLOR_DISK_ID,
	HTE_TOPIC_TORN_END_DISK_ID,

	HTE_MASTER_DETAIL_DISK_ID,
	HTE_MASTER_LUMA_PRESERVE_DISK_ID
};

/* Popup choices (1-based, as AE popups are). */
enum {
	PATTERN_DOT = 1,
	PATTERN_CIRCLE,
	PATTERN_LINE
};

enum {
	COLORMODE_TWO_COLOR = 1,	// Photoshop-accurate: foreground/background
	COLORMODE_TINT,				// monochrome tint that keeps tonal detail
	COLORMODE_ORIGINAL			// per-channel (CMYK-like) halftone: keeps color & detail
};

enum {
	TORN_MODE_EDGES = 1,		// tear only the matte / silhouette (keeps interior detail)
	TORN_MODE_WHOLE				// Photoshop-accurate 2-tone threshold over whole image
};


/* Resolved, render-time parameter snapshot shared by all bit depths. */
typedef struct {
	// Halftone
	A_Boolean		halftone_enable;
	A_long			halftone_pattern;
	double			halftone_size;			// cell size in pixels (already scaled by downsample)
	double			halftone_contrast;		// 0..1
	double			halftone_angle_rad;		// radians
	A_long			halftone_colormode;
	double			fg_r, fg_g, fg_b;		// 0..1
	double			bg_r, bg_g, bg_b;		// 0..1
	double			halftone_tonal_detail;	// gamma-ish, >0
	double			halftone_mix;			// 0..1

	// Torn edges
	A_Boolean		torn_enable;
	A_long			torn_mode;
	double			torn_balance;			// 0..1 threshold
	double			torn_smoothness;		// 0..1 (low = rough, high = smooth)
	double			torn_contrast;			// 0..1 roughness amount
	double			torn_amount;			// tear depth in pixels (scaled by downsample)
	A_long			torn_seed;
	double			torn_fg_r, torn_fg_g, torn_fg_b;
	double			torn_bg_r, torn_bg_g, torn_bg_b;

	// Master
	double			master_detail;			// 0..1 blend of final result with original
	A_Boolean		luma_preserve;
} HTE_RenderParams;


extern "C" {

	DllExport
	PF_Err
	EffectMain(
		PF_Cmd			cmd,
		PF_InData		*in_data,
		PF_OutData		*out_data,
		PF_ParamDef		*params[],
		PF_LayerDef		*output,
		void			*extra);

}

#endif // HALFTONE_TORN_EDGES_H
