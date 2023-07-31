# CurveVST

*Curve* is a VST3 that transforms incoming automation parameter values according to a user-defined curve. You define the curve by setting automation parameters **Curve0** through **Curve10**, each of which specifies the height of the curve at points 0.0, 0.1, ..., 1.0, respectively. Thereafter, changing parameter **In1** to any value *x* causes the VST to change its **Out1** parameter to be the curve's value at *x*. For example, if you change **In1** to be 0.3 then the VST changes **Out1** to be the value of **Curve3** (which defines the curve height at 0.3).

Incoming *x* values that are not exactly at a curve point are linearly interpolated. So if you send value 0.35 to **In1**, then **Out1** will change to the average of the values of **Curve3** and **Curve4**.

By default, *Curve* exports 20 in-out parameter pairs, all of which are curved according to the same function *f* defined by the curve points **Curve0** through **Curve10**.
The curving is sample-accurate for all changes to sent to the **In** and **Curve** parameters.

### Change History

* v1.0 - initial release
* v1.1 - support sample-accurate automation of curve function points
