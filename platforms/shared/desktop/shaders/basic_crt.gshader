[Preset]
Name=Basic CRT
Passes=1

[Pass0]
Path=crt_basic.glsl
ScaleType=Viewport
Filter=Linear

[Parameters]
ScanlineIntensity=0.30
MaskIntensity=0.28
Brightness=1.12
Gamma=1.00

[Parameter.ScanlineIntensity]
Label=Scanlines
Min=0.0
Max=0.75
Step=0.01

[Parameter.MaskIntensity]
Label=Mask
Min=0.0
Max=0.70
Step=0.01

[Parameter.Brightness]
Label=Brightness
Min=0.70
Max=1.50
Step=0.01

[Parameter.Gamma]
Label=Gamma
Min=0.60
Max=1.80
Step=0.01
