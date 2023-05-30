#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include "Curve.h"
#include "CurveController.h"
#include "interpolate.h"

Curve::Curve(void)
{
	LOG("Curve constructor called.\n");
	setControllerClass(FUID(CurveControllerUID));
	processSetup.maxSamplesPerBlock = 8192;
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

tresult PLUGIN_API Curve::setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
	// We just say "ok" to all speaker arrangements because this plug-in doesn't process any audio,
	// but some hosts refuse to load any plug-in for which they can't find a speaker arrangement.
	LOG("Curve::setBusArrangements called and exited.\n");
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

	// Organize all parameter change queues into an array indexed by parameter id.
	IParameterChanges* param_changes[2] = { data.inputParameterChanges, data.outputParameterChanges };
	IParamValueQueue* queue[2][num_params] = {};
	for (int32 io = 0; io < 2; ++io)
	{
		if (param_changes[io])
		{
			int32 n = param_changes[io]->getParameterCount();
			for (int32 i = 0; i < n; ++i)
			{
				IParamValueQueue* q = param_changes[io]->getParameterData(i);
				ParamID id = q->getParameterId();
				if (id < num_params)
					queue[io][id] = q;
			}
		}
	}

	// Update all curve point parameters with the final values in the block.
	bool curve_changed = false;
	for (int32 id = 0; id < num_curve_points; ++id)
	{
		IParamValueQueue* q = queue[0][id];
		if (q)
		{
			int32 n = q->getPointCount();
			if (n > 0)
			{
				int32 dummy;
				q->getPoint(n - 1, dummy, param_value[id]);
				curve_changed = true;
			}
		}
	}

	// Sample-accurate translation of each in-parameter to each out-parameter:
	for (int32 id = 0; id < num_curved_params; ++id)
	{
		int32 dummy;
		IParamValueQueue* in_queue = queue[0][num_curve_points + 2 * id];
		IParamValueQueue* out_queue = queue[1][num_curve_points + 2 * id + 1];
		int32 prev_offset = -1;
		ParamValue prev_x = param_value[num_curve_points + 2 * id];

		// Create out-queue if it doesn't already exist.
		if (!out_queue && (in_queue || curve_changed))
		{
			out_queue = param_changes[1]->addParameterData(num_curve_points + 2 * id + 1, dummy);
			if (!out_queue)
				return kResultFalse;
		}

		// If the curve changed, add a point at sample offset 0 if one doesn't already exist.
		if (curve_changed)
		{
			int32 first_offset = -1;
			if (in_queue && (in_queue->getPointCount() > 0))
			{
				ParamValue x;
				if (in_queue->getPoint(0, first_offset, x) != kResultOk)
					return kResultFalse;
			}
			if (first_offset != 0)
			{
				ParamValue y = curve_y(num_curve_points, param_value, prev_x);
				param_value[num_curve_points + 2 * id + 1] = y;
				out_queue->addPoint(0, y, dummy);
				prev_offset = 0;
			}
		}

		if (in_queue)
		{
			// Process each point in the automation curve of the in-parameter:
			int32 n = in_queue->getPointCount();
			for (int32 i = 0; i < n; ++i)
			{
				// Read the next in-parameter point x:
				ParamValue x;
				int32 offset;
				if (in_queue->getPoint(i, offset, x) != kResultOk)
					return kResultFalse;
				if (x < 0.) x = 0.; else if (x > 1.) x = 1.;
				param_value[num_curve_points + 2 * id] = x;

				// Compute and output the corresponding out-parameter point y:
				ParamValue y = curve_y(num_curve_points, param_value, x);
				param_value[num_curve_points + 2 * id + 1] = y;
				out_queue->addPoint(offset, y, dummy);

				// If this segment of the in-parameter automation curve x passes through any curve points
				// (where the slope of the x-y curve might change), introduce interpolated points into the
				// out-parameter automation curve y.
				if ((x != prev_x) && (offset > prev_offset))
				{
					ParamValue m = (ParamValue)(x - prev_x) / (ParamValue)(offset - prev_offset);
					ParamValue b = x - m * offset;
					ParamValue offset_interval = (ParamValue)1. / (abs(m) * (ParamValue)num_intervals);
					for (ParamValue interpolated_offset = (floor((ParamValue)prev_offset / offset_interval) + 1.) * offset_interval;
						 (int32)round(interpolated_offset) < offset;
						 interpolated_offset += (offset_interval < 1.) ? 1. : offset_interval)
					{
						ParamValue interpolated_x = m * interpolated_offset + b;
						out_queue->addPoint((int32)round(interpolated_offset), curve_y(num_curve_points, param_value, interpolated_x), dummy);
					}
				}

				prev_offset = offset;
				prev_x = x;
			}
		}
	}

	return kResultOk;
}
