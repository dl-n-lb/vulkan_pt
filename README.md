# Vulkan Path Tracer
This is the continuation of work on [`vk_rt_simple`](https://github.com/mattdrinksglue/vk_rt_simple). Work is being done to redesign the architecture to support more complex methods of path tracing as well as more types of materials.

# Progress
- [x] Normal Maps
- [ ] Fix lighting bugs
- [ ] Light Sampling
- [ ] MIS
- [ ] Reorganise shaders into reusable files
- [ ] Disney BSDF
- [ ] Reprojection
- [ ] Sample reuse

# Old README
This was put together to get used to the vulkan/vulkan ray tracing apis. The raytracing itself is literally the most basic possible integrator where it just randomly samples on a cosine distribution.
My code depends on:
- [cgltf](https://github.com/jkuhlmann/cgltf)
- [vulkan memory allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [handmade math](https://github.com/HandmadeMath/HandmadeMath)
- [stb_image.h](https://github.com/nothings/stb)
- [dear imgui](https://github.com/ocornut/imgui)
- [cimgui](https://github.com/cimgui/cimgui)

To compile and run:
```zsh
> make main
> ./main
```
Expect to see a more tidy/practical implementation on my github soon, possibly with more features implemented.

(MIT license - but please don't actually use this.)
