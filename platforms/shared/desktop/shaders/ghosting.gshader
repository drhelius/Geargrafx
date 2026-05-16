[Preset]
Name=Screen Ghosting
Passes=1

[Pass0]
Path=legacy_ghosting.glsl
ScaleType=Viewport
Filter=Nearest
Feedback=true

[Parameters]
MixAlpha=0.40
RoundColor=1.0

[Parameter.MixAlpha]
Label=Current Frame
Min=0.15
Max=0.65
Step=0.01

[Parameter.RoundColor]
Label=Round Color
Min=0.90
Max=1.0
Step=0.01
