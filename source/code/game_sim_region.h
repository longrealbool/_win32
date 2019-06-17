#if !defined(GAME_SIM_REGION_H)

enum entity_type {
  
  EntityType_Null,
  EntityType_Hero,
  EntityType_Monster,
  EntityType_Familiar,
  EntityType_Wall,
  EntityType_Sword
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

struct sim_entity {
  
  uint32 StorageIndex;
  
  v2 P; // NOTE(Egor): already relative to the camera

  uint32 FacingDirection;
  uint32 ChunkZ;
  
  real32 Z;
  real32 dZ;
  
  entity_type Type;
  

  v2 dP;
  real32 Height;
  real32 Width;
  
  bool32 Collides;
  // NOTE(Egor): stairs implementation
  int32 dAbsTileZ;
  
  uint32 HitPointMax;
  hit_point HitPoint[16];
  
  entity_reference Sword;
  real32 DistanceRemaining;
};

struct move_spec {
  
  bool32 UnitMaxAccelVector;
  real32 Speed;
  real32 Drag;
};

struct sim_entity_hash {
 
  sim_entity *Ptr;
  uint32 Index;
};

struct sim_region {
  
  world *World;
  
  world_position Origin;
  rectangle2 Bounds;
  
  uint32 MaxEntityCount;
  uint32 EntityCount;
  sim_entity *Entities;
  
  sim_entity_hash Hash[4096];
};

#define GAME_WORLD_H
#endif