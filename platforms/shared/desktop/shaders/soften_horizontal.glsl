in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform vec4 SourceSize;
uniform float BlurStrength;

void main()
{
    vec2 step_uv = vec2(SourceSize.z, 0.0);
    vec4 center = texture(Source, vTexCoord);
    vec4 blur = texture(Source, vTexCoord - step_uv) * 0.25 + center * 0.5 + texture(Source, vTexCoord + step_uv) * 0.25;
    FragColor = mix(center, blur, BlurStrength);
}
