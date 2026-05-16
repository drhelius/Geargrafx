[Preset]
Name=MMPX Lite
Passes=2

[Pass0]
Path=mmpx_lite.glsl
ScaleType=Source
Scale=4.0
Filter=Nearest

[Pass1]
Path=present.glsl
ScaleType=Viewport
Filter=Nearest

[Parameters]
Strength=0.82
Similarity=0.18

[Parameter.Strength]
Label=Strength
Min=0.0
Max=1.0
Step=0.01

[Parameter.Similarity]
Label=Similarity
Min=0.02
Max=0.40
Step=0.01
