local o = 0x20cfed09128
TARGET = {rt.f32(o), rt.f32(o + 4), rt.f32(o + 8), rt.f32(o + 12), rt.f32(o + 16)}
TOL = 0.001
TAG = 'cam2'
MAXNODES = 30000
SPAN = 0x1000
ROOTSPAN = 0x2000
MAXDEPTH = 3
