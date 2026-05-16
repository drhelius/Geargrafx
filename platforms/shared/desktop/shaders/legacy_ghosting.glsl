in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform sampler2D PassFeedback0;
uniform float MixAlpha;
uniform float RoundColor;

void main()
{
    vec4 current = texture(Source, vTexCoord);
    vec4 previous = texture(PassFeedback0, vTexCoord);
    FragColor = mix(previous, vec4(current.rgb * RoundColor, 1.0), MixAlpha);
}
