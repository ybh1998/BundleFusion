#pragma once

#ifndef _CUDA_UTIL_
#define _CUDA_UTIL_

#undef max
#undef min

#include <cutil_inline.h>
#include <cutil_math.h>

// Enable run time assertion checking in kernel code
#define cudaAssert(condition) if (!(condition)) { printf("ASSERT: %s %s\n", #condition, __FILE__); }
//#define cudaAssert(condition)

# if defined(__CUDA_ARCH__)
#define __CONDITIONAL_UNROLL__ #pragma unroll
# else
#define __CONDITIONAL_UNROLL__ 
# endif

# if __CUDACC_VER_MAJOR__ >= 9

// Note: From CUDA 9, warp-level api has changed. Turing cards will hang forever if warp-level operation not treated correctly.
// (https://devblogs.nvidia.com/using-cuda-warp-level-primitives/)
// (https://github.com/opencv/opencv/pull/13658/commits/970293a229ef314603ffaf77fc62495bf849aba8)

#define FULL_MASK 0xffffffff

__forceinline__ __device__
float warpReduceSum(float val) {
  unsigned mask = __ballot_sync(FULL_MASK, threadIdx.x < warpSize);
  for (int offset = warpSize / 2; offset > 0; offset /= 2)
    val += __shfl_down_sync(mask, val, offset);
  return val;
}
__forceinline__ __device__
float warpReduceMax(float val) {
  unsigned mask = __ballot_sync(FULL_MASK, threadIdx.x < warpSize);
  for (int offset = warpSize / 2; offset > 0; offset /= 2) {
    float temp = __shfl_down_sync(mask, val, offset);
    val = max(val, temp);
  }
  return val;
}
__forceinline__ __device__
float warpReduceMin(float val) {
  unsigned mask = __ballot_sync(FULL_MASK, threadIdx.x < warpSize);
  for (int offset = warpSize / 2; offset > 0; offset /= 2) {
    val = min(val, __shfl_down_sync(mask, val, offset));
  }
  return val;
}

__forceinline__ __device__
float warpReduceSumAll(float val) {
  unsigned mask = __ballot_sync(FULL_MASK, threadIdx.x < warpSize);
  for (int offset = warpSize / 2; offset > 0; offset /= 2) {
    val += __shfl_xor_sync(mask, val, offset);
  }
  return val;
}
__forceinline__ __device__
float warpReduceMaxAll(float val) {
  unsigned mask = __ballot_sync(FULL_MASK, threadIdx.x < warpSize);
  for (int offset = warpSize / 2; offset > 0; offset /= 2) {
    val = max(val, __shfl_xor_sync(mask, val, offset));
  }
  return val;
}
__forceinline__ __device__
float warpReduceMinAll(float val) {
  unsigned mask = __ballot_sync(FULL_MASK, threadIdx.x < warpSize);
  for (int offset = warpSize / 2; offset > 0; offset /= 2) {
    val = min(val, __shfl_xor_sync(mask, val, offset));
  }
  return val;
}
# elif __CUDACC__ // __CUDACC_VER_MAJOR__ < 9
//__inline__ __device__
//float warpReduceSum(float val) {
//	for (int offset = warpSize / 2; offset > 0; offset /= 2)
//		val += __shfl_down(val, offset);
//	return val;
//}
//__inline__ __device__
//float warpReduceMax(float val) {
//	for (int offset = 32 / 2; offset > 0; offset /= 2) {
//		val = max(val, __shfl_down(val, offset, 32));
//	}
//	return val;
//}
//__inline__ __device__
//float warpReduceMin(float val) {
//	for (int offset = 32 / 2; offset > 0; offset /= 2) {
//		val = min(val, __shfl_down(val, offset, 32));
//	}
//	return val;
//}
//
//__inline__ __device__
//float warpReduceSumAll(float val) {
//	for (int offset = 32 / 2; offset > 0; offset /= 2) {
//		val += __shfl_xor(val, offset, 32);
//	}
//	return val;
//}
//__inline__ __device__
//float warpReduceMaxAll(float val) {
//	for (int offset = 32 / 2; offset > 0; offset /= 2) {
//		val = max(val, __shfl_xor(val, offset, 32));
//	}
//	return val;
//}
//__inline__ __device__
//float warpReduceMinAll(float val) {
//	for (int offset = 32 / 2; offset > 0; offset /= 2) {
//		val = min(val, __shfl_xor(val, offset, 32));
//	}
//	return val;
//}
# endif

#endif
