cbuffer UBO : register(b0, space1) {
	float4x4 view_proj;
}

struct InstanceData {
	float3 position;
	float  rotation;
	float2 scale;
	float2 padding_1;
	float2 tex_pos;
	float2 tex_size;
	float4 color;
	float4 border_color;
	float  border_width;
	float  border_radius;
	float2 padding_2;
	uint   flags;
};

static const uint indices[6] = {0, 1, 2, 3, 2, 1};
static const float2 vertices[4] = {
	{0.0f, 0.0f},
	{1.0f, 0.0f},
	{0.0f, 1.0f},
	{1.0f, 1.0f}
};

struct Output {
	float4 position      : SV_Position;
	float4 color         : TEXCOORD0;
	float2 uv            : TEXCOORD1;
	uint   flags         : TEXCOORD2;
	float2 size          : TEXCOORD3;
	float  border_radius : TEXCOORD4;
	float4 border_color  : TEXCOORD5;
	float  border_width  : TEXCOORD6;
	float2 local_pos     : TEXCOORD7;
};

StructuredBuffer<InstanceData> DataBuffer : register(t0, space0);

Output main(uint id : SV_VertexID) {
	uint index = id / 6;
	uint vertex_id = indices[id % 6];
	InstanceData data = DataBuffer[index];

	float2 texture_coord[4] = {
		{ data.tex_pos.x                  , data.tex_pos.y                   },
		{ data.tex_pos.x + data.tex_size.x, data.tex_pos.y                   },
		{ data.tex_pos.x                  , data.tex_pos.y + data.tex_size.y },
		{ data.tex_pos.x + data.tex_size.x, data.tex_pos.y + data.tex_size.y },
	};

	float c = cos(data.rotation);
	float s = sin(data.rotation);
	float2x2 rotation = {c, s, -s, c};

	float2 unit_pos = vertices[vertex_id];
	float2 coord = mul(unit_pos * data.scale, rotation);
	float3 coord_with_depth = float3(coord + data.position.xy, data.position.z);

	Output output;

	output.position = mul(view_proj, float4(coord_with_depth, 1.0f));
	output.color    = data.color;
	output.uv       = texture_coord[vertex_id];
	output.flags    = data.flags;
	output.size     = data.scale;

	output.border_radius = data.border_radius;
	output.border_color  = data.border_color;
	output.border_width  = data.border_width;

	output.local_pos = (unit_pos - 0.5f) * data.scale;

	return output;
}

