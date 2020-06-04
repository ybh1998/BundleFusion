# HDRBundleFusion

![HDRBundleFusion](img/test.gif)


Fork from [BundleFusion](https://github.com/wuyingnan/BundleFusion/). 

The license is the same with original BundleFusion. (Please see [License.txt](LICENSE.txt)).

## Installation
The modified code was tested under VS2019 with CUDA10.2 and a GTX 1660.

Requirements:
- Windows SDK (Install it from your Visual Studio)
- NVIDIA CUDA 10.2
- eigen, a git submodule in external/eigen
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
Put *.oni *_curve.txt and *_frame_exp.txt files in BundleFusion/data/.

## End
