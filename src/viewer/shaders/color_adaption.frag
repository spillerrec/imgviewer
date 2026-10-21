#version 450

layout(location = 0) in vec2 v_texcoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 0, std140) uniform Uniforms {
    mat4 projection;
    float isBt2020; // 1.0 if image is already BT.2020, skip sRGB->BT.2020 conversion
} uniforms;


float toSrgb(float v)
{
	v = min(v, 1.0);
	if (v <= 0.0031308)
		return 12.92 * v;
	else
		return 1.055 * pow(v, 1.0/2.4) - 0.055;
}
vec4 toSrgb(vec4 val)
{
	return vec4(
		toSrgb(val.r),
		toSrgb(val.g),
		toSrgb(val.b),
		val.a);
}

float srgb(float v)
{
	if( v <= 0.04045 )
		return v / 12.92;
	else
		return pow( (v+0.055) / 1.055, 2.4 );
}

vec4 fromSrgb(vec4 val)
{
	return vec4(
		srgb(val.r),
		srgb(val.g),
		srgb(val.b),
		val.a);
}

// Direct combined matrix: Linear sRGB -> Linear BT.2020
vec3 linearSrgbToRec2020(vec3 rgb)
{
	mat3 m = mat3(
		0.6274040, 0.0690973, 0.0163916,
		0.3292820, 0.9195480, 0.0880134,
		0.0433136, 0.0113548, 0.8955947
	);
	return m * rgb;
}


vec4 toPQ(vec4 val)
{
	vec4 m1 = vec4( 1305 / 8192.0 );
	vec4 m2 = vec4( 2523 / 32.0 );
	vec4 c1 = vec4(  107 / 128.0 );
	vec4 c2 = vec4( 2413 / 128.0 );
	vec4 c3 = vec4( 2392 / 128.0 );
	
	return pow( (c1 + c2 * pow(val, m1)) / (1 + c3 * pow(val, m1)), m2);
	
	vec4 v = pow( val, 1/m2 );
	return 1 * pow( max( vec4(0.0), v - c1) / ( c2 - c3 * v ), 1/m1 );
}

void main() {
	vec4 tex = texture(texSampler, v_texcoord);

	if (uniforms.isBt2020 > 0.5) {
		// Image is already BT.2020; skip sRGB->BT.2020 conversion.
		outColor = tex;
	} else {
		// Default: sRGB -> linear sRGB -> linear Rec2020
		vec3 linSrgb = fromSrgb(tex).rgb;
		vec3 linRec2020 = linearSrgbToRec2020(linSrgb);
		outColor = toPQ(vec4(linRec2020 / 10000.0 * 100.0, tex.a));
	}

	outColor.a = 1.0;
}