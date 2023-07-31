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

	~CurveController(void);
};

