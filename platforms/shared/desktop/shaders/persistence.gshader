[Preset]
Name=Persistence
Passes=1

[Pass0]
Path=persistence.glsl
ScaleType=Viewport
Filter=Linear
Feedback=true

[Parameters]
Persistence=0.72
CurrentWeight=0.10

[Parameter.Persistence]
Label=Persistence
Min=0.0
Max=0.95
Step=0.01

[Parameter.CurrentWeight]
Label=Current Frame
Min=0.0
Max=0.50
Step=0.01
