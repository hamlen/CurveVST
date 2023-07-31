#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"
#include <pluginterfaces/vst/ivstmidicontrollers.h>

#include "Curve.h"
#include "CurveController.h"
#include "interpolate.h"
#include <string>

CurveController::CurveController(void)
{
	LOG("CurveController constructor called and exited.\n");
}

CurveController::~CurveController(void)
{
	LOG("CurveController destructor called and exited.\n");
}

tresult PLUGIN_API CurveController::queryInterface(const char* iid, void** obj)
{
	return EditControllerEx1::queryInterface(iid, obj);
}

static void uint32_to_str16(TChar* p, uint32 n)
{
	if (n == 0)
	{
		*p++ = u'0';
		*p = 0;
	}
	else
	{
		uint32 width = 0;
		for (uint32 i = n; i; i /= 10)
			++width;

		*(p + width) = 0;
		for (uint32 i = n; width > 0; i /= 10)
			*(p + --width) = STR16("0123456789")[i % 10];
	}
}

tresult PLUGIN_API CurveController::initialize(FUnknown* context)
{
	LOG("CurveController::initialize called.\n");
	tresult result = EditControllerEx1::initialize(context);

	if (result != kResultOk)
	{
		LOG("CurveController::initialize exited prematurely with code %d.\n", result);
		return result;
	}

	char16_t cname[32] = STR16("Curve");
	char16_t iname[32] = STR16("In");
	char16_t oname[32] = STR16("Out");
	char16_t* cindex = cname + std::char_traits<char16_t>::length(cname);
	char16_t* iindex = iname + std::char_traits<char16_t>::length(iname);
	char16_t* oindex = oname + std::char_traits<char16_t>::length(oname);

	addUnit(new Unit(STR16("Curve"), kCurveUnitId));
	addUnit(new Unit(STR16("I/O Parameters"), kIOUnitId));

	for (int32 i = 0; i < num_curve_points; ++i)
	{
		uint32_to_str16(cindex, i);
		parameters.addParameter(cname, nullptr, 0, (ParamValue)i / (ParamValue)num_intervals, ParameterInfo::kCanAutomate, i, kCurveUnitId);
	}
	for (int32 i = 0; i < num_curved_params; ++i)
	{
		uint32_to_str16(iindex, i + 1);
		uint32_to_str16(oindex, i + 1);
		parameters.addParameter(iname, nullptr, 0, 0., ParameterInfo::kCanAutomate, num_curve_points + 2 * i, kIOUnitId);
		parameters.addParameter(oname, nullptr, 0, 0., ParameterInfo::kCanAutomate, num_curve_points + 2 * i + 1, kIOUnitId);
	}

	LOG("CurveController::initialize exited normally with code %d.\n", result);
	return result;
}

tresult PLUGIN_API CurveController::terminate()
{
	LOG("CurveController::terminate called.\n");
	tresult result = EditControllerEx1::terminate();
	LOG("CurveController::terminate exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API CurveController::setComponentState(IBStream* state)
{
	LOG("CurveController::setComponentState called.\n");
	if (!state)
	{
		LOG("CurveController::setComponentState failed because no state argument provided.\n");
		return kResultFalse;
	}

	IBStreamer streamer(state, kLittleEndian);
	int32 num_in_curve_points;
	if (!streamer.readInt32(num_in_curve_points))
	{
		LOG("Curve::setState unable to read curve point count.\n");
		return kResultFalse;
	}
	if (num_in_curve_points < 2)
	{
		LOG("Curve::setState received illegal curve point count of %d.\n", num_in_curve_points);
		return kResultFalse;
	}

	if (num_in_curve_points == num_curve_points)
	{
		ParamValue vals[num_curve_points];
		if (!streamer.readDoubleArray(vals, num_curve_points))
		{
			LOG("CurveController::setComponentState failed due to streamer error.\n");
			return kResultFalse;
		}
		for (uint32 i = 0; i < num_curve_points; ++i)
			setParamNormalized(i, vals[i]);
	}
	else
	{
		ParamValue in_curve[2] = { 0. };
		int32 in_cp1 = -1;
		for (int32 i = 0; i < num_curve_points; ++i)
		{
			ParamValue x = (ParamValue)i / (ParamValue)num_curve_points;
			for (; (ParamValue)in_cp1 / (ParamValue)num_in_curve_points < x; ++in_cp1)
			{
				in_curve[0] = in_curve[1];
				if (in_cp1 < num_in_curve_points)
				{
					if (!streamer.readDouble(in_curve[1]))
					{
						LOG("Curve::setState unable to read curve points during resizing from %d to %d points.\n", num_in_curve_points, num_curve_points);
						return kResultFalse;
					}
				}
			}
			setParamNormalized(i, curve_y(2, in_curve, x * (ParamValue)num_in_curve_points - (ParamValue)(in_cp1 - 1)));
		}
	}

	for (uint32 i = 0; i < num_curved_params; ++i)
	{
		ParamValue vals[2];
		if (!streamer.readDoubleArray(vals, 2))
		{
			LOG("CurveController::setComponentState stopped early with %dx3 curved params read.\n", i);
			return kResultOk;
		}
		setParamNormalized(num_curve_points + 2 * i, vals[0]);
		setParamNormalized(num_curve_points + 2 * i + 1, vals[1]);
	}

	LOG("CurveController::setComponentState exited normally.\n");
	return kResultOk;
}
