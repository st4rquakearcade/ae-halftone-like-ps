/*
	HalftoneTornEdges.cpp

	Command dispatch, parameter UI, and SmartFX rendering for the
	"Halftone & Torn Edges" After Effects effect.

	SmartFX is used for all bit depths (8/16/32-bit float) so detail is kept at
	full precision. Both effects sample neighbouring pixels (halftone cell tone,
	torn-edge matte blur), so PreRender expands the requested input region.
*/

#include "HalftoneTornEdges.h"
#include "HalftoneTornEdges_Process.h"

// ------------------------------------------------------------------
// About / Global setup
// ------------------------------------------------------------------

static PF_Err
About(PF_InData *in_data, PF_OutData *out_data,
	  PF_ParamDef *params[], PF_LayerDef *output)
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	suites.ANSICallbacksSuite1()->sprintf(
		out_data->return_msg,
		"%s v%d.%d\r%s",
		GetStringPtr(StrID_Name),
		MAJOR_VERSION, MINOR_VERSION,
		GetStringPtr(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err
GlobalSetup(PF_InData *in_data, PF_OutData *out_data,
			PF_ParamDef *params[], PF_LayerDef *output)
{
	out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION,
									  BUG_VERSION, STAGE_VERSION, BUILD_VERSION);

	out_data->out_flags  = PF_OutFlag_DEEP_COLOR_AWARE |
						   PF_OutFlag_PIX_INDEPENDENT |	// each output pixel computed independently
						   PF_OutFlag_USE_OUTPUT_EXTENT;

	out_data->out_flags2 = PF_OutFlag2_SUPPORTS_SMART_RENDER |
						   PF_OutFlag2_FLOAT_COLOR_AWARE;

	return PF_Err_NONE;
}

// ------------------------------------------------------------------
// Parameters
// ------------------------------------------------------------------

static PF_Err
ParamsSetup(PF_InData *in_data, PF_OutData *out_data,
			PF_ParamDef *params[], PF_LayerDef *output)
{
	PF_Err		err = PF_Err_NONE;
	PF_ParamDef	def;

	// ---- Halftone group ----
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC(GetStringPtr(StrID_Topic_Halftone), HTE_TOPIC_HALFTONE_START_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOX(GetStringPtr(StrID_Halftone_Enable), "", TRUE,
					0, HTE_HALFTONE_ENABLE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(GetStringPtr(StrID_Halftone_Pattern), 3, PATTERN_DOT,
				 GetStringPtr(StrID_Halftone_Pattern_Choices), HTE_HALFTONE_PATTERN_DISK_ID);

	// PS "Size" is 1..12; we expose pixels-per-cell for AE precision.
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Halftone_Size),
						 1, 200, 1, 40, 6,
						 PF_Precision_TENTHS, 0, 0, HTE_HALFTONE_SIZE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Halftone_Contrast),
						 0, 100, 0, 100, 0,
						 PF_Precision_TENTHS, 0, 0, HTE_HALFTONE_CONTRAST_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE(GetStringPtr(StrID_Halftone_Angle), 0, HTE_HALFTONE_ANGLE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(GetStringPtr(StrID_Halftone_ColorMode), 3, COLORMODE_TWO_COLOR,
				 GetStringPtr(StrID_Halftone_ColorMode_Choices), HTE_HALFTONE_COLORMODE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(GetStringPtr(StrID_Halftone_FGColor), 0, 0, 0, HTE_HALFTONE_FGCOLOR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(GetStringPtr(StrID_Halftone_BGColor), 255, 255, 255, HTE_HALFTONE_BGCOLOR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Halftone_TonalDetail),
						 0.2, 5, 0.2, 3, 1,
						 PF_Precision_HUNDREDTHS, 0, 0, HTE_HALFTONE_TONALDETAIL_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Halftone_Mix),
						 0, 100, 0, 100, 100,
						 PF_Precision_TENTHS, 0, 0, HTE_HALFTONE_MIX_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(HTE_TOPIC_HALFTONE_END_DISK_ID);

	// ---- Torn Edges group ----
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC(GetStringPtr(StrID_Topic_Torn), HTE_TOPIC_TORN_START_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOX(GetStringPtr(StrID_Torn_Enable), "", FALSE,
					0, HTE_TORN_ENABLE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(GetStringPtr(StrID_Torn_Mode), 2, TORN_MODE_EDGES,
				 GetStringPtr(StrID_Torn_Mode_Choices), HTE_TORN_MODE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Torn_Balance),
						 0, 100, 0, 100, 50,
						 PF_Precision_TENTHS, 0, 0, HTE_TORN_BALANCE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Torn_Smoothness),
						 0, 100, 0, 100, 50,
						 PF_Precision_TENTHS, 0, 0, HTE_TORN_SMOOTHNESS_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Torn_Contrast),
						 0, 100, 0, 100, 50,
						 PF_Precision_TENTHS, 0, 0, HTE_TORN_CONTRAST_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Torn_Amount),
						 0, 200, 0, 80, 20,
						 PF_Precision_TENTHS, 0, 0, HTE_TORN_AMOUNT_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Torn_Seed),
						 0, 9999, 0, 100, 1,
						 PF_Precision_INTEGER, 0, 0, HTE_TORN_SEED_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(GetStringPtr(StrID_Torn_FGColor), 0, 0, 0, HTE_TORN_FGCOLOR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(GetStringPtr(StrID_Torn_BGColor), 255, 255, 255, HTE_TORN_BGCOLOR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(HTE_TOPIC_TORN_END_DISK_ID);

	// ---- Master ----
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(GetStringPtr(StrID_Master_Detail),
						 0, 100, 0, 100, 0,
						 PF_Precision_TENTHS, 0, 0, HTE_MASTER_DETAIL_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOX(GetStringPtr(StrID_Master_LumaPreserve), "", FALSE,
					0, HTE_MASTER_LUMA_PRESERVE_DISK_ID);

	out_data->num_params = HTE_NUM_PARAMS;
	return err;
}

// ------------------------------------------------------------------
// Parameter snapshot
// ------------------------------------------------------------------

static double FixToDeg(PF_Fixed f) { return (double)f / 65536.0; }

static PF_Err
ReadParams(PF_InData *in_data, HTE_RenderParams *rp)
{
	PF_Err		err = PF_Err_NONE;
	PF_ParamDef	p;

	float dsx = (float)in_data->downsample_x.num / (float)in_data->downsample_x.den;
	float dsy = (float)in_data->downsample_y.num / (float)in_data->downsample_y.den;
	float ds  = 0.5f * (dsx + dsy);
	if (ds <= 0.f) ds = 1.f;

	#define HTE_CHECKOUT(IDX) \
		AEFX_CLR_STRUCT(p); \
		ERR(PF_CHECKOUT_PARAM(in_data, (IDX), in_data->current_time, \
							  in_data->time_step, in_data->time_scale, &p));

	// Halftone
	HTE_CHECKOUT(HTE_HALFTONE_ENABLE)   rp->halftone_enable   = p.u.bd.value;
	HTE_CHECKOUT(HTE_HALFTONE_PATTERN)  rp->halftone_pattern  = p.u.pd.value;
	HTE_CHECKOUT(HTE_HALFTONE_SIZE)     rp->halftone_size     = p.u.fs_d.value * ds;
	HTE_CHECKOUT(HTE_HALFTONE_CONTRAST) rp->halftone_contrast = p.u.fs_d.value / 100.0;
	HTE_CHECKOUT(HTE_HALFTONE_ANGLE)    rp->halftone_angle_rad= FixToDeg(p.u.ad.value) * 3.14159265358979 / 180.0;
	HTE_CHECKOUT(HTE_HALFTONE_COLORMODE)rp->halftone_colormode= p.u.pd.value;

	HTE_CHECKOUT(HTE_HALFTONE_FGCOLOR)
	rp->fg_r = p.u.cd.value.red / 255.0; rp->fg_g = p.u.cd.value.green / 255.0; rp->fg_b = p.u.cd.value.blue / 255.0;
	HTE_CHECKOUT(HTE_HALFTONE_BGCOLOR)
	rp->bg_r = p.u.cd.value.red / 255.0; rp->bg_g = p.u.cd.value.green / 255.0; rp->bg_b = p.u.cd.value.blue / 255.0;

	HTE_CHECKOUT(HTE_HALFTONE_TONALDETAIL) rp->halftone_tonal_detail = p.u.fs_d.value;
	HTE_CHECKOUT(HTE_HALFTONE_MIX)         rp->halftone_mix          = p.u.fs_d.value / 100.0;

	// Torn
	HTE_CHECKOUT(HTE_TORN_ENABLE)     rp->torn_enable     = p.u.bd.value;
	HTE_CHECKOUT(HTE_TORN_MODE)       rp->torn_mode       = p.u.pd.value;
	HTE_CHECKOUT(HTE_TORN_BALANCE)    rp->torn_balance    = p.u.fs_d.value / 100.0;
	HTE_CHECKOUT(HTE_TORN_SMOOTHNESS) rp->torn_smoothness = p.u.fs_d.value / 100.0;
	HTE_CHECKOUT(HTE_TORN_CONTRAST)   rp->torn_contrast   = p.u.fs_d.value / 100.0;
	HTE_CHECKOUT(HTE_TORN_AMOUNT)     rp->torn_amount     = p.u.fs_d.value * ds;
	HTE_CHECKOUT(HTE_TORN_SEED)       rp->torn_seed       = (A_long)(p.u.fs_d.value + 0.5);

	HTE_CHECKOUT(HTE_TORN_FGCOLOR)
	rp->torn_fg_r = p.u.cd.value.red / 255.0; rp->torn_fg_g = p.u.cd.value.green / 255.0; rp->torn_fg_b = p.u.cd.value.blue / 255.0;
	HTE_CHECKOUT(HTE_TORN_BGCOLOR)
	rp->torn_bg_r = p.u.cd.value.red / 255.0; rp->torn_bg_g = p.u.cd.value.green / 255.0; rp->torn_bg_b = p.u.cd.value.blue / 255.0;

	// Master
	HTE_CHECKOUT(HTE_MASTER_DETAIL)        rp->master_detail = p.u.fs_d.value / 100.0;
	HTE_CHECKOUT(HTE_MASTER_LUMA_PRESERVE) rp->luma_preserve = p.u.bd.value;

	#undef HTE_CHECKOUT
	return err;
}

// ------------------------------------------------------------------
// SmartFX
//
// We request the *whole* input layer in PreRender so that the position-
// dependent halftone screen and the circular ring pattern stay continuous no
// matter how After Effects tiles the output. Sampling then works in absolute
// layer coordinates (see HalftoneTornEdges_Process.h).
// ------------------------------------------------------------------

typedef struct {
	HTE_RenderParams	params;
	PF_LRect			input_rect;		// checked-out input region (layer coords)
	A_long				layerW, layerH;	// full layer dimensions
} PreRenderData;

static void
DeletePreRenderData(void *pre_render_dataPV)
{
	if (pre_render_dataPV)
		free(pre_render_dataPV);
}

static PF_Err
SmartPreRender(PF_InData *in_data, PF_OutData *out_data, PF_PreRenderExtra *extra)
{
	PF_Err err = PF_Err_NONE;

	PreRenderData *infoP = (PreRenderData *)malloc(sizeof(PreRenderData));
	if (!infoP) return PF_Err_OUT_OF_MEMORY;
	AEFX_CLR_STRUCT(*infoP);

	ERR(ReadParams(in_data, &infoP->params));

	infoP->layerW = in_data->width;
	infoP->layerH = in_data->height;

	// Ask for the entire input layer.
	PF_RenderRequest req = extra->input->output_request;
	req.rect.left   = 0;
	req.rect.top    = 0;
	req.rect.right  = in_data->width;
	req.rect.bottom = in_data->height;
	req.preserve_rgb_of_zero_alpha = TRUE;

	PF_CheckoutResult in_result;
	AEFX_CLR_STRUCT(in_result);
	ERR(extra->cb->checkout_layer(in_data->effect_ref, HTE_INPUT, HTE_INPUT,
								  &req, in_data->current_time, in_data->time_step,
								  in_data->time_scale, &in_result));

	if (!err) {
		infoP->input_rect = in_result.result_rect;

		// We produce output everywhere AE asked for it.
		extra->output->result_rect     = extra->input->output_request.rect;
		extra->output->max_result_rect = extra->input->output_request.rect;

		extra->output->pre_render_data = infoP;
		extra->output->delete_pre_render_data_func = DeletePreRenderData;
	} else {
		free(infoP);
	}

	return err;
}

static PF_Err
SmartRender(PF_InData *in_data, PF_OutData *out_data, PF_SmartRenderExtra *extra)
{
	PF_Err err = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PreRenderData *infoP = (PreRenderData *)extra->input->pre_render_data;
	if (!infoP) return PF_Err_INTERNAL_STRUCT_DAMAGED;

	PF_EffectWorld *input_worldP = NULL, *output_worldP = NULL;

	ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref, HTE_INPUT, &input_worldP));
	ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

	if (!err && input_worldP && output_worldP) {

		PF_PixelFormat fmt = PF_PixelFormat_INVALID;
		{
			PF_WorldSuite2 *wsP = NULL;
			ERR(in_data->pica_basicP->AcquireSuite(
				kPFWorldSuite, kPFWorldSuiteVersion2, (const void **)&wsP));
			if (!err && wsP) {
				ERR(wsP->PF_GetPixelFormat(input_worldP, &fmt));
				in_data->pica_basicP->ReleaseSuite(kPFWorldSuite, kPFWorldSuiteVersion2);
			}
		}

		HTE_Src s;
		s.base     = (const char *)input_worldP->data;
		s.rowbytes = input_worldP->rowbytes;
		s.inLeft   = infoP->input_rect.left;
		s.inTop    = infoP->input_rect.top;
		s.inW      = input_worldP->width;
		s.inH      = input_worldP->height;
		s.layerW   = infoP->layerW;
		s.layerH   = infoP->layerH;

		char  *dstBase = (char *)output_worldP->data;
		A_long dstRow  = output_worldP->rowbytes;
		A_long ow      = output_worldP->width;
		A_long oh      = output_worldP->height;
		A_long outLeft = extra->input->output_request.rect.left;
		A_long outTop  = extra->input->output_request.rect.top;

		switch (fmt) {
			case PF_PixelFormat_ARGB128:
				HTE_RenderWorld<PF_PixelFloat>(s, dstBase, dstRow, outLeft, outTop, ow, oh, infoP->params);
				break;
			case PF_PixelFormat_ARGB64:
				HTE_RenderWorld<PF_Pixel16>(s, dstBase, dstRow, outLeft, outTop, ow, oh, infoP->params);
				break;
			case PF_PixelFormat_ARGB32:
			default:
				HTE_RenderWorld<PF_Pixel8>(s, dstBase, dstRow, outLeft, outTop, ow, oh, infoP->params);
				break;
		}
	}

	if (input_worldP)
		extra->cb->checkin_layer_pixels(in_data->effect_ref, HTE_INPUT);

	// infoP is owned by AE and released via DeletePreRenderData().
	return err;
}

// ------------------------------------------------------------------
// Classic render fallback (8/16-bit hosts without SmartFX path)
// ------------------------------------------------------------------

static PF_Err
LegacyRender(PF_InData *in_data, PF_OutData *out_data,
			 PF_ParamDef *params[], PF_LayerDef *output)
{
	PF_Err err = PF_Err_NONE;

	HTE_RenderParams rp;
	AEFX_CLR_STRUCT(rp);
	ERR(ReadParams(in_data, &rp));
	if (err) return err;

	PF_LayerDef *input = &params[HTE_INPUT]->u.ld;

	HTE_Src s;
	s.base     = (const char *)input->data;
	s.rowbytes = input->rowbytes;
	s.inLeft   = 0;
	s.inTop    = 0;
	s.inW      = input->width;
	s.inH      = input->height;
	s.layerW   = input->width;
	s.layerH   = input->height;

	char  *dstBase = (char *)output->data;
	A_long dstRow  = output->rowbytes;
	A_long w       = output->width;
	A_long h       = output->height;

	bool deep = PF_WORLD_IS_DEEP(output);
	if (deep)
		HTE_RenderWorld<PF_Pixel16>(s, dstBase, dstRow, 0, 0, w, h, rp);
	else
		HTE_RenderWorld<PF_Pixel8>(s, dstBase, dstRow, 0, 0, w, h, rp);

	return err;
}

// ------------------------------------------------------------------
// Entry point
// ------------------------------------------------------------------

extern "C" DllExport
PF_Err
PluginDataEntryFunction2(
	PF_PluginDataPtr		inPtr,
	PF_PluginDataCB2		inPluginDataCallBackPtr,
	SPBasicSuite			*inSPBasicSuitePtr,
	const char				*inHostName,
	const char				*inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT_EXT2(
		inPtr, inPluginDataCallBackPtr,
		"Halftone & Torn Edges",			// Name
		"ST4RQUAKE Halftone TornEdges",		// Match Name (must be globally unique & stable)
		"Stylize",							// Category
		AE_RESERVED_INFO,					// Reserved Info
		"EffectMain",						// Entry point
		"https://github.com/st4rquakearcade/ae-halftone-like-ps"); // support URL

	return result;
}

PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err err = PF_Err_NONE;

	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data, out_data, params, output);
				break;
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(in_data, out_data, params, output);
				break;
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(in_data, out_data, params, output);
				break;
			case PF_Cmd_RENDER:
				err = LegacyRender(in_data, out_data, params, output);
				break;
			case PF_Cmd_SMART_PRE_RENDER:
				err = SmartPreRender(in_data, out_data, (PF_PreRenderExtra *)extra);
				break;
			case PF_Cmd_SMART_RENDER:
				err = SmartRender(in_data, out_data, (PF_SmartRenderExtra *)extra);
				break;
			default:
				break;
		}
	} catch (PF_Err &thrown_err) {
		err = thrown_err;
	}
	return err;
}
