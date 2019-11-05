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
  int32 TempCount;
  
};


internal void 
InitializeArena(memory_arena *Arena, size_t Size, void *Base) {
  
  Arena->Size = Size;
  Arena->Base = (uint8 *)Base;
  Arena->Used = 0;
  Arena->TempCount = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
#define PushSize(Arena, Size) PushSize_(Arena, Size)

void *PushSize_(memory_arena *Arena, size_t Size) {
  
  Assert((Arena->Used + Size) <= Arena->Size);
  void *Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return(Result);
}

struct temporary_memory {
  
  memory_arena *Arena;
  size_t Used;
};

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena) {
  
  temporary_memory Result;
  
  Result.Arena = Arena;
  Result.Used = Arena->Used;
  Result.Arena->TempCount++;
  
  return Result;
}

inline void EndTemporaryMemory(temporary_memory TempMem) {
  
  memory_arena *Arena = TempMem.Arena;
  Assert(Arena->Used >= TempMem.Used);
  
  
  Arena->Used = TempMem.Used;
  Assert(Arena->TempCount > 0);
  Arena->TempCount--;
}

inline void CheckArena(memory_arena *Arena) {
  
  
  Assert(Arena->TempCount == 0);
  
}


#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
inline void
ZeroSize(size_t Size, void *Ptr) {
  
  uint8 *Byte = (uint8 *)Ptr;
  while(Size--) {
    
    *Byte++ = 0;
  }
}


#include "game_intrinsic.h"
#include "game_math.cpp"
#include "game_random.h"
#include "game_render_group.h"
#include "game_world.h"
#include "game_sim_region.h"
#include "game_entity.h"


struct hero_bitmaps {
  
  loaded_bitmap HeroHead;
  loaded_bitmap HeroCape;
  v2 Align;
};


struct low_entity {
  
  world_position P;  
  world_position OldP;
  sim_entity Sim;
  
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
  
  bool32 CanCollide;
  uint32 StorageIndexA;
  uint32 StorageIndexB;
  pairwise_collision_rule *NextInHash;
};

struct ground_buffer {
  
  world_position P;
  loaded_bitmap Bitmap;
};

struct game_state {
  
  memory_arena WorldArena;
  world* World;
  
  controlled_entity ControlledEntities[ArrayCount(((game_input *)0)->Controllers)];
  
  uint32 LowEntityCount;
  low_entity LowEntity[10000];
  
  loaded_bitmap Grass[2];
  loaded_bitmap Stones[4];
  loaded_bitmap Tuft[3];
  loaded_bitmap Slumps[4];
  
  uint32 CameraFollowingEntityIndex;
  world_position CameraP;
  
  loaded_bitmap Backdrop;
  hero_bitmaps Hero[4];
  
  
  loaded_bitmap Tree;
  loaded_bitmap Tree1;
  loaded_bitmap Sword;
  loaded_bitmap Stairwell;
  
  // TODO(Egor): must be a power of two
  pairwise_collision_rule *CollisionRuleHash[256];
  pairwise_collision_rule *FirstFreeCollisionRule;
  
  sim_entity_collision_volume_group *NullCollision;
  sim_entity_collision_volume_group *SwordCollision;
  sim_entity_collision_volume_group *StairCollision;
  sim_entity_collision_volume_group *PlayerCollision;
  sim_entity_collision_volume_group *WallCollision;
  sim_entity_collision_volume_group *MonsterCollision;
  sim_entity_collision_volume_group *FamiliarCollision;
  sim_entity_collision_volume_group *StandardRoomCollision;
  
  real32 FloorHeight;
  
  real32 OffsetZ;
  
  real32 Time;
  real32 Scale;
  real32 Angle;
  
  loaded_bitmap TestDiffuse;
  loaded_bitmap TestNormal;
};


struct transient_state {
  
  platform_work_queue *RenderQueue;
  platform_work_queue *LowPriorityQueue;
  
  bool32 Initialized;
  memory_arena TranArena;
  uint32 GroundBufferCount;
  ground_buffer *GroundBuffers;

  uint32 EnvMapWidth;
  uint32 EnvMapHeight;
  environment_map EnvMaps[3];
};


inline low_entity *
GetLowEntity(game_state *GameState, uint32 LowIndex) {
  
  low_entity *Result = 0;
  
  if((LowIndex > 0) && (LowIndex < GameState->LowEntityCount)) {
    
    Result = GameState->LowEntity + LowIndex;
  }
  
  return Result;
}

global_variable platform_add_entry *PlatformAddEntry;
global_variable platform_complete_all_work *PlatformCompleteAllWork;

#define GAME_H
#endif
