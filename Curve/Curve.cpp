#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include "Curve.h"
#include "CurveController.h"
#include "interpolate.h"

// Return the first time strictly after t when a new segment of the automation
// curve starts (or INT32_MAX if no segments start after t).
// Also reposition the index to the point at time offset t (or the total number of segments).
int32 StatefulParamQueue::next_segment_start(int32 t, ParamValue& y)
{
	if (!q) return INT32_MAX;

	int32 n = q->getPointCount();
	if (n < 0) n = 0; // should never happen (host provided bad point count)

	int32 t1;
	if (index < n)
		q->getPoint(index, t1, y);
	else
	{
		y = init_y;
		if (n > 0)
			q->getPoint(n - 1, t1, y);
		t1 = INT32_MAX;
	}

	if (t1 <= t)
	{
		while ((uint32)++index < (uint32)n)
		{
			q->getPoint(index, t1, y);
			if (t1 > t)
				return t1;
		}
		index = n;
		return INT32_MAX;
	}
	else
	{
		while (--index >= 0)
		{
			int32 t0;
			ParamValue y0;
			q->getPoint(index, t0, y0);
			if (t0 <= t)
			{
				++index;
				return t1;
			}
			t1 = t0;
			y = y0;
		}
		index = 0;
		return t1;
	}
}

ParamValue StatefulParamQueue::value_at(int32 t)
{
	ParamValue y1;
	int32 t1 = next_segment_start(t, y1);
	int32 t0 = -1;
	ParamValue y0 = init_y;
	if (index > 0)
		q->getPoint(index - 1, t0, y0);
	return interpolate(t0, y0, t1, y1, t);
}

Curve::Curve(void)
{
	LOG("Curve constructor called.\n");
	setControllerClass(FUID(CurveControllerUID));
	processSetup.maxSamplesPerBlock = INT32_MAX;
	for (int32 i = 0; i < num_curve_points; ++i)
		param_value[i] = (ParamValue)i / (ParamValue)num_intervals;
	LOG("Curve constructor exited.\n");
}

Curve::~Curve(void)
{
	LOG("Curve destructor called and exited.\n");
}

tresult PLUGIN_API Curve::initialize(FUnknown* context)
{
	LOG("Curve::initialize called.\n");
	tresult result = AudioEffect::initialize(context);

	if (result != kResultOk)
	{
		LOG("Curve::initialize failed with code %d.\n", result);
		return result;
	}

	for (int32 i = 0; i < num_curve_points; ++i)
		param_value[i] = (ParamValue)i / (ParamValue)num_intervals;

	LOG("Curve::initialize exited normally.\n");
	return kResultOk;
}

tresult PLUGIN_API Curve::terminate()
{
	LOG("Curve::terminate called.\n");
	tresult result = AudioEffect::terminate();
	LOG("Curve::terminate exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API Curve::setActive(TBool state)
{
	LOG("Curve::setActive called.\n");
	tresult result = AudioEffect::setActive(state);
	LOG("Curve::setActive exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API Curve::setIoMode(IoMode mode)
{
	LOG("Curve::setIoMode called and exited.\n");
	return kResultOk;
}

tresult PLUGIN_API Curve::setProcessing(TBool state)
{
	LOG("Curve::setProcessing called and exited.\n");
	initial_values_sent = false;
	return kResultOk;
}

tresult PLUGIN_API Curve::setState(IBStream* state)
{
	LOG("Curve::setState called.\n");

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
		if (!streamer.readDoubleArray(param_value, num_curve_points))
		{
			LOG("Curve::setState unable to read %u curve points.\n", num_curve_points);
			return kResultFalse;
		}
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
			param_value[i] = curve_y(2, in_curve, x * (ParamValue)num_in_curve_points - (ParamValue)(in_cp1 - 1));
		}
	}

	for (uint32 i = 0; i < num_curved_params; ++i)
	{
		if (!streamer.readDoubleArray(&param_value[num_curve_points + 2 * i], 2))
		{
			LOG("Curve::setState stopped early with %dx3 curved params read.\n", i);
			return kResultOk;
		}
	}

	LOG("Curve::setState exited successfully.\n");
	return kResultOk;
}

tresult PLUGIN_API Curve::getState(IBStream* state)
{
	LOG("Curve::getState called.\n");

	IBStreamer streamer(state, kLittleEndian);
	if (!streamer.writeInt32(num_curve_points) || !streamer.writeDoubleArray(param_value, num_params))
	{
		LOG("Curve::getState failed due to streamer error.\n");
		return kResultFalse;
	}

	LOG("Curve::getState exited successfully.\n");
	return kResultOk;
}

tresult PLUGIN_API Curve::setupProcessing(ProcessSetup& newSetup)
{
	LOG("Curve::setupProcessing called.\n");
	processContextRequirements.flags = 0;
	tresult result = AudioEffect::setupProcessing(newSetup);
	LOG("Curve::setupProcessing exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API Curve::canProcessSampleSize(int32 symbolicSampleSize)
{
	LOG("Curve::canProcessSampleSize called with arg %d and exited.\n", symbolicSampleSize);
	return (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API Curve::getRoutingInfo(RoutingInfo& inInfo, RoutingInfo& outInfo)
{
	LOG("Curve::getRoutingInfo called.\n");
	if (inInfo.mediaType == kEvent && inInfo.busIndex == 0)
	{
		outInfo = inInfo;
		LOG("Curve::getRoutingInfo exited with success.\n");
		return kResultOk;
	}
	else
	{
		LOG("Curve::getRoutingInfo exited with failure.\n");
		return kResultFalse;
	}
}

tresult PLUGIN_API Curve::process(ProcessData& data)
{
	if (data.numSamples < 0)
		return kResultFalse;

	// We shouldn't be asked for any audio, but process it anyway (emit silence) to tolerate uncompliant hosts.
	for (int32 i = 0; i < data.numOutputs; ++i)
	{
		for (int32 j = 0; j < data.outputs[i].numChannels; ++j)
		{
			bool is32bit = (data.symbolicSampleSize == kSample32);
			if (is32bit || (data.symbolicSampleSize == kSample64))
			{
				void* channelbuffer = is32bit ? (void*)data.outputs[i].channelBuffers32[j] : (void*)data.outputs[i].channelBuffers64[j];
				size_t datasize = is32bit ? sizeof(*data.outputs[i].channelBuffers32[j]) : sizeof(*data.outputs[i].channelBuffers64[j]);
				if (channelbuffer)
					memset(channelbuffer, 0, data.numSamples * datasize);
			}
		}
		data.outputs[i].silenceFlags = (1ULL << data.outputs[i].numChannels) - 1;
	}

	if (data.numSamples <= 0)
		return kResultTrue;

	// Organize parameter change queues into arrays.
	StatefulParamQueue cp_in[num_curve_points] = {};
	IParamValueQueue* param_in[num_curved_params] = {};
	IParamValueQueue* param_out[num_curved_params] = {};
	bool curve_changed = false;
	if (data.inputParameterChanges)
	{
		int32 n = data.inputParameterChanges->getParameterCount();
		for (int32 i = 0; i < n; ++i)
		{
			IParamValueQueue* q = data.inputParameterChanges->getParameterData(i);
			if (!q) continue;
			ParamID id = q->getParameterId();
			if (id < num_curve_points)
			{
				cp_in[id].q = q;
				if (q->getPointCount() > 0)
					curve_changed = true;
			}
			else if (id < num_curve_points + 2 * num_curved_params && (id - num_curve_points) % 2 == 0)
				param_in[(id - num_curve_points) / 2] = q;
		}
	}
	if (data.outputParameterChanges)
	{
		int32 n = data.outputParameterChanges->getParameterCount();
		for (int32 i = 0; i < n; ++i)
		{
			IParamValueQueue* q = data.outputParameterChanges->getParameterData(i);
			if (!q) continue;
			ParamID id = q->getParameterId();
			if ((num_curve_points <= id) && (id < num_curve_points + 2 * num_curved_params) && ((id - num_curve_points) % 2 == 1))
				param_out[(id - num_curve_points) / 2] = q;
		}
	}

	// Initialize automation curves for curve function points from saved values.
	for (int32 cp = 0; cp < num_curve_points; ++cp)
		cp_in[cp].init_y = param_value[cp];

	// Sample-accurate translation of each in-parameter to each out-parameter:
	for (ParamID id = 0; id < num_curved_params; ++id)
	{
		// Quick exit for parameters that didn't change.
		const int32 n = param_in[id] ? param_in[id]->getPointCount() : 0;
		if (n <= 0 && !curve_changed)
			continue;

		// Reset all automation curves of curve function points (for efficiency).
		for (int32 cp = 0; cp < num_curve_points; ++cp)
			cp_in[cp].index = 0;

		// For each segment of the in-parameter's automation curve...
		int32 t0 = -1;
		ParamValue x0 = param_value[num_curve_points + 2 * id];
		for (int32 i = 0; i <= n; ++i)
		{
			// Let (t0,x0)--(t1,x1) be the start and end points of this in-parameter curve segment.
			int32 t1 = data.numSamples;
			ParamValue x1 = x0;
			if (i < n)
				param_in[id]->getPoint(i, t1, x1);

			// Line (t0,x0)--(t1,x1) spans a range of x-values bounded by a series of curve points [cp0, cp1, ...].
			// As time progresses from t0 to t1, the values of the curve points cp0, cp1, ... might also change
			// according to their own automation curves.  We therefore advance a point (t,x) starting at (t0,x0)
			// through the segment to (t1,x1), finding places where line (t0,x0)--(t1,x1) crosses into new x-intervals
			// [cp0_x,cp1_x] of the curve function, or where the automation curves for the points cp0 and cp1 that
			// define the slope of the current curve function segment change non-linearly.
			// Invariant: Point (t,x) is the previously outputted point.
			constexpr ParamValue intervals = (ParamValue)(num_curve_points - 1);
			int32 t = t0;
			ParamValue x = x0;
			do
			{
				// Let (t_cp, x_cp) be the point at the next time t_cp > t that line (t0,x0)--(t1,x1) crosses into
				// a new interval of the curve function, or (t1,x1) if no interval boundaries are crossed.
				int32 t_cp = t1;
				const ParamValue x_nudged = std::nextafter(x, x1);
				if (x0 != x1)
				{
					constexpr auto floord = static_cast<ParamValue(*)(ParamValue)>(std::floor);
					constexpr auto ceild = static_cast<ParamValue(*)(ParamValue)>(std::ceil);
					int32 cp = (x0 <= x1 ? ceild : floord)(x_nudged * intervals);
					if (cp < 0) cp = 0; else if (num_curve_points <= cp) cp = num_curve_points - 1;
					const ParamValue x_cp = (ParamValue)cp / intervals;
					t_cp = std::round((ParamValue)t0 + (ParamValue)(t1 - t0) * ((x_cp - x0) / (x1 - x0)));
					if (t_cp <= t) t_cp = t + 1;
					if (t_cp > t1) t_cp = t1;
				}

				// Find the indexes cp0 and cp1 of the curve function points that bound interval (x, x_cp].
				int32 cp0 = (int32)(std::floor(x_nudged * intervals) + 0.1);
				if (cp0 < 0) cp0 = 0; else if (cp0 >= num_curve_points) cp0 = num_curve_points - 1;
				int32 cp1 = (int32)(std::ceil(x_nudged * intervals) + 0.1);
				if (cp1 < 0) cp1 = 0; else if (cp1 >= num_curve_points) cp1 = num_curve_points - 1;

				// Advance (t,x) to the earliest of the following three events:
				// (a) line segment (t0,x0)--(t1,x1) moves into a new interval of the curve function or ends (at t_cp),
				// (b) bounding curve-point cp0's automation curve switches to a new linear segment (at t_cp0), or
				// (c) bounding curve-point cp1's automation curve switches to a new linear segment (at t_cp1).
				ParamValue y;
				const int32 t_cp0 = cp_in[cp0].next_segment_start(t, y);
				const int32 t_cp1 = cp_in[cp1].next_segment_start(t, y);
				t = t_cp;
				if (t_cp0 < t) t = t_cp0;
				if (t_cp1 < t) t = t_cp1;
				x = interpolate(t0, x0, t1, x1, t);

				// Compute the y-value returned by the curve function for x at time t.
				if (cp0 == cp1)
				{
					y = cp_in[cp0].value_at(t);
				}
				else
				{
					const ParamValue cp0_x = (ParamValue)cp0 / intervals;
					const ParamValue cp1_x = (ParamValue)cp1 / intervals;
					const ParamValue cp0_y = cp_in[cp0].value_at(t);
					const ParamValue cp1_y = cp_in[cp1].value_at(t);
					y = cp0_y + (cp1_y - cp0_y) * ((x - cp0_x) / (cp1_x - cp0_x));
				}

				// Output point (x,y) and update stored param values.
				if (t < data.numSamples)
				{
					int32 dummy;
					if (!param_out[id] && data.outputParameterChanges)
						param_out[id] = data.outputParameterChanges->addParameterData(num_curve_points + 1 + 2 * id, dummy);
					if (param_out[id])
						param_out[id]->addPoint(t, y, dummy);
				}
				param_value[num_curve_points + 2 * id] = x;
				param_value[num_curve_points + 2 * id + 1] = y;
			} while (t < t1);

			// Progress to the next segment of in-parameter's automation curve and continue.
			t0 = t1;
			x0 = x1;
		}
	}

	// Update stored curve-point values for the next call to process().
	if (curve_changed)
	{
		for (int32 cp = 0; cp < num_curve_points; ++cp)
		{
			if (cp_in[cp].q)
			{
				int32 n = cp_in[cp].q->getPointCount();
				if (n > 0)
				{
					int32 dummy;
					cp_in[cp].q->getPoint(n - 1, dummy, param_value[cp]);
				}
			}
		}
	}

	// Force-output initial values on first call to process(), to help hosts sync up.
	if (data.outputParameterChanges && !initial_values_sent)
	{
		for (ParamID id = 0; id < num_curved_params; ++id)
		{
			int32 dummy;
			if (!param_out[id])
				param_out[id] = data.outputParameterChanges->addParameterData(num_curve_points + 1 + 2 * id, dummy);
			if (param_out[id] && param_out[id]->getPointCount() <= 0)
				param_out[id]->addPoint(0, param_value[num_curve_points + 2 * id + 1], dummy);
		}
		initial_values_sent = true;
	}

	return kResultOk;
}
