# Odin Render System
primery code about DX12 with beginning
just for learning
VS 2017 Windows SDK 10.0.17763.0 or greater

2020-02-07
1. Screen-space reflection component.
2. Recover direct light.
3. Standard grid support.


2020-01-12
1. Clean up because of differences between graphics context and compute context.
2. Extract the blur function as a utility function.
3. Ssao with blur.

2020-01-10
1. Recover ssao without blur.

2020-01-01
1. IBL:environment lighting.
2. Sphere render object config.

2019-12-30
1. Cube texture.
2. Skybox.
3. Game settings configure modification.

2019-12-11
1. Deferred lighting for 4 passes.
2. Simple render group, render queue.
3. Singleton base template class.

2019-12-04
1. Deferred shading optimize, reconstruct world position with depth map and matrix[view*projection]^-1, instead of      sampleing frag world position.

2019-12-02
1. Fixed bug for lack of memory in every render frame.
2. Simple render object data strcut.
3. Simple config for game settings.
4. Point light shading.
5. Unperfect solution of deferred shading.

2019-10-22
1. Simple camera and camera controller.

2019-10-07
1. Compute context.
2. Simple blur for 'Depth of View'.

2019-09-15
1. Fixed bug about color buffer with 'uav' flag and transition creation.

2019-08-30
1. Dynamic descriptor heap for command context.
2. Set descriptor table for texture resource.
3. Simple texture structure for grouping resource and handles.

2019-08-30
1. Dynamic descriptor heap for command context.
2. Set descriptor table for texture resource.
3. Simple texture structure for grouping resource and handles.

2019-08-24
1. Simple game core .
2. Upgrade render system(?).

2019-08-22
1. Simple graphics core.

2019-08-18
1. ResourceBarrier.

2019-08-17
1. GpuResource  
2. Pixel Buffer, and Color Buffer/Depth Stencil Buffer for render target.
3. Gpu Buffer, and Structure Buffer for vertex buffer, and ByteAddress Buffer for index buffer.

2019-08-13
1. Linear Allocator.

2019-08-10
1. CommandQueue vs Fence.

2019-08-07
1. CommandAllocator.
2. CommandContext.
3. CommandQueue.
4. DescriptorAllocator.

2019-08-03
1. RootParameter vs RootSignature.
2. Pso and GraphicsPso.

2019-08-01
1. Fixed shadow issue with no scaling

2019-07-30
1. Shadow helper class.
2. 2d texture debug layer.
3. Depth map debug shader and pso ect..,.
4. Floor render item to recieve shadow.
5. Shadow sampler.
6. Shadow in scene.

issues:
Only a few area of light on the floor, not all the floor.


2019-07-22
1. Normal map.
2. Different shaders for standard opaque and normal map opaque.
3. Some issues of draw call.

2019-07-19
1. Dynamic cube mapping.

2019-07-17
1. Sky render item.
2. Sky cube map.
3. Sky box shader.
4. Some issuses still here, such as sky item visibility.
5. The root signature, can i swap [4] and [5]?.

2019-07-11
1. Devide render layer and patch buffer, primery render queue.
2. Additional pick function for picking, change material for pick effect.
3. Retry bounding box for picking, but still some issues for picking, maybe ray to local transform.

2019-07-08
1. Update project.
2. Frustum culling enable.
3. Skull texcood update.
4. Instance data material bind.

2019-07-05
1. Complete instance data.
