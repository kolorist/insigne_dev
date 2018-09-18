#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec3 v_Normal_W;

uniform mediump vec3 coeff1;
uniform mediump vec3 coeff2;
uniform mediump vec3 coeff3;
uniform mediump vec3 coeff4;
uniform mediump vec3 coeff5;
uniform mediump vec3 coeff6;
uniform mediump vec3 coeff7;
uniform mediump vec3 coeff8;
uniform mediump float coeff9x;
uniform mediump float coeff9y;
uniform mediump float coeff9z;

const mediump float c1 = 0.429043f;
const mediump float c2 = 0.511664f;
const mediump float c3 = 0.743125f;
const mediump float c4 = 0.886227f;
const mediump float c5 = 0.247708f;

mediump vec3 evalSH(in mediump vec3 i_normal)
{
	mediump vec3 coeff9 = vec3(coeff9x, coeff9y, coeff9z);
	return
		// constant term, lowest frequency //////
		c4 * coeff1 +

		// axis aligned terms ///////////////////
		2.0f * c2 * coeff2 * i_normal.y +
		2.0f * c2 * coeff3 * i_normal.z +
		2.0f * c2 * coeff4 * i_normal.x +

		// band 2 terms /////////////////////////
		2.0f * c1 * coeff5 * i_normal.x * i_normal.y +
		2.0f * c1 * coeff6 * i_normal.y * i_normal.z +
		c3 * coeff7 * i_normal.z * i_normal.z - c5 * coeff7 +
		2.0f * c1 * coeff8 * i_normal.x * i_normal.z +
		c1 * coeff9 * (i_normal.x * i_normal.x - i_normal.y * i_normal.y);
}

void main()
{
	mediump vec3 c = evalSH(v_Normal_W);
	o_Color = vec4(c, 1.0f);
}
