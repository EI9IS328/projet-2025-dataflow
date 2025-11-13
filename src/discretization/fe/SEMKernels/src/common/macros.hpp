#pragma once


#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
#define SEMKERNELS_HOST_DEVICE __host__ __device__
#else
#define SEMKERNELS_HOST_DEVICE
#endif

#define SEMKERNELS_INLINE inline
