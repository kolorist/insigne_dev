## Build instructions
### Requirements
* CMake
* Clang
* Ninja build tool

### Build Steps
1. `_gen_prj_clang.bat`
2. `_build_prj_clang.bat`
3. run `clang64\demo.exe`, set `data\` as the application's starting directory

## Available Demos / Tests
### Playground
Contains various testing code
* `shader vault`: demo code for some shader optimization techniques
* `geometry vault`: demo code for geometry optimization using `meshoptimizer` library

### Performance
* `empty`: render nothing, use for comparison or testing the hardware counters reading
* `triangle`: render a triangle with various floating point precision
* `scene loader`: render sponza scene with PBR material, HDR and shadow mapping
* `ldr blending` and `hdr blending`: use for testing GPU performance when doing alpha blending
in LDR and HDR
* `textures`: render a texture quad with compressed texture, various kinds of filterings
* `cubemap textures`: render a cubemap environment texture
* `simd`: SIMD testing
* `lua scripting`: Lua scripting testing

### Render Tech
* `pbr helmet`: render Damaged Helmet GLTF model in HDR + PBR + FXAA
* `hdr bloom`: HDR blooming, using the same technique with Unity Engine
* `sky`: precompute the sky model in the CPU.<br>
Reference: https://ebruneton.github.io/precomputed_atmospheric_scattering/
* `sky runtime`: render the sky using precomputed data from `sky`

### Tools
* `sh calculator`: calculate SH coeffs from cube map
* `noise`: generate simplex noise using `opensimplex` library

### Misc
* `camera work`: demo the camera WVP and the effect of orientation of matrices
* `game of life`: Conway's game of life demo
* `font`: Font rendering

## Screenshots / Videos
Spherical Harmonics coefficients calculator
![alt text](https://github.com/kolorist/insigne_dev/blob/master/screenshots/sh_calculator.jpg)

Damaged Helmet
![alt text](https://github.com/kolorist/insigne_dev/blob/master/screenshots/c_helmet_cloudy_vondelpark.png)


Mali Bifrost's hardware counter (running on Samsung Galaxy S10e)
![alt text](https://github.com/kolorist/insigne_dev/blob/master/screenshots/c_mali_hardware_counters.jpg)

Conway's game of life, featuring HDR bloom: https://youtu.be/YIpDZvP923g
