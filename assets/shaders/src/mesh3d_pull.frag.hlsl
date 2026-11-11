cbuffer UBO : register(b0, space3) {
	float3 view_position;
}

struct Input {
	float4 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float3 frag_pos : TEXCOORD3;
	uint flags : TEXCOORD4;
};

Texture2D<float4> tex : register(t0, space2);
SamplerState smp : register(s0, space2);

static const uint TEXTURED = 1u << 0;

float4 main(Input input) : SV_Target0 {
	float3 light_pos = float3(0, 2, -2);
	float3 light_color = float3(1, 1, 1);

	float ambient_strength = 0.1;
	float3 ambient = ambient_strength * light_color;

	float3 norm = normalize(input.normal);
	float3 light_dir = normalize(light_pos - input.frag_pos);
	float diff = max(dot(norm, light_dir), 0);
	float3 diffuse = diff * light_color;

	float specular_strength = 1;
	float3 view_direction = normalize(view_position - input.frag_pos);
	float3 reflect_direction = reflect(-light_dir, norm);
	float spec = pow(max(dot(view_direction, reflect_direction), 0), 32);
	float3 specular = specular_strength * spec * light_color;

	float3 result = ambient + diffuse + specular;

	if (input.flags & TEXTURED) {
		return tex.Sample(smp, input.uv) * input.color * float4(result, 1);
	}

	return input.color * float4(result, 1.0);
}

