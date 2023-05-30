#include "interpolate.h"

constexpr ParamValue small_double = 0.00001;

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
