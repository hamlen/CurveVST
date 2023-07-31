#include "interpolate.h"

ParamValue interpolate(int32 x0, ParamValue y0, int32 x1, ParamValue y1, int32 x)
{
	if (x0 == x1)
	{
		return (x >= x1) ? y1 : y0;
	}
	else
	{
		ParamValue y = y0 + (y1 - y0) * ((ParamValue)(x - x0) / (ParamValue)(x1 - x0));
		if (y < 0.)
			return 0.;
		else if (y > 1.)
			return 1.;
		else
			return y;
	}
}

ParamValue curve_y(int32 n, const ParamValue* curve, ParamValue x)
{
	if (x < 0.) x = 0.; else if (x > 1.) x = 1.;
	ParamValue cp = x * (ParamValue)(n - 1);
	int32 cp1 = (int32)(cp + 0.5); // round to nearest int
	ParamValue y;
	if (abs(cp - (ParamValue)cp1) <= small_double)
	{
		y = curve[cp1];
	}
	else
	{
		cp1 = (int32)cp;
		int32 cp2 = cp1 + 1;
		if (cp2 >= n) cp2 = n - 1;
		y = curve[cp1] + (curve[cp2] - curve[cp1]) * (cp - (ParamValue)cp1);
	}
	if (y < 0.) y = 0.; else if (y > 1.) y = 1.;
	return y;
}
