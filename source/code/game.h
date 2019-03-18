#if !defined(GAME_H)
/* ========================================================================
   ======================================================================== */

/*
  NOTE(Egor):
  
  GAME_INTERNAL:
    0 - Build for public release
    1 - Build for developer only
    
  GAME_SLOW:
    0 - No slow code allowed!
    1 - Slow code welcome.
*/


#include "game_platform.h"


#define internal static 
#define local_persist static 
#define global_variable static

#define Pi32 3.14159265359f


#if GAME_SLOW
// TODO(Egor): Complete assertion macro - don't worry everyone!
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
// TODO(Egor): swap, min, max ... macros???

inline uint32
SafeTruncateUInt64(uint64 Value)
{
  // TODO(Egor): Defines for maximum values
  Assert(Value <= 0xFFFFFFFF);
  uint32 Result = (uint32)Value;
  return(Result);
}

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return(Result);
}


struct memory_arena {
  size_t Size;
  uint8 *Base;
  size_t Used;
  
};


internal void 
InitializeArena(memory_arena *Arena, size_t Size, uint8 *Base) {
  
  Arena->Size = Size;
  Arena->Base = Base;
  Arena->Used = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
void *
PushSize_(memory_arena *Arena, size_t Size) {
  
  Assert((Arena->Used + Size) <= Arena->Size);
  void *Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return(Result);
}

#define MINE_RAND_MAX 32767

internal uint32
RollTheDice(void) {
  
  static uint32 next = 17;
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % (MINE_RAND_MAX + 1);
}

#include "game_intrinsic.h"
#include "game_tile.h"

struct world {
  
  tile_map *TileMap;
};

struct loaded_bitmap {
  uint32 Height;
  uint32 Width;
  
  uint32* Pixels;
};
  
struct game_state {
  
  memory_arena WorldArena;
  world* World;
  tile_map_position PlayerP;
  
  loaded_bitmap Backdrop;
  loaded_bitmap HeroHead;
  loaded_bitmap HeroCape;
  loaded_bitmap HeroTorso;
  loaded_bitmap Feature;
  
  bool32 IsFeatureOn;
  
};

#define GAME_H
#endif
