/*
	HalftoneTornEdgesPiPL.r

	PiPL resource describing the effect to After Effects / Premiere Pro.

	The Global_OutFlags values below MUST stay in sync with the flags set in
	GlobalSetup() (HalftoneTornEdges.cpp). If After Effects ever prompts that
	the cached flags differ, let it update them.

	    Global_OutFlags   = PF_OutFlag_DEEP_COLOR_AWARE   (1<<25 = 0x02000000)
	                      | PF_OutFlag_PIX_INDEPENDENT    (1<<10 = 0x00000400)
	                      | PF_OutFlag_USE_OUTPUT_EXTENT  (1<<6  = 0x00000040)
	                      = 0x02000440

	    Global_OutFlags_2 = PF_OutFlag2_SUPPORTS_SMART_RENDER (1<<10 = 0x00000400)
	                      | PF_OutFlag2_FLOAT_COLOR_AWARE     (1<<12 = 0x00001000)
	                      = 0x00001400
*/

#include "AEConfig.h"
#include "AE_EffectVers.h"

#ifndef AE_OS_WIN
	#include <AE_General.r>
#endif

resource 'PiPL' (16000) {
	{
		/* [1] */
		Kind {
			AEEffect
		},
		/* [2] */
		Name {
			"Halftone & Torn Edges"
		},
		/* [3] */
		Category {
			"Stylize"
		},

		/* [4] platform-specific code descriptors */
#ifdef AE_OS_WIN
	#ifdef AE_PROC_INTELx64
		CodeWin64X86 {"EffectMain"},
	#endif
#else
	#ifdef AE_OS_MAC
		CodeMacIntel64 {"EffectMain"},
		CodeMacARM64 {"EffectMain"},
	#endif
#endif

		/* [5] */
		AE_PiPL_Version {
			2,
			0
		},
		/* [6] */
		AE_Effect_Spec_Version {
			PF_PLUG_IN_VERSION,
			PF_PLUG_IN_SUBVERS
		},
		/* [7] */
		AE_Effect_Version {
			524289	/* 1.0 -- bump when the plugin version changes */
		},
		/* [8] */
		AE_Effect_Info_Flags {
			0
		},
		/* [9] */
		AE_Effect_Global_OutFlags {
			0x02000440
		},
		/* [10] */
		AE_Effect_Global_OutFlags_2 {
			0x00001400
		},
		/* [11] */
		AE_Effect_Match_Name {
			"ST4RQUAKE Halftone TornEdges"
		},
		/* [12] */
		AE_Reserved_Info {
			0
		}
	}
};
