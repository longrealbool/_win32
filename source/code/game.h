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

#include "game_math.cpp"
#include "game_intrinsic.h"
#include "game_tile.h"

struct world {
  
  tile_map *TileMap;
};

struct loaded_bitmap {
  int32 Height;
  int32 Width;
  
  uint32* Pixels;
};

struct hero_bitmaps {
  
  loaded_bitmap HeroHead;
  loaded_bitmap HeroCape;
  loaded_bitmap HeroTorso;
  uint32 AlignX;
  uint32 AlignY;
};

struct high_entity {
  
  bool32 Exists;

  v2 P; // NOTE(Egor): already relative to the camera
  v2 dP;
  uint32 FacingDirection;
  uint32 AbsTileZ;
};

struct low_entity {
  

  
};

struct dormant_entity {
  
  tile_map_position P;  
  real32 Height;
  real32 Width;
  
  bool32 Collides;
  // NOTE(Egor): stairs implementation
  int32 dAbsTileZ;
};

enum entity_residence {

  EntityResidence_NoneExistant,
  EntityResidence_Dormant,
  EntityResidence_Low,
  EntityResidence_High
};

struct entity {
  
  uint32 Residence;
  high_entity *High;
  low_entity *Low;
  dormant_entity *Dormant;
};
  
struct game_state {
  
  memory_arena WorldArena;
  world* World;

  uint32 PlayerIndexForController[ArrayCount(((game_input *)0)->Controllers)];
  uint32 EntityCount;
  
  entity_residence EntityResidency[256];
  high_entity HighEntity[256];
  low_entity LowEntity[256];
  dormant_entity DormantEntity[256];
  
  
  uint32 CameraFollowingEntityIndex;
  tile_map_position CameraP;
  
  loaded_bitmap Backdrop;
  hero_bitmaps Hero[4];

  
};

#define GAME_H
#endif
