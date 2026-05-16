# Geargrafx Shaders

Geargrafx desktop shader presets use `.gshader` files. A preset is an INI-style file that describes one or more GLSL fragment shader passes. Presets placed in this `shaders` directory are discovered automatically by the desktop app under `Video > Shader > Presets`.

GLSL files are normal fragment shaders. Geargrafx supplies a fullscreen vertex shader and prepends the correct GLSL version, so shader files should not include a `#version` line.

## Minimal Preset

```ini
[Preset]
Name=Crisp
Passes=1

[Pass0]
Path=crisp.glsl
ScaleType=Viewport
Filter=Nearest

[Parameters]
Sharpness=0.35

[Parameter.Sharpness]
Label=Sharpness
Min=0.0
Max=1.0
Step=0.01
```

## Preset Sections

`[Preset]`

- `Name`: display name shown in the menu. If omitted, the filename is used.
- `Passes`: number of passes, from 1 to 8.

`[PassN]`

- `Path`: GLSL fragment shader path. Relative paths are resolved from the `.gshader` file directory first, then from the `shaders` resource directory.
- `Filter`: `Nearest` by default. Use `Linear` or `Bilinear` for bilinear sampling on that pass input/output texture.
- `Feedback`: `true` when the pass needs the previous frame through `PassFeedback0`.
- `ScaleType`: sets both axes. Values are `Viewport`, `Source`, `Previous`, or `Absolute`.
- `ScaleTypeX`, `ScaleTypeY`: optional per-axis scale type overrides.
- `Scale`: output scale for both axes. Defaults to `1.0`.
- `ScaleX`, `ScaleY`: optional per-axis scale overrides.
- `AbsoluteWidth`, `AbsoluteHeight`: used with `ScaleType=Absolute`.

The final pass is what Geargrafx displays. Non-final passes render to intermediate textures.

`[Parameters]`

Each entry creates a float uniform with the same name:

```ini
[Parameters]
Amount=0.5
```

Optional metadata for sliders:

```ini
[Parameter.Amount]
Label=Amount
Min=0.0
Max=1.0
Step=0.01
```

## Shader Inputs

Available samplers:

- `uniform sampler2D Source`: current pass input.
- `uniform sampler2D Original`: original emulator frame.
- `uniform sampler2D PassFeedback0`: previous final output, for feedback presets.

Available size uniforms are `vec4(width, height, 1.0 / width, 1.0 / height)`:

- `SourceSize`: current pass input size.
- `OriginalSize`: original emulator frame size.
- `OutputSize`: current pass output size.
- `FinalViewportSize`: final display viewport size.
- `PassFeedback0Size`: feedback texture size.

Other uniforms:

- `FrameCount`: increments once per rendered preset frame.
- `FrameDirection`: currently `1`.
- `OriginalAspect`: intended emulator image aspect ratio.

Any parameter declared in `[Parameters]` is also available as a `float` uniform.

## Minimal GLSL Shader

```glsl
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;

void main()
{
    FragColor = texture(Source, vTexCoord);
}
```

## Feedback Example

```ini
[Preset]
Name=Persistence
Passes=1

[Pass0]
Path=persistence.glsl
ScaleType=Viewport
Filter=Linear
Feedback=true
```

A feedback shader can sample the previous final frame:

```glsl
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform sampler2D PassFeedback0;
uniform float Persistence;

void main()
{
    vec4 current = texture(Source, vTexCoord);
    vec4 previous = texture(PassFeedback0, vTexCoord);
    FragColor = mix(current, previous, Persistence);
}
```
