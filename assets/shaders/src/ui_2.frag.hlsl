struct Input {
	float4 color         : TEXCOORD0;
	float2 uv            : TEXCOORD1;
	uint   flags         : TEXCOORD2;
	float2 size          : TEXCOORD3;
	float  border_radius : TEXCOORD4;
	float4 border_color  : TEXCOORD5;
	float  border_width  : TEXCOORD6;
	float2 local_pos     : TEXCOORD7;
};

Texture2D<float4> tex : register(t0, space2);
SamplerState smp : register(s0, space2);

static const uint TEXTURED = 1u << 0;

float rounded_rect_sdf(float2 position, float2 half_size, float radius) {
	float2 q = abs(position) - half_size + radius;
	return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

float4 main(Input input) : SV_Target0 {
	float4 base_color;

	if (input.flags & TEXTURED) {
		base_color = tex.Sample(smp, input.uv) * input.color;
	} else {
		base_color = input.color;
	}

	float2 half_size = input.size * 0.5;
	float2 position  = input.local_pos;

	float dist = rounded_rect_sdf(position, half_size, input.border_radius);

	float aa = fwidth(dist) * 0.5;

	float outer = 1.0 - smoothstep(-aa, aa, dist);
	float inner = 1.0 - smoothstep(-aa, aa, dist + input.border_width);

	float border_factor = outer - inner;

	float3 rgb = lerp(base_color.rgb, input.border_color.rgb, border_factor);
	float a    = base_color.a * outer;

	return float4(rgb, a);
}

