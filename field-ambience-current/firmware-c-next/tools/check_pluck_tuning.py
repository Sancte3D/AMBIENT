#!/usr/bin/env python3
"""Rendered fundamental regression (requires numpy/scipy and a host C compiler).
Checks actual PCM using a windowed spectral peak, not the delay formula.
Validated range: 60–1760 Hz. Not a timbre or physical CPU qualification.
"""
import ctypes as C
import numpy as np
from scipy.optimize import minimize_scalar
SR=44100
P=C.POINTER(C.c_float)
def engine(path):
 e=C.CDLL(path);e.pluck_note.argtypes=[C.c_float,C.c_float];e.pluck_set_damp.argtypes=[C.c_float];e.pluck_render_mix.argtypes=[P,P,P,P,C.c_int];return e
def render(e,f,d,change=None):
 e.pluck_init();e.pluck_set_damp(d);e.pluck_note(f,.5)
 a=np.zeros((4,44100),np.float32)
 for start in range(0,44100,100):
  if change is not None and start==4410:e.pluck_set_damp(change)
  args=[row[start:].ctypes.data_as(P) for row in a]
  e.pluck_render_mix(*args,100)
 return a[:2].T.copy()
def pitch(x,f):
 x=x[4410:17640,0].astype(float);x-=x.mean();x*=np.hanning(len(x));t=np.arange(len(x))/SR
 def power(hz):return -abs(np.dot(x,np.exp(-2j*np.pi*hz*t)))**2
 result=minimize_scalar(power,bounds=(f*.97,f*1.03),method='bounded',options={'xatol':1e-7})
 return 1200*np.log2(result.x/f)
if __name__ == '__main__':
 import pathlib, subprocess, tempfile
 root=pathlib.Path(__file__).resolve().parents[1]
 with tempfile.TemporaryDirectory() as tmp:
  lib=str(pathlib.Path(tmp)/'pluck.so')
  subprocess.run(['cc','-shared','-fPIC','-O2','-I'+str(root/'include'),str(root/'src/pluck.c'),str(root/'src/shape.c'),'-lm','-o',lib],check=True)
  e=engine(lib)
  worst=0.
  for d in [0,.2,.42,.6,.9]:
   for f in [60,110,220,440,880,1320,1760]:
    x=render(e,f,d)
    assert np.isfinite(x).all() and np.max(np.abs(x))<1
    cents=float(pitch(x,f));worst=max(worst,abs(cents))
    assert abs(cents)<.3,(f,d,cents)
  # Both directions, after the 80 ms smoother has settled. No silence/NaNs.
  for initial,target in [(0,.9),(.9,0)]:
   for f in [220,880,1760]:
    x=render(e,f,initial,target)
    cents=float(pitch(x[22050:],f))
    assert np.isfinite(x).all() and abs(cents)<.3,(f,initial,target,cents)
  print(f'PASS: 35 static + 6 live damping pitch cases; worst static {worst:.4f} cents')
