in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform vec4 SourceSize;
uniform vec4 FinalViewportSize;
uniform float ScanlineIntensity;
uniform float MaskIntensity;
uniform float Brightness;
uniform float Gamma;

void main()
{
    vec4 color = texture(Source, vTexCoord);

    float source_y = vTexCoord.y * SourceSize.y;
    float scan_phase = abs(fract(source_y) - 0.5) * 2.0;
    float scanline = mix(1.0 - ScanlineIntensity, 1.0, scan_phase);

    float triad = mod(gl_FragCoord.x, 3.0);
    vec3 mask = vec3(0.82, 0.82, 0.82);
    if (triad < 1.0)
        mask.r = 1.0;
    else if (triad < 2.0)
        mask.g = 1.0;
    else
        mask.b = 1.0;

    float vignette_x = vTexCoord.x * (1.0 - vTexCoord.x);
    float vignette_y = vTexCoord.y * (1.0 - vTexCoord.y);
    float vignette = smoothstep(0.0, 0.055, vignette_x * vignette_y);

    vec3 rgb = color.rgb * scanline;
    rgb *= mix(vec3(1.0), mask, MaskIntensity);
    rgb *= mix(0.92, 1.0, vignette) * Brightness;
    rgb = pow(max(rgb, vec3(0.0)), vec3(1.0 / max(Gamma, 0.1)));

    FragColor = vec4(rgb, color.a);
}
