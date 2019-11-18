#if !defined(GAME_INTRINSICS_H)
#include "math.h"


inline uint32 CompareExchangeUInt32(uint32 volatile *Value, uint32 Expected, uint32 New) {
 
  uint32 Result = _InterlockedCompareExchange((long *)Value, Expected, New);
  return Result;
}

inline int32 SignOf(int32 Value) {
  
  int32 Result = Value >= 0 ? 1 : -1;
  return Result;
}

inline real32 AbsoluteValue(real32 Real32) {
 
  real32 Result = (real32)fabs(Real32);
  return Result;
}

inline int32 AbsoluteValue(int32 Real32) {
  
  int32 Result = (int32)fabs(Real32);
  return Result;
}

inline real32 Cos(real32 Phi) {
 
  real32 Result = cosf(Phi);
  return Result;
}

inline real32 Sin(real32 Phi) {
  
  real32 Result = sinf(Phi);
  return Result;
}

inline real32 
SquareRoot(real32 Value) {
  
  real32 Result = sqrtf(Value);
  return Result;
}

inline int32 RoundReal32ToInt32(real32 Real32) {
  // TODO: intrinsic 
  int32 Result = (int32)roundf(Real32);
  return Result;
}

inline uint32 RoundReal32ToUInt32(real32 Real32) {
  // TODO: intrinsic 
  uint32 Result = (uint32)roundf(Real32);
  return Result;
}

inline int32 FloorReal32ToInt32(real32 Real32) {
  
  int32 Result = (int32)floorf(Real32);
  return Result;
}

inline int32 CeilReal32ToInt32(real32 Real32) {
  
  int32 Result = (int32)ceilf(Real32);
  return Result;
}

inline int32 TruncateReal32ToInt32(real32 Real32) {
  int32 Result = (int32)Real32;
  return Result;
}


struct bit_scan_result {
  bool32 Found;
  uint32 Index;
};

inline uint32
RotateLeft(uint32 Value, int32 Amount) {
  
  uint32 Result = _rotl(Value, Amount);
  return Result;
}

inline uint32
RotateRight(uint32 Value, int32 Amount) {
  
  uint32 Result = _rotr(Value, Amount);
  return Result;
}


inline bit_scan_result 
FindLeastSignificantSetBit(uint32 Value) {
  
  bit_scan_result Result = {};
  
#if COMPILER_MSVC
  
  Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
  
  for(uint32 Test = 0; Test < 32; ++Test) {
    
    if(Value & (1 << Test)) {
      Result.Index = Test;
      Result.Found = true;
      break;
    }
  }
#endif
  
  return Result;
  
}


#define GAME_INTRINSICS_H
#endif