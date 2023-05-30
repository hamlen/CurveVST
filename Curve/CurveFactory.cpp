#include "Curve.h"
#include "CurveController.h"

#include "public.sdk/source/main/pluginfactory.h"

#define PluginCategory "Fx"
#define PluginName "Curve"

#define PLUGINVERSION "1.0.0"

bool InitModule()
{
	LOG("InitModule called and exited.\n");
	return true;
}

bool DeinitModule()
{
	LOG("DeinitModule called and exited.\n");
	return true;
}

BEGIN_FACTORY_DEF("Kevin Hamlen",
	"https://github.com/hamlen/CurveVST",
	"no contact")

	LOG("GetPluginFactory called.\n");

	DEF_CLASS2(INLINE_UID_FROM_FUID(CurveProcessorUID),
		PClassInfo::kManyInstances,
		kVstAudioEffectClass,
		PluginName,
		Vst::kDistributable,
		PluginCategory,
		PLUGINVERSION,
		kVstVersionString,
		Curve::createInstance)

	DEF_CLASS2(INLINE_UID_FROM_FUID(CurveControllerUID),
		PClassInfo::kManyInstances,
		kVstComponentControllerClass,
		PluginName "Controller",
		0,  // unused
		"", // unused
		PLUGINVERSION,
		kVstVersionString,
		CurveController::createInstance)

END_FACTORY
