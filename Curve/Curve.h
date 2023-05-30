#pragma once

#undef LOGGING

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "base/source/fstring.h"
#include "pluginterfaces/base/funknown.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

constexpr ParamID num_curve_points = 11; // must be at least 2
constexpr ParamID num_curved_params = 20;
constexpr ParamID num_params = num_curve_points + 2 * num_curved_params;

constexpr ParamID num_intervals = num_curve_points - 1;

static const FUID CurveProcessorUID(0x0dc477ea, 0xf2db4745, 0xbfad7285, 0x9786d4ba);

class Curve : public AudioEffect
{
public:
	Curve(void);

	static FUnknown* createInstance(void* context)
	{
		return (IAudioProcessor*) new Curve();
	}

	tresult PLUGIN_API initialize(FUnknown* context);
	tresult PLUGIN_API terminate();
	tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup);
	tresult PLUGIN_API setActive(TBool state);
	tresult PLUGIN_API setProcessing(TBool state);
	tresult PLUGIN_API process(ProcessData& data);
	tresult PLUGIN_API getRoutingInfo(RoutingInfo& inInfo, RoutingInfo& outInfo);
	tresult PLUGIN_API setIoMode(IoMode mode);
	tresult PLUGIN_API setState(IBStream* state);
	tresult PLUGIN_API getState(IBStream* state);
	tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts);
	tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize);
	~Curve(void);

protected:
	ParamValue param_value[num_params];
};

#ifdef LOGGING
	void log(const char* format, ...);
#	define LOG(format, ...) log((format), __VA_ARGS__)
#else
#	define LOG(format, ...) 0
#endif
