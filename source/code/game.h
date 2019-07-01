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
InitializeArena(memory_arena *Arena, size_t Size, void *Base) {
  
  Arena->Size = Size;
  Arena->Base = (uint8 *)Base;
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


#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
inline void
ZeroSize(size_t Size, void *Ptr) {
  
  uint8 *Byte = (uint8 *)Ptr;
  while(Size--) {
    
    *Byte++ = 0;
  }
}

#define MINE_RAND_MAX 32767

internal uint32
RollTheDice(void) {
  
  static uint32 next = 17;
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % (MINE_RAND_MAX + 1);
}

#include "game_intrinsic.h"
#include "game_math.cpp"
#include "game_world.h"
#include "game_sim_region.h"
#include "game_entity.h"


struct loaded_bitmap {
  int32 Height;
  int32 Width;
  
  uint32* Pixels;
};

struct hero_bitmaps {
  
  loaded_bitmap HeroHead;
  loaded_bitmap HeroCape;
  loaded_bitmap HeroTorso;
  v2 Align;
};


struct low_entity {
  
  world_position P;  
  world_position OldP;
  sim_entity Sim;
  
};



struct entity_visible_piece {
  
  loaded_bitmap *Bitmap;
  v2 Offset;
  real32 OffsetZ;
  real32 OffsetZC;
  real32 R, G, B, A;
  v2 Dim;
};

enum entity_residence {
  
  EntityResidence_NoneExistant,
  EntityResidence_Low,
  EntityResidence_High
};


struct low_entity_chunk_reference {
  
  world_chunk *TileChunk;
  uint32 EntityIndexInChunk;
};


struct controlled_entity {
  
  uint32 EntityIndex;
  v2 ddP;
  v2 dSword;
  real32 dZ;
};

struct pairwise_collision_rule {
  
  bool32 ShouldCollide;
  uint32 StorageIndexA;
  uint32 StorageIndexB;
  pairwise_collision_rule *NextInHash;
};
  
struct game_state {
  
  memory_arena WorldArena;
  world* World;

  controlled_entity ControlledEntities[ArrayCount(((game_input *)0)->Controllers)];
  
  uint32 LowEntityCount;
  low_entity LowEntity[10000];
  
  uint32 CameraFollowingEntityIndex;
  world_position CameraP;
  
  loaded_bitmap Backdrop;
  hero_bitmaps Hero[4];
  
  real32 MetersToPixels;
  
  loaded_bitmap Tree;
  loaded_bitmap Sword;
  
  // TODO(Egor): must be a power of two
  pairwise_collision_rule *CollisionRuleHash[256];
  pairwise_collision_rule *FirstFreeCollisionRule;
};

struct entity_visible_piece_group {
  
  game_state *GameState;
  uint32 Count;
  entity_visible_piece Pieces[32];
};

inline low_entity *
GetLowEntity(game_state *GameState, uint32 LowIndex) {
  
  low_entity *Result = 0;
  
  if((LowIndex > 0) && (LowIndex < GameState->LowEntityCount)) {
    
    Result = GameState->LowEntity + LowIndex;
  }
  
  return Result;
}

#define GAME_H
#endif
