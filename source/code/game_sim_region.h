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
  
  EntityFlag_Collides = (0x1 << 1),
  EntityFlag_NonSpatial = (0x1 << 2),
  EntityFlag_Moveable = (0x1 << 3),
  EntityFlag_ZSupported = (0x1 << 4),
  EntityFlag_Simulated = (0x1 << 30),
};


struct sim_entity_collision_volume {

  v3 OffsetP;
  v3 Dim;
};


struct sim_entity_collision_volume_group {
  
  sim_entity_collision_volume TotalVolume;
  
  // TODO(Egor): must be 1 or greater
  uint32 VolumeCount;
  sim_entity_collision_volume *Volumes;
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
  
  sim_entity_collision_volume_group *Collision;
  
  uint32 HitPointMax;
  hit_point HitPoint[16];
  
  entity_reference Sword;
  real32 DistanceLimit;
  
  // NOTE(Egor): for stairs only
  real32 WalkableHeight;
  v2 WalkableDim;
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