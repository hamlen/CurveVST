#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
using namespace Steinberg;
using namespace Steinberg::Vst;

constexpr ParamValue small_double = 0.00001;

ParamValue interpolate(int32 x0, ParamValue y0, int32 x1, ParamValue y1, int32 x);
ParamValue curve_y(int32 n, const ParamValue* curve, ParamValue x);
