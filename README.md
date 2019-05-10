# BundleFusion

![BundleFusion](img/test.gif)

This is a modern port for [BundleFusion](http://graphics.stanford.edu/projects/bundlefusion/). Since DirectX SDK has been deprecated and Nvidia changed some CUDA apis, original BundleFusion code would not run smoothly on latest platforms. For example, code compiled in VS2017 does not run in Release mode. Also, the original code hangs on GPU synchronize function if you use new volta/turing GPUs.

The license is the same with original BundleFusion. (Please see [License.txt](License.txt)).

## Installation
The modified code was tested under VS2017 with CUDA10.1 and a RTX2060.

Requirements:
- Windows SDK (Install it from your Visual Studio)
- DirectX SDK **IS NOT NEEDED**
- NVIDIA CUDA 10.1
- research library mLib, a git submodule in external/mLib
- mLib external libraries can be downloaded [here](http://kaldir.vc.in.tum.de/mLib/mLibExternal.zip)

Default file tree:
```
BundleFusion-master
├──BundleFusion
│  │──data
│  │──external
│  │──FriedLiver
│  └──img
└──mlibExternal
   │──include
   │──libsLinux
   │──libsOSX
   └──libsWindows
```
Put sequence.sens in BundleFusion-master/BundleFusion/data/.

## End
You may star me if you find it useful. :)
