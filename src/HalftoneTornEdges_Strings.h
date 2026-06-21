/*
	HalftoneTornEdges_Strings.h

	String table indices and lookup declaration. Keeping all user-facing text
	in one place makes localization and PiPL/parameter consistency easier.
*/

#pragma once

#ifndef HALFTONE_TORN_EDGES_STRINGS_H
#define HALFTONE_TORN_EDGES_STRINGS_H

typedef enum {
	StrID_NONE = 0,
	StrID_Name,
	StrID_Description,

	// Halftone group
	StrID_Topic_Halftone,
	StrID_Halftone_Enable,
	StrID_Halftone_Pattern,
	StrID_Halftone_Pattern_Choices,
	StrID_Halftone_Size,
	StrID_Halftone_Contrast,
	StrID_Halftone_Angle,
	StrID_Halftone_ColorMode,
	StrID_Halftone_ColorMode_Choices,
	StrID_Halftone_FGColor,
	StrID_Halftone_BGColor,
	StrID_Halftone_TonalDetail,
	StrID_Halftone_Mix,

	// Torn edges group
	StrID_Topic_Torn,
	StrID_Torn_Enable,
	StrID_Torn_Mode,
	StrID_Torn_Mode_Choices,
	StrID_Torn_Balance,
	StrID_Torn_Smoothness,
	StrID_Torn_Contrast,
	StrID_Torn_Amount,
	StrID_Torn_Seed,
	StrID_Torn_FGColor,
	StrID_Torn_BGColor,

	// Master
	StrID_Master_Detail,
	StrID_Master_LumaPreserve,

	StrID_NUMTYPES
} StrIDType;

A_char *GetStringPtr(int strNum);

#endif // HALFTONE_TORN_EDGES_STRINGS_H
