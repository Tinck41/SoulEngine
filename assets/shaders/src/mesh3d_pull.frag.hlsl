cbuffer UBO : register(b0, space3) {
	float4x4 view_proj;
	float4 light_position;
	float4 light_color;
	float3 view_position;
}

cbuffer Material : register(b1, space3) {
	float4 ambient_in;
	float4 diffuse_in;
	float4 specular_in;
	float shininess_in;
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
	float3 ambient = light_color.rgb * ambient_in.rgb;

	float3 norm = normalize(input.normal);
	float3 light_dir = normalize(light_position.xyz - input.frag_pos);
	float diff = max(dot(norm, light_dir), 0);
	float3 diffuse = light_color.rgb * (diff * diffuse_in.rgb);

	float3 view_direction = normalize(view_position - input.frag_pos);
	float3 reflect_direction = reflect(-light_dir, norm);
	float spec = pow(max(dot(view_direction, reflect_direction), 0), shininess_in);
	float3 specular = light_color.rgb * (spec * specular_in.rgb);

	float3 result = ambient + diffuse + specular;

	if (input.flags & TEXTURED) {
		return tex.Sample(smp, input.uv) * input.color * float4(result, 1);
	}

	return input.color * float4(result, 1.0);
}

