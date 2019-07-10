#if !defined(GAME_SIM_REGION_H)

enum entity_type {
  
  EntityType_Null,
  EntityType_Hero,
  EntityType_Monster,
  EntityType_Familiar,
  EntityType_Wall,
  EntityType_Sword,
  EntityType_Stairwell
};


#define SUB_HIT_POINT 4
struct hit_point {
  
  uint8 Flags;
  uint8 FilledAmount;
};

struct sim_entity;
union entity_reference {
  
  sim_entity *Ptr;
  uint32 Index;
};

enum sim_entity_flags {
  
  EntityFlag_Collides = (1 << 1),
  EntityFlag_NonSpatial = (1 << 2),
  EntityFlag_Moveable = (1 << 3),
  EntityFlag_Simulated = (1 << 30),
};



struct sim_entity {
  
  world_chunk *OldChunk;
  uint32 StorageIndex;
  bool32 Updatable;
  
  entity_type Type;
  uint32 Flags;
  
  v3 P; // NOTE(Egor): already relative to the camera
  v3 dP;
  
  // NOTE(Egor): stairs implementation
  int32 dAbsTileZ;
  
  uint32 FacingDirection;
  
  v3 Dim;
  
  uint32 HitPointMax;
  hit_point HitPoint[16];
  
  entity_reference Sword;
  //real32 DistanceRemaining;
  real32 DistanceLimit;
};



struct sim_entity_hash {
 
  sim_entity *Ptr;
  uint32 Index;
};

struct sim_region {
  
  world *World;
  world_position Origin;
  
  real32 MaxEntityRadius;
  real32 MaxEntityVelocity;
  
  rectangle3 UpdateBounds;
  rectangle3 Bounds;
  
  uint32 MaxEntityCount;
  uint32 EntityCount;
  sim_entity *Entities;
  
  sim_entity_hash Hash[4096];
};

#define GAME_WORLD_H
#endif