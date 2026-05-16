in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform sampler2D PassFeedback0;
uniform int FrameCount;
uniform float Persistence;
uniform float CurrentWeight;

void main()
{
    vec4 current = texture(Source, vTexCoord);
    vec4 previous = texture(PassFeedback0, vTexCoord);
    float keep = FrameCount == 0 ? 0.0 : Persistence;
    vec3 rgb = mix(current.rgb, previous.rgb, keep);
    rgb = mix(rgb, current.rgb, CurrentWeight);
    FragColor = vec4(rgb, current.a);
}
