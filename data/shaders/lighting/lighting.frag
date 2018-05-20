#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec3 v_Normal_W;
in mediump vec3 v_ViewDir_W;

uniform mediump vec3 iu_Ka;
uniform mediump vec3 iu_Kd;
uniform mediump vec3 iu_Ks;
uniform mediump float iu_Shininess;
uniform mediump vec3 iu_LightIntensity;
uniform mediump vec3 iu_LightDirection;

void main()
{
	mediump vec3 lightDir = normalize(iu_LightDirection);
	mediump vec3 halfVec = normalize(lightDir + v_ViewDir_W);
	mediump float NoL = max(dot(v_Normal_W, lightDir), 0.0f);
	mediump float NoH = max(dot(v_Normal_W, halfVec), 0.0f);
	mediump vec3 R = normalize(-iu_LightDirection - 2.0f * dot(v_Normal_W, -iu_LightDirection) * v_Normal_W);
	mediump float RoV = max(dot(R, v_ViewDir_W), 0.0f);
	mediump vec3 ambientTerm = iu_Ka * iu_LightIntensity.x;
	mediump vec3 diffuseTerm = iu_Kd * iu_LightIntensity.y * NoL;
	mediump vec3 specularTerm_Phong = iu_Ks * pow(RoV, iu_Shininess);
	mediump vec3 specularTerm_BlinnPhong = iu_Ks * iu_LightIntensity.z * pow(NoH, iu_Shininess);
	mediump vec3 outColor = ambientTerm + diffuseTerm + specularTerm_BlinnPhong;
	o_Color = vec4(vec3(outColor), 1.0f);
}