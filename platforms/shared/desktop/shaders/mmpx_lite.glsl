in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform vec4 SourceSize;
uniform float Strength;
uniform float Similarity;

float similar(vec3 a, vec3 b)
{
    return 1.0 - step(Similarity, max(max(abs(a.r - b.r), abs(a.g - b.g)), abs(a.b - b.b)));
}

vec3 sample_source(vec2 pixel)
{
    vec2 uv = (clamp(pixel, vec2(0.0), SourceSize.xy - vec2(1.0)) + vec2(0.5)) * SourceSize.zw;
    return texture(Source, uv).rgb;
}

void main()
{
    vec2 source_pos = vTexCoord * SourceSize.xy - vec2(0.5);
    vec2 base = floor(source_pos);
    vec2 phase = fract(source_pos);

    vec3 c = sample_source(base);
    vec3 l = sample_source(base + vec2(-1.0, 0.0));
    vec3 r = sample_source(base + vec2(1.0, 0.0));
    vec3 u = sample_source(base + vec2(0.0, -1.0));
    vec3 d = sample_source(base + vec2(0.0, 1.0));

    vec3 candidate = c;

    if (phase.x < 0.5 && phase.y < 0.5)
    {
        float gate = similar(l, u) * (1.0 - similar(l, d)) * (1.0 - similar(u, r));
        candidate = mix(c, (l + u) * 0.5, gate * Strength);
    }
    else if (phase.x >= 0.5 && phase.y < 0.5)
    {
        float gate = similar(r, u) * (1.0 - similar(r, d)) * (1.0 - similar(u, l));
        candidate = mix(c, (r + u) * 0.5, gate * Strength);
    }
    else if (phase.x < 0.5 && phase.y >= 0.5)
    {
        float gate = similar(l, d) * (1.0 - similar(l, u)) * (1.0 - similar(d, r));
        candidate = mix(c, (l + d) * 0.5, gate * Strength);
    }
    else
    {
        float gate = similar(r, d) * (1.0 - similar(r, u)) * (1.0 - similar(d, l));
        candidate = mix(c, (r + d) * 0.5, gate * Strength);
    }

    FragColor = vec4(candidate, 1.0);
}
