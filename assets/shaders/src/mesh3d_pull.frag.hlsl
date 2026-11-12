cbuffer UBO : register(b0, space3) {
	float4x4 view_proj;
	float3 view_position;
}

cbuffer Light : register(b1, space3) {
	float4 light_position;

	float4 light_ambient;
	float4 light_diffuse;
	float4 light_specular;
}

struct Material {
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float shininess;
};

struct Input {
	float4 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float3 frag_pos : TEXCOORD3;
	uint flags : TEXCOORD4;
	uint id : TEXCOORD5;
};

Texture2D<float4> tex : register(t0, space2);
StructuredBuffer<Material> MaterialBuffer : register(t1, space2);
SamplerState smp : register(s0, space2);

static const uint TEXTURED = 1u << 0;

float4 main(Input input) : SV_Target0 {
	Material material = MaterialBuffer[input.id];

	float3 ambient = light_ambient.rgb * material.ambient.rgb;

	float3 norm = normalize(input.normal);
	float3 light_dir = normalize(light_position.xyz - input.frag_pos);
	float diff = max(dot(norm, light_dir), 0);
	float3 diffuse = light_diffuse.rgb * (diff * material.diffuse.rgb);

	float3 view_direction = normalize(view_position - input.frag_pos);
	float3 reflect_direction = reflect(-light_dir, norm);
	float spec = pow(max(dot(view_direction, reflect_direction), 0), material.shininess);
	float3 specular = light_specular.rgb * (spec * material.specular.rgb);

	float3 result = ambient + diffuse + specular;

	if (input.flags & TEXTURED) {
		return tex.Sample(smp, input.uv) * input.color * float4(result, 1);
	}

	return input.color * float4(result, 1.0);
}

