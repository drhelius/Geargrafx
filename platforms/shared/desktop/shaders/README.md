# Geargrafx Shaders

Geargrafx desktop shader presets use `.gshader` files. A preset is an INI-style file that describes one or more GLSL fragment shader passes. Presets placed in this `shaders` directory, up to 64, are discovered once when the desktop app starts and appear in the `Video > Shader` menu after `Pixel Perfect`. Shader presets are not applied while the debugger is enabled.

GLSL files are normal fragment shaders. Geargrafx supplies a fullscreen vertex shader and prepends the correct GLSL version, so shader files should not include a `#version` line.

## Minimal Preset

```ini
[Preset]
Name=Scanlines + Bilinear
Passes=1

[Pass0]
Path=scanlines.glsl
ScaleType=Viewport
Filter=Linear

[Parameters]
ScanlineIntensity=0.10

[Parameter.ScanlineIntensity]
Label=Intensity
Min=0.0
Max=1.0
Step=0.01
```

## Preset Sections

`[Preset]`

- `Name`: display name shown in the menu. If omitted, the filename without its extension is used.
- `Group`: optional submenu name. Presets with the same group are listed together in that submenu.
- `Passes`: number of passes, from 1 to 32.
- `Variants`: optional number of `[VariantN]` sections, up to 16.
- `DefaultVariant`: variant applied when the preset is loaded for the first time. Defaults to `0`.

`[PassN]`

- `Path`: GLSL fragment shader path. Relative paths are resolved from the `.gshader` file directory first, then from the `shaders` resource directory.
- `Alias`: optional name for the pass output. Later passes can sample it with that name. See Shader Inputs.
- `Filter`: `Nearest` by default. Use `Linear` or `Bilinear` for bilinear sampling on that pass input/output texture.
- `Feedback`: `true` when the pass needs the previous frame through `PassFeedback0`.
- `History`: `true` when the pass needs previous inputs to the same pass through `SourceHistory0` through `SourceHistory7`.
- `ScaleType`: sets both axes. Defaults to `Viewport`. Values are:
  - `Source` (or `Original`): relative to the original emulator frame, at native core resolution.
  - `Previous`: relative to the previous pass output. This is RetroArch's `source` scale type.
  - `Viewport` (or `FinalViewport`): relative to the final display viewport.
  - `Absolute`: fixed size from `AbsoluteWidth` and `AbsoluteHeight`.
- `ScaleTypeX`, `ScaleTypeY`: optional per-axis scale type overrides.
- `Scale`: output scale for both axes. Defaults to `1.0`.
- `ScaleX`, `ScaleY`: optional per-axis scale overrides.
- `AbsoluteWidth`, `AbsoluteHeight`: used with `ScaleType=Absolute`.
- `FloatFramebuffer`: `true` when the pass output needs signed or high-range intermediate values. Uses a floating-point render target.

The final pass is what Geargrafx displays. Non-final passes render to intermediate textures. Only the final pass needs
`Viewport`; passes that work at core resolution should use `Source` or `Previous` to avoid rendering at display size.

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

`Label` defaults to the parameter name. `Min`, `Max` and `Step` default to `0.0`, `1.0` and `0.01`, and the `[Parameters]`
value is clamped to the `Min` to `Max` range.

A parameter with `Min=0.0`, `Max=1.0` and a `Step` of `1.0` or higher is displayed as a checkbox. Other parameters with
a `Step` of `1.0` or higher are displayed as integer sliders.

Optional `Group` puts the parameter in a collapsible section of the Shader Parameters window, which opens from
`Video > Shader > Parameters...`:

```ini
[Parameter.Amount]
Label=Amount
Group=Glow
```

A preset can expose up to 256 parameters.

`[VariantN]`

A variant is a named set of parameter values. Selecting it resets every parameter to its `[Parameters]` value and then
applies the variant values. Variants are listed as submenus of the preset in `Video > Shader` and in the Shader
Parameters window. `Restore Defaults` restores the selected variant.

```ini
[Preset]
Name=My CRT
Variants=2

[Variant0]
Name=Slot Mask

[Variant1]
Name=Aperture Grille
MaskType=2.0
```

## Shader Inputs

Available samplers:

- `uniform sampler2D Source`: current pass input.
- `uniform sampler2D Original`: original emulator frame.
- `uniform sampler2D PassFeedback0`: previous final output, for feedback presets.
- `uniform sampler2D SourceHistory0` through `SourceHistory7`: previous inputs to the same pass, newest first, for history presets.
- `uniform sampler2D PassOutputN`: output of any previous pass, where `PassOutput0` is pass 0 output, `PassOutput1` is pass 1 output, and so on.
- `uniform sampler2D <Alias>`: output of a previous pass with that `Alias`.
- `uniform sampler2D <Alias>Feedback`: previous frame output of the non-final pass with that `Alias`. A pass can sample its own feedback.
- `uniform sampler2D LineClock`: one texel per `Original` row, with the VCE dot clock of that row in MHz in the red
  channel: `5.369`, `7.159` or `10.739`. Sample it with `texture(LineClock, vec2(0.5, y)).r`. `PixelClock / LineClock`
  is how many `Original` texels one VCE pixel covers on that row, which is more than `1` on frames that mix dot clocks.

A pass can use up to 16 samplers.

Available size uniforms are `vec4(width, height, 1.0 / width, 1.0 / height)`:

- `SourceSize`: current pass input size.
- `OriginalSize`: original emulator frame size.
- `OutputSize`: current pass output size.
- `FinalViewportSize`: final display viewport size.
- `PassFeedback0Size`: feedback texture size.
- `SourceHistorySize`: source history texture size.
- `PassOutputSizeN` (or `PassOutputNSize`): previous pass output texture sizes.
- `<Alias>Size`: output size of the pass with that `Alias`.

Other uniforms:

- `FrameCount`: increments once per new emulated frame. It starts at `0` when a preset is loaded.
- `FrameDirection`: `1` normally, `-1` while rewinding.
- `SourceHistoryCount`: number of valid source history textures, from `0` to `8`.
- `OriginalAspect`: width / height of `Original` in emulated pixels. Frames that mix dot clocks count their width in
  7.16 MHz pixels, a third of their texel width. LaserActive frames report `4:3`.
- `FramebufferScale`: display pixel density, as the rounded ratio between framebuffer pixels and window points, at
  least `1.0`. Use it to keep masks and grids at a fixed physical size on high-density displays.
- `BackgroundColor`: configured normal desktop background color as `vec4(r, g, b, 1)`.

PC Engine video uniforms, updated every frame:

- `float OriginalFPS`: emulated frame rate in Hz, about `59.83` in 262-line mode and `59.60` in 263-line mode.
- `float PixelClock`: rate of the `Original` texels in MHz: `5.369`, `7.159` or `10.739`, following the VCE dot clock.
  Frames that mix dot clocks, and LaserActive frames, are expanded to one texel per master clock and report `21.477`.
- `float LineRate`: horizontal scan rate in kHz, `15.734`.
- `float SourceLines`: number of scanlines in `Original`. It always equals `OriginalSize.y` here. Scanline shaders use
  it instead of `OriginalSize.y` so they also work in emulators that double lines.
- `vec4 ChromaPhase`: NTSC color subcarrier timing, for composite shaders. `x` is the subcarrier phase at the left edge
  of `Original` row `0`, in cycles from `0.0` to `1.0`. `y` is cycles per `Original` texel: `4/6`, `3/6` or `2/6`
  for the three dot clocks, and `1/6` on expanded frames. `z` is cycles per row, `227.5`, so the phase flips every
  line. `w` is the fractional cycles added every frame: `0.5` in 263-line mode, where the phase also flips every
  frame, and `0.0` in 262-line mode, where the artifact pattern stays still. The phase at `Original` position
  `(px, row)` is `fract(x + px * y + row * z)`, and `x` already includes the frame-to-frame change. The origin follows
  overscan, auto crop and the first visible scanline. LaserActive frames report `vec4(0.0)`.
- `vec4 ActiveRegion`: picture area inside `Original` as `vec4(x, y, width, height)` in texels. With overscan enabled
  it is the standard picture width between the extra border columns. Otherwise it is the whole frame.
- `vec4 BorderColor`: VCE color `0x100` as `vec4(r, g, b, 1)`, which is what the VDC outputs outside its display area.
  `vec4(0.0)` on LaserActive frames.
- `int SystemType`: `0` PC Engine or TurboGrafx-16, `1` SuperGrafx, `2` LaserActive.
- `int VideoMode`: `0` 5.37 MHz, `1` 7.16 MHz, `2` 10.74 MHz, `3` several dot clocks in the same frame.
- `int CoreFlags`: bit flags, `0` when none is set.
  - `2`: `Video > Low Pass Filter` is enabled for the current dot clock, so a composite shader can skip its own luma
    filter.
  - `8`: the game turned on the VCE black and white mode (colorburst off) and
    `Video > Color Palette > Black & White Mode` is enabled, so a composite shader can drop all chroma like a TV color
    killer. The frame already uses the B&W palette colors.

`FrameCount`, `SourceHistory`, `PassFeedback0`, `<Alias>Feedback` and the frame part of `ChromaPhase` only advance
when the emulator produces a new frame, or when rewinding steps back one. Redraws while paused, after a window resize
or after a parameter change render the same frame again with the same state.

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
Name=Feedback Blend
Passes=1

[Pass0]
Path=feedback_blend.glsl
ScaleType=Viewport
Filter=Nearest
Feedback=true

[Parameters]
FeedbackAmount=0.5
```

A feedback shader can sample the previous final frame. Save this one as `feedback_blend.glsl` next to the preset:

```glsl
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform sampler2D PassFeedback0;
uniform float FeedbackAmount;

void main()
{
    vec3 current = texture(Source, vTexCoord).rgb;
    vec3 previous = texture(PassFeedback0, vTexCoord).rgb;
    FragColor = vec4(mix(current, previous, FeedbackAmount), 1.0);
}
```
