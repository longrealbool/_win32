#if !defined(GAME_INTRINSICS_H)
#include "math.h"

inline real32 AbsoluteValue(real32 Real32) {
 
  real32 Result = (real32)fabs(Real32);
  
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
  // TODO: dirty hack should consider moving to intrinsic
  
  int32 Result = (int32)floorf(Real32);
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