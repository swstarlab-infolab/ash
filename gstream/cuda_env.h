#ifndef GSTREAM_CUDA_ENV_H
#define GSTREAM_CUDA_ENV_H
#include <ash/pp/pparg.h>
#include <ash/config.h>

#if defined(__CUDACC__)
#define GSTREAM_DEVICE_COMPATIBLE __host__ __device__
#define GSTREAM_HOST_ONLY __host__
#define GSTREAM_DEVICE_ONLY __device__
#define GSTREAM_CUDA_KERNEL __global__
#define CUDA_atomicCAS(...) atomicCAS(__VA_ARGS__)
#define CUDA_atomicAdd(...) atomicAdd(__VA_ARGS__)
#define CUDA_atomicAdd_block(...) atomicAdd_block(__VA_ARGS__)
#define CUDA_atomicOr(...) atomicOr(__VA_ARGS__)
#define CUDA_atomicOr_block(...) atomicOr(__VA_ARGS__)
#define CUDA_atomicLoad(address) atomicAdd(address, 0)
#else
#define GSTREAM_DEVICE_COMPATIBLE
#define GSTREAM_HOST_ONLY 
#define GSTREAM_DEVICE_ONLY [[deprecated]]
#define GSTREAM_CUDA_KERNEL 
#endif

#if !defined(__CUDACC__) && defined(ASH_TOOLCHAIN_MSVC)
// http://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#

template <typename T>
inline T __unused_function__(...) { return T{}; }

#define CUDA_atomicCAS(address, expected, desired) __unused_function__<decltype(expected)>(address, expected, desired)
#define CUDA_atomicAdd(address, val) __unused_function__<decltype(val)>(address, val) 
#define CUDA_atomicAdd_block(address, val) __unused_function__<decltype(val)>(address, val) 
#define CUDA_atomicOr(address, val) __unused_function__<decltype(val)>(address, val) 
#define CUDA_atomicOr_block(address, val) __unused_function__<decltype(val)>(address, val) 
#define CUDA_atomicLoad(address) __unused_function__<int>(address, 0) 
#define __threadfence_block() 1
#define __syncthreads() 1
#define __syncwarp() 1
#define __shfl_down_sync(...) 1
// https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#warp-shuffle-functions
#define __shfl_sync(/*unsigned*/ mask, /*T*/ var, /*int*/ srcLane, /*int*/ width /*= warpSize*/) 1
#define GSTREAM_CUDA_KERNEL_CALL_2(Kernel, GridDim, BlockDim) \
    _ash_unused(GridDim); _ash_unused(BlockDim); Kernel
#define GSTREAM_CUDA_KERNEL_CALL_3(Kernel, GridDim, BlockDim, Shm) \
    _ash_unused(GridDim); _ash_unused(BlockDim); _ash_unused(Shm); Kernel
#define GSTREAM_CUDA_KERNEL_CALL_4(Kernel, GridDim, BlockDim, Shm, Stream) \
    _ash_unused(GridDim); _ash_unused(BlockDim); _ash_unused(Shm); _ash_unused(Stream); Kernel
#else
#define GSTREAM_CUDA_KERNEL_CALL_2(Kernel, GridDim, BlockDim) Kernel<<<GridDim, BlockDim>>>
#define GSTREAM_CUDA_KERNEL_CALL_3(Kernel, GridDim, BlockDim, Shm) Kernel<<<GridDim, BlockDim, Shm>>>
#define GSTREAM_CUDA_KERNEL_CALL_4(Kernel, GridDim, BlockDim, Shm, Stream) Kernel<<<GridDim, BlockDim, Shm, Stream>>>
#endif // !__INTELLISENSE__

#define __GSTREAM_CUDA_KERNEL_CALL_N(N) GSTREAM_CUDA_KERNEL_CALL_##N
#define _GSTREAM_CUDA_KERNEL_CALL_N(N) __GSTREAM_CUDA_KERNEL_CALL_N(N)
#define GSTREAM_CUDA_KERNEL_CALL(CUDA_Kernel, ...) \
    ASH_PP_EXPAND(_GSTREAM_CUDA_KERNEL_CALL_N(ASH_PP_NARG(__VA_ARGS__)) (CUDA_Kernel, __VA_ARGS__))

namespace gstream {
namespace cuda {

using device_id_t = int;
using stream_type = void*;

} // namespace cuda
} // namespace gstream

#endif // !GSTREAM_CUDA_ENV_H
