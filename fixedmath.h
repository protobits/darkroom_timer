#ifndef __FIXEDMATH_H__
#define __FIXEDMATH_H__

#define FIXED_MANTISSA_BITS 12

#define FIXED2UINT(x) ((x) >> FIXED_MANTISSA_BITS)
#define UINT2FIXED(x) ((x) << FIXED_MANTISSA_BITS)

#endif /* __FIXEDMATH_H__ */