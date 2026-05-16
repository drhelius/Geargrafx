[Preset]
Name=Soft Two-Pass
Passes=2

[Pass0]
Path=soften_horizontal.glsl
ScaleType=Source
Filter=Linear

[Pass1]
Path=soften_vertical.glsl
ScaleType=Viewport
Filter=Linear

[Parameters]
BlurStrength=0.40

[Parameter.BlurStrength]
Label=Softness
Min=0.0
Max=1.0
Step=0.01
