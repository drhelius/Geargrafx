in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform vec4 SourceSize;
uniform float Sharpness;

void main()
{
    vec2 texel = SourceSize.zw;
    vec4 center = texture(Source, vTexCoord);
    vec4 left = texture(Source, vTexCoord - vec2(texel.x, 0.0));
    vec4 right = texture(Source, vTexCoord + vec2(texel.x, 0.0));
    vec4 up = texture(Source, vTexCoord - vec2(0.0, texel.y));
    vec4 down = texture(Source, vTexCoord + vec2(0.0, texel.y));
    vec4 blur = (left + right + up + down) * 0.25;
    FragColor = mix(center, center + (center - blur) * 0.65, Sharpness);
}
