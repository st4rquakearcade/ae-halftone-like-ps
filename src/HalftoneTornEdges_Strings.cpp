/*
	HalftoneTornEdges_Strings.cpp

	User-facing string table.
*/

#include "HalftoneTornEdges.h"

typedef struct {
	A_u_long	index;
	A_char		str[256];
} TableString;

TableString g_strs[StrID_NUMTYPES] = {
	{StrID_NONE,						""},
	{StrID_Name,						"Halftone & Torn Edges"},
	{StrID_Description,					"Photoshop Filter Gallery 'Halftone Pattern' and 'Torn Edges' for After Effects, "
										"with detail-preserving controls. Apply to any layer (precompose a comp to treat it as one)."},

	{StrID_Topic_Halftone,				"Halftone Pattern"},
	{StrID_Halftone_Enable,				"Enable Halftone"},
	{StrID_Halftone_Pattern,			"Pattern Type"},
	{StrID_Halftone_Pattern_Choices,	"Dot|Circle|Line"},
	{StrID_Halftone_Size,				"Size"},
	{StrID_Halftone_Contrast,			"Contrast"},
	{StrID_Halftone_Angle,				"Screen Angle"},
	{StrID_Halftone_ColorMode,			"Color"},
	{StrID_Halftone_ColorMode_Choices,	"Two-Color (FG/BG)|Tint (Mono, keep detail)|Original Color (CMYK)"},
	{StrID_Halftone_FGColor,			"Foreground Color"},
	{StrID_Halftone_BGColor,			"Background Color"},
	{StrID_Halftone_TonalDetail,		"Tonal Detail"},
	{StrID_Halftone_Mix,				"Halftone Mix"},

	{StrID_Topic_Torn,					"Torn Edges"},
	{StrID_Torn_Enable,					"Enable Torn Edges"},
	{StrID_Torn_Mode,					"Apply To"},
	{StrID_Torn_Mode_Choices,			"Edges Only (keep detail)|Whole Image (2-tone)"},
	{StrID_Torn_Balance,				"Image Balance"},
	{StrID_Torn_Smoothness,				"Smoothness"},
	{StrID_Torn_Contrast,				"Contrast"},
	{StrID_Torn_Amount,					"Tear Amount"},
	{StrID_Torn_Seed,					"Random Seed"},
	{StrID_Torn_FGColor,				"Foreground Color"},
	{StrID_Torn_BGColor,				"Background Color"},

	{StrID_Master_Detail,				"Preserve Original Detail"},
	{StrID_Master_LumaPreserve,			"Preserve Original Luminance"}
};

A_char *GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}
