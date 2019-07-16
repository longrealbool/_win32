#if !defined(GAME_ENTITY_H)

#define INVALID_P V3(100000.0f, 100000.0f, 100000.0f)

struct move_spec {
  
  bool32 UnitMaxAccelVector;
  real32 Speed;
  real32 Drag;
};

inline bool32
IsSet(sim_entity *Entity, uint32 TestFlag) {
  
  bool32 Result = Entity->Flags & TestFlag;
  return Result;
}

inline void 
AddFlag(sim_entity *Entity, uint32 Flag) {
 
  Entity->Flags |= Flag;
}

inline void
ClearFlag(sim_entity *Entity, uint32 Flag) {
 
  Entity->Flags &= ~Flag;
}

inline void
MakeEntityNonSpatial(sim_entity *Entity) {
  
  Entity->P = INVALID_P;
  AddFlag(Entity, EntityFlag_NonSpatial);
}

inline void
MakeEntitySpatial(sim_entity *Entity, v3 P, v3 dP) {
  
  Entity->P = P;
  Entity->dP = dP;
  ClearFlag(Entity, EntityFlag_NonSpatial);
}

inline v3
GetEntityGroundPoint(sim_entity *Entity) {
  
  v3 Result = Entity->P;
  return Result;
}

inline real32
GetStairGround(sim_entity *Entity, v3 AtGroundPoint) {

  Assert(Entity->Type == EntityType_Stairwell);
  
  rectangle2 EntityRect = RectCenterDim(Entity->P.XY, Entity->WalkableDim);
  // NOTE(Egor): get local normalized over axis coordinates inside AABB
  v2 Bary = Clamp01(GetBarycentric(EntityRect, AtGroundPoint.XY));
  real32 Ground = Entity->P.Z + Bary.Y*Entity->WalkableHeight;
  
  return Ground;
}


#define GAME_ENTITY_H
#endif