#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
using namespace Steinberg;
using namespace Steinberg::Vst;

ParamValue curve_y(int32 n, const ParamValue* curve, ParamValue x);
