#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

enum CurveUnitId : Steinberg::Vst::UnitID
{
	kCurveUnitId = 1,
	kIOUnitId = 2
};


static const FUID CurveControllerUID(0xadd77d71, 0x69d049be, 0x8aa5fa81, 0x46c747f8);

class CurveController : public EditControllerEx1
{
public:
	CurveController(void);

	static FUnknown* createInstance(void* context)
	{
		return (IEditController*) new CurveController;
	}

	DELEGATE_REFCOUNT(EditControllerEx1)
	tresult PLUGIN_API queryInterface(const char* iid, void** obj) SMTG_OVERRIDE;

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;

	tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;

	// Uncomment to add a GUI
	// IPlugView * PLUGIN_API createView (const char * name);

	// Uncomment to override default EditController behavior
	// tresult PLUGIN_API setState(IBStream* state);
	// tresult PLUGIN_API getState(IBStream* state);
	// tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value);
	// tresult PLUGIN_API getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string);
	// tresult PLUGIN_API getParamValueByString(ParamID tag, TChar* string, ParamValue& valueNormalized);

	~CurveController(void);
};

