#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos		: POSITION;
	float4 texCoord	: TEXCOORD0;
	float4 diffuse	: TEXCOORD1;
};

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	
	float4 rBasisRot = uBasis[0];
	float4 rBasisPos = uBasis[0 + 1];

	Out.texCoord = In.aTexCoord * (1.0 / 32767.0);

	float3 coord;
	
	if (TYPE_SPRITE) {
		coord = mulBasis(rBasisRot, rBasisPos.xyz + In.aCoord.xyz, float3(In.aTexCoord.z, In.aTexCoord.w, 0.0));
	} else {
		coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);
	}

	Out.diffuse = float4(RGB(In.aColor) * (uMaterial.x * 1.8), 1.0);
	Out.diffuse.xyz *= RGB(In.aLight);

	Out.diffuse *= uMaterial.w;
	
	if (TYPE_SPRITE) {
		Out.diffuse *= RGBA(In.aLight).a;
	}

	Out.pos = mul(uViewProj, float4(coord, rBasisPos.w));
	
	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float4 color = tex2D(sDiffuse, In.texCoord.xy).bgra;

	if (ALPHA_TEST) {
		if (color.w <= 0.5)
			discard;
	}

	color *= In.diffuse;

	return color;
}
#endif