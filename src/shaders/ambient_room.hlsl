#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos      : POSITION;
	float4 texCoord : TEXCOORD0;
	float4 diffuse  : COLOR0;
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	
	float4 rBasisRot = uBasis[0];
	float4 rBasisPos = uBasis[1];

	Out.texCoord = In.aTexCoord * (1.0 / 32767.0);

	float3 coord;
	
	coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);

	Out.diffuse = float4(In.aColor.rgb * (uMaterial.x * 1.8), 1.0);
	Out.diffuse.xyz *= In.aLight.rgb;

	Out.diffuse *= uMaterial.w;
	
	Out.pos = mul(uViewProj, float4(coord, rBasisPos.w));
	
	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float4 color = tex2D(sDiffuse, In.texCoord.xy);

	#ifdef ALPHA_TEST
        clip(color.w - ALPHA_REF);
	#endif

	color *= In.diffuse;

	return color;
}

#endif
