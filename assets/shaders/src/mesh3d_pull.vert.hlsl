cbuffer UBO : register(b0, space1) {
	float4x4 mvp;
}

struct InstanceData {
	float4x4 model;
	uint flags;
	uint3 padding;
};

struct Input {
	float3 position : TEXCOORD0;
	float4 color : TEXCOORD1;
	float2 uv : TEXCOORD2;
	float3 normal : TEXCOORD3;
};

struct Output {
	float4 position : SV_Position;
	float4 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float3 frag_pos : TEXCOORD3;
	uint flags : TEXCOORD4;
};

StructuredBuffer<InstanceData> DataBuffer : register(t0, space0);

Output main(Input input, uint id : SV_InstanceID) {
	Output output;

	InstanceData instance = DataBuffer[id];

	output.position = mul(mvp, mul(instance.model, float4(input.position, 1)));
	output.color = input.color;
	output.uv = input.uv;
	output.normal = mul((float3x3)instance.model, input.normal);
	output.frag_pos = mul(instance.model, float4(input.position, 1)).xyz;
	output.flags = instance.flags;

	return output;
}

