internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source, v3 *SimP);




inline bool32 
EntityOverlapsRectangle(v3 P, sim_entity_collision_volume Volume, rectangle3 Rect) {
  
  rectangle3 GrownRectange = AddRadiusTo(Rect, 0.5f*Volume.Dim);
  bool32 Result = IsInRectangle(GrownRectange, P + Volume.OffsetP);
  
  return Result;
}

inline v3
GetSimSpaceP(sim_region *SimRegion, low_entity *Stored) {
  
  v3 Result = INVALID_P;
  if(!IsSet(&Stored->Sim, EntityFlag_NonSpatial)) {
    
    Result = Subtract(SimRegion->World,
                      &Stored->P, &SimRegion->Origin);
  }
  
  return Result;
}


internal sim_entity_hash *
GetHashFromStorageIndex(sim_region *SimRegion, uint32 StorageIndex) {
  
  Assert(StorageIndex);
  
  sim_entity_hash *Result = 0;
  uint32 HashValue = StorageIndex;
  for(uint32 Offset = 0; Offset < ArrayCount(SimRegion->Hash); ++Offset) {
    
    uint32 HashMask = ArrayCount(SimRegion->Hash) - 1;
    uint32 HashIndex = (HashValue + Offset) & HashMask;
    sim_entity_hash *Entry = SimRegion->Hash + HashIndex;
    
    if((Entry->Index == StorageIndex) || 
       (Entry->Index == 0)) {
      
      Result = Entry;
      break; 
    }
  }
  
  return Result;
}




inline void
LoadEntityReference(game_state *GameState, sim_region *SimRegion, entity_reference *Ref) {
  
  if(Ref->Index) {
    
    sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, Ref->Index);
    if(Entry->Ptr == 0) {
      
      Entry->Index = Ref->Index;
      low_entity *Low = GetLowEntity(GameState, Ref->Index);
      v3 SimP = GetSimSpaceP(SimRegion, Low);
      Entry->Ptr = AddEntity(GameState, SimRegion, Ref->Index, Low, &SimP);
    }
    Ref->Ptr = Entry->Ptr;
  }
}


inline void
StoreEntityReference(entity_reference *Ref) {
  
  if(Ref->Ptr != 0) {
    
    Ref->Index = Ref->Ptr->StorageIndex;
  }
}



internal sim_entity *
AddEntityRaw(game_state *GameState, sim_region *SimRegion,
             uint32 StorageIndex, low_entity *Source ) {
  
  Assert(StorageIndex);
  sim_entity *Entity = 0;
  
  
  // NOTE(Egor): check for preventing multiple adding 
  sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
  if(!Entry->Ptr) {
    
    if(SimRegion->EntityCount < SimRegion->MaxEntityCount) {
      
      Entity = SimRegion->Entities + SimRegion->EntityCount++;
      
      Assert(Entry->Index == StorageIndex || Entry->Index == 0);
      Entry->Index = StorageIndex;
      Entry->Ptr = Entity;
      
      if(Source) {
        
        *Entity = Source->Sim;
        
        // This storage entity already simmed
        AddFlag(&Source->Sim, EntityFlag_Simulated);
        
        // NOTE(Egor): when we walk here, we have Index in Reference Union from LowEntity
        // but when we leave the scope, we already have pointer in sim_entity Reference
        LoadEntityReference(GameState, SimRegion, &Entity->Sword);
      }
      
      Entity->StorageIndex = StorageIndex;
      Entity->Updatable = false;
    }
    else {
      
      InvalidCodePath;
      
    }
  }
  
  return Entity;
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source, v3 *SimP) {
  
  sim_entity *Dest = AddEntityRaw(GameState, SimRegion, StorageIndex, Source);
  
  if(Dest) {
    
    if(SimP) {
      
      Dest->P = *SimP;
      Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Collision->TotalVolume, SimRegion->UpdateBounds);
    }
    else {
      Dest->P = GetSimSpaceP(SimRegion, Source);
    }
  }
  
  return Dest;
  
}


internal sim_region *
BeginSim(memory_arena *SimArena, game_state *GameState,  world *World, world_position Origin, rectangle3 Bounds, real32 dt) {
  
  sim_region *SimRegion = PushStruct(SimArena, sim_region);
  ZeroStruct(SimRegion->Hash);
  
  SimRegion->MaxEntityRadius = 5.0f;
  SimRegion->MaxEntityVelocity = 100.0f;
  
  real32 MaxEntityVelocity = SimRegion->MaxEntityVelocity;
  real32 MaxEntityRadius = SimRegion->MaxEntityRadius;
  
  real32 UpdateSafetyMargin =  MaxEntityRadius + dt* MaxEntityVelocity;
  real32 UpdateSafetyMarginZ = 1.0f;
  
  
  SimRegion->World = World;
  SimRegion->Origin = Origin;
  SimRegion->UpdateBounds = AddRadiusTo(Bounds, V3(MaxEntityRadius,
                                                   MaxEntityRadius,
                                                   MaxEntityRadius));
  
  SimRegion->Bounds = AddRadiusTo(SimRegion->UpdateBounds,
                                  V3(UpdateSafetyMargin,
                                     UpdateSafetyMargin,
                                     UpdateSafetyMarginZ));
  
  SimRegion->MaxEntityCount = 4096;
  SimRegion->EntityCount = 0;
  SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);
  
  
  world_position MinChunk = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
  world_position MaxChunk = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
  
  
  for(int32 ChunkZ = MinChunk.ChunkZ; ChunkZ <= MaxChunk.ChunkZ; ++ChunkZ) {
    for(int32 ChunkY = MinChunk.ChunkY; ChunkY <= MaxChunk.ChunkY; ++ChunkY) {
      for(int32 ChunkX = MinChunk.ChunkX; ChunkX <= MaxChunk.ChunkX; ++ChunkX) {
        
        world_chunk *Chunk = GetChunk(World, ChunkX, ChunkY, ChunkZ);
        if(Chunk) {
          
          for(world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next) { 
            for(uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex) {
              
              uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndex];
              low_entity *Low = GameState->LowEntity + LowEntityIndex;
              // NOTE(Egor): if entity is nonspatial we don't need to load it in sim region
              if(!IsSet(&Low->Sim, EntityFlag_NonSpatial)) {
                
                v3 SimSpaceP = GetSimSpaceP(SimRegion, Low);
                if(EntityOverlapsRectangle(SimSpaceP, Low->Sim.Collision->TotalVolume,
                                           SimRegion->Bounds)) {
                  
                  AddEntity(GameState, SimRegion, LowEntityIndex, Low, &SimSpaceP);
                }
              }
            }
          }
        }
      }
    }
  }
  
  return SimRegion;
}

inline sim_entity *
GetEntityByStorageIndex(sim_region *SimRegion, uint32 StorageIndex) {
  
  sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
  sim_entity *Result = Entry->Ptr;
  
  return Result;
}


internal void
EndSim(sim_region *Region, game_state *GameState) {
  
  sim_entity *Entity = Region->Entities;
  for(uint32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity) {
    
    
    low_entity *Stored = GameState->LowEntity + Entity->StorageIndex;
    
    Assert(IsSet(&Stored->Sim, EntityFlag_Simulated));
    Stored->Sim = *Entity;
    Assert(!IsSet(&Stored->Sim, EntityFlag_Simulated));
    
    StoreEntityReference(&Stored->Sim.Sword);
    
    world_position NewP = (IsSet(Entity, EntityFlag_NonSpatial) ? 
                           NullPosition() :
                           MapIntoChunkSpace(Region->World, Region->Origin, Entity->P));
    
    if(Entity->StorageIndex == 189) {
      int a = 3;
    }
    ChangeEntityLocation(&GameState->WorldArena, GameState->World,
                         Entity->StorageIndex, Stored, 
                         NewP);
    
    
    if(Entity->StorageIndex == GameState->CameraFollowingEntityIndex) {
      
      world_position NewCameraP = GameState->CameraP;
      NewCameraP.ChunkZ = Stored->P.ChunkZ;
#if 0
      
      
      if(CameraFollowingEntity.High->P.x > (9.0f * World->TileSideInMeters)) {
        NewCameraP.AbsTileX += 17;
      }
      if(CameraFollowingEntity.High->P.x < -(9.0f * World->TileSideInMeters)) {
        NewCameraP.AbsTileX -= 17;
      }
      if(CameraFollowingEntity.High->P.Y > (5.0f * World->TileSideInMeters)) {
        NewCameraP.AbsTileY += 9;
      }
      if(CameraFollowingEntity.High->P.Y < -(5.0f * World->TileSideInMeters)) {
        NewCameraP.AbsTileY -= 9;
      }
      
#else
      real32 CameraZOffset = NewCameraP.Offset_.z;
      NewCameraP = Stored->P;
      NewCameraP.Offset_.z = CameraZOffset;
#endif
      
      GameState->CameraP = NewCameraP;
    }
  }
}




internal bool32
TestWall(real32 *tMin, real32 WallC, real32 RelX, real32 RelY, 
         real32 PlayerDeltaX, real32 PlayerDeltaY,
         real32 MinCornerY, real32 MaxCornerY) {
  
  bool32 Hit = false;
  real32 tEpsilon = 0.001f;
  if(PlayerDeltaX != 0.0f) {
    
    real32 tResult = (WallC - RelX) / PlayerDeltaX;
    real32 Y = RelY + tResult*PlayerDeltaY;
    
    if((tResult >= 0) && (*tMin > tResult)) {
      if((Y >= MinCornerY) && (Y <= MaxCornerY)) {
        
        *tMin = Max(0.0f, tResult - tEpsilon); 
        Hit = true;
      }
    }
  }
  
  return Hit;
}


internal void
ClearCollisionRulesFor(game_state *GameState, uint32 StorageIndex) {
  
  for(uint32 HashBucket = 0;
      HashBucket < ArrayCount(GameState->CollisionRuleHash); ++HashBucket) {
    
    for(pairwise_collision_rule **Rule = &GameState->CollisionRuleHash[HashBucket];
        *Rule;) {
      
      if((*Rule)->StorageIndexA == StorageIndex ||
         (*Rule)->StorageIndexB == StorageIndex) {
        
        pairwise_collision_rule *Removed = *Rule;
        *Rule = (*Rule)->NextInHash;
        
        // Put thing on a free list
        Removed->NextInHash = GameState->FirstFreeCollisionRule;
        GameState->FirstFreeCollisionRule = Removed;
      }
      else {
        Rule = &(*Rule)->NextInHash;
      }
    }
  }
}


internal void
AddCollisionRule(game_state *GameState, uint32 StorageIndexA, uint32 StorageIndexB, bool32 CanCollide) {
  
  if(StorageIndexA > StorageIndexB) {
    
    uint32 Temp = StorageIndexA;
    StorageIndexA = StorageIndexB;
    StorageIndexB = Temp;
  }
  
  pairwise_collision_rule *Found = 0;
  uint32 HashBucket = StorageIndexA & (ArrayCount(GameState->CollisionRuleHash) - 1);
  for(pairwise_collision_rule *Rule = GameState->CollisionRuleHash[HashBucket];
      Rule;
      Rule = Rule->NextInHash) {
    
    if((Rule->StorageIndexA == StorageIndexA) &&
       (Rule->StorageIndexB == StorageIndexB)) {
      
      Found = Rule;
      break;
    }
  }
  
  if(!Found) {
    
    Found = GameState->FirstFreeCollisionRule;
    
    if(Found) {
      
      GameState->FirstFreeCollisionRule = Found->NextInHash;
    }
    else {
      
      Found = PushStruct(&GameState->WorldArena, pairwise_collision_rule);
    }
    
    Found->NextInHash = GameState->CollisionRuleHash[HashBucket];
    GameState->CollisionRuleHash[HashBucket] = Found;
    
  }
  
  if(Found) {
    
    Found->StorageIndexA = StorageIndexA;
    Found->StorageIndexB = StorageIndexB;
    Found->CanCollide = CanCollide;
  }
}

internal bool32
CanCollide(game_state *GameState, sim_entity *A, sim_entity *B) {
  
  bool32 Result = false;
  
  if(A != B) {
    
    if(A->StorageIndex > B->StorageIndex) {
      
      sim_entity *Temp = A;
      A = B;
      B = Temp;
    }
    
    if(IsSet(A, EntityFlag_Collides) && IsSet(B, EntityFlag_Collides)) {
      
      if(!IsSet(A, EntityFlag_NonSpatial) &&
         !IsSet(B , EntityFlag_NonSpatial)) {
        
        // TODO(Egor): property-based logic should be here
        Result = true;
      }
      
      uint32 HashBucket = A->StorageIndex & (ArrayCount(GameState->CollisionRuleHash) - 1);
      for(pairwise_collision_rule *Rule = GameState->CollisionRuleHash[HashBucket];
          Rule;
          Rule = Rule->NextInHash) {
        
        if((Rule->StorageIndexA == A->StorageIndex) &&
           (Rule->StorageIndexB == B->StorageIndex)) {
          
          Result = Rule->CanCollide;
          break;
        }
      }
    }
  }
  
  return Result;
}


internal bool32
HandleCollision(game_state *GameState, sim_entity *A, sim_entity* B) {
  
  bool32 StopsOnCollision = false;
  
  if(A->Type == EntityType_Sword) {
    
    AddCollisionRule(GameState, A->StorageIndex, B->StorageIndex, false);
    StopsOnCollision = false;
  }
  else {
    
    StopsOnCollision = true;
  }
  
  if(A->Type > B->Type) {
    
    sim_entity *Temp = A;
    A = B;
    B = Temp;
  }
  
  if((A->Type == EntityType_Monster)   &&
     (B->Type == EntityType_Sword)) {
    
    --A->HitPointMax;
  }
  
  if((A->Type == EntityType_Hero) &&
     (B->Type == EntityType_Stairwell)) {
    
    // AddCollisionRule(GameState, A->StorageIndex, B->StorageIndex, false);
    // StopsOnCollision = false;
  }
  
  return StopsOnCollision;
}

internal bool32 
CanOverlap(game_state *GameState, sim_entity *Mover, sim_entity *Region) {
  
  bool32 Result = false;
  if(Mover != Region) {
    if(Region->Type == EntityType_Stairwell) {
      
      Result = true;
    }
  }
  
  return Result;
}


internal void
HandleOverlap(game_state *GameState, sim_entity *Mover,
              sim_entity *Region, real32 dt, real32 *Ground) {
  
  if(Region->Type == EntityType_Stairwell) {
    
    *Ground = GetStairGround(Region, GetEntityGroundPoint(Mover));
  }
}


internal bool32
SpeculativeCollide(sim_entity *Mover, sim_entity *Region, v3 TestP) {
  
  bool32 Result = true;
  if(Region->Type == EntityType_Stairwell) {
    
    v3 MoverGroundPoint = GetEntityGroundPoint(Mover, TestP);
    real32 StepHeight = 0.1f;
    
    real32 Ground = GetStairGround(Region, MoverGroundPoint);
    Result = (AbsoluteValue(MoverGroundPoint.z - Ground) > StepHeight);
  }
  
  return Result;
}

struct test_wall {
  
  v3 Normal;
  real32 WallC;
  real32 RelX;
  real32 RelY; 
  real32 DeltaX;
  real32 DeltaY;
  real32 MinY;
  real32 MaxY;
};

internal bool32
EntitiesOverlap(sim_entity *Entity, sim_entity *TestEntity, v3 Epsilon = V3(0, 0, 0)) {
  
  bool32 Result = false;
  for(uint32 EntityVolumeIndex = 0;
      !Result && (EntityVolumeIndex < Entity->Collision->VolumeCount);
      ++EntityVolumeIndex) {
    
    sim_entity_collision_volume *Volume =
      Entity->Collision->Volumes + EntityVolumeIndex;
    
    // NOTE(Egor): TestVolumeIndex = TestEntityVolumeIndex
    for(uint32 TestVolumeIndex = 0;
        !Result && (TestVolumeIndex < TestEntity->Collision->VolumeCount);
        ++TestVolumeIndex) {
      
      sim_entity_collision_volume *TestVolume = TestEntity->Collision->Volumes + TestVolumeIndex;
      // check multiple volumes after this
      
      rectangle3 EntityRect = RectCenterDim(Entity->P + Volume->OffsetP, Volume->Dim + Epsilon);
      rectangle3 TestEntityRect = RectCenterDim(TestEntity->P + TestVolume->OffsetP, TestVolume->Dim);
      Result = RectIntersect(EntityRect, TestEntityRect);
    }
  }
  
  return Result;
}


internal void
MoveEntity(game_state *GameState, sim_region *SimRegion, sim_entity *Entity,
           real32 dT, move_spec *MoveSpec, v3 ddP) {
  
  // NOTE(Egor): if entity is nonspatial we shouldn't move it, have no meaning
  Assert(!IsSet(Entity, EntityFlag_NonSpatial));
  
  world *World = SimRegion->World;
  
  if(MoveSpec->UnitMaxAccelVector) {
    
    real32 ddLengthSq = LengthSq(ddP);
    
    if(ddLengthSq > 1.0f) {
      
      ddP *= (1.0f/SquareRoot(ddLengthSq));
    }
  }
  
  real32 PlayerSpeed = MoveSpec->Speed;
  
  ddP *= PlayerSpeed;
  
  ddP += -Entity->dP*MoveSpec->Drag;
  
  if(!IsSet(Entity, EntityFlag_ZSupported)) {
    // NOTE(Egor): this is gravity
    ddP += V3(0,0, -9.8f);
  }
  
  v3 OldPlayerP = Entity->P;
  v3 PlayerDelta = (0.5f * ddP * Square(dT) + Entity->dP*dT);
  Entity->dP = ddP*dT + Entity->dP;
  Assert(LengthSq(Entity->dP) <= Square(SimRegion->MaxEntityVelocity));
  
  real32 DistanceRemaining = Entity->DistanceLimit;
  if(DistanceRemaining == 0.0f) {
    
    DistanceRemaining = 10000.0f;
  }
  
  
  // NOTE(Egor): collision test iterations
  for(uint32 Iteration = 0; (Iteration < 4); ++Iteration) {
    
    real32 PlayerDeltaLength = Length(PlayerDelta);
    // NOTE(Egor): construction solid geometry
    real32 tMin = 1.0f;
    real32 tMax = 0.0f;
    
    // TODO(Egor): What do we want to do for epsilon
    if(PlayerDeltaLength > 0.0f ) {
      
      if(PlayerDeltaLength > DistanceRemaining) {
        // NOTE(Egor): if we not having enough moving resource, we cap our
        // movement distance
        tMin = DistanceRemaining/ PlayerDeltaLength;
      }
      
      v3 WallNormalMin = {};
      v3 WallNormalMax = {};
      sim_entity *HitEntityMax = 0;
      sim_entity *HitEntityMin = 0;
      
      v3 DesiredPosition = Entity->P + PlayerDelta;
      
      // NOTE(Egor): check if Entity Collides and Spatial
      if(!IsSet(Entity, EntityFlag_NonSpatial)) {
        for(uint32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex) {
          
          sim_entity *TestEntity = SimRegion->Entities + TestEntityIndex;
          if(TestEntity != Entity) {
            
            real32 OverlapEpsion = 0.001f;
            bool32 Collides = ((IsSet(TestEntity, EntityFlag_Traversable) &&
                                EntitiesOverlap(Entity, TestEntity, V3(1.0f, 1.0f, 1.0f)*OverlapEpsion)) ||
                               CanCollide(GameState, Entity, TestEntity));
            if(Collides) {
              
              for(uint32 EntityVolumeIndex = 0;
                  EntityVolumeIndex < Entity->Collision->VolumeCount;
                  ++EntityVolumeIndex) {
                
                sim_entity_collision_volume *Volume =
                  Entity->Collision->Volumes + EntityVolumeIndex;
                
                // NOTE(Egor): TestVolumeIndex = TestEntityVolumeIndex
                for(uint32 TestVolumeIndex = 0;
                    TestVolumeIndex < TestEntity->Collision->VolumeCount;
                    ++TestVolumeIndex) {
                  
                  sim_entity_collision_volume *TestVolume =
                    TestEntity->Collision->Volumes + TestVolumeIndex;
                  
                  
                  // TODO(Egor): idk how much it will impair perfomance, but maybe
                  // I should fix my indentation module, and use generic { }
                  v3 MinkowskiDiameter = V3(TestVolume->Dim.x + Volume->Dim.x,
                                            TestVolume->Dim.y + Volume->Dim.y,
                                            TestVolume->Dim.z + Volume->Dim.z);
                  
                  v3 MinCorner = -0.5f*MinkowskiDiameter;
                  v3 MaxCorner = 0.5f*MinkowskiDiameter;
                  v3 Rel = ((Entity->P + Volume->OffsetP) -
                            (TestEntity->P + TestVolume->OffsetP));
                  
                  if((Rel.z >= MinCorner.z) &&
                     (Rel.z < MaxCorner.z)) {
                    
                    // TODO(Egor): this is junky, get rid of this or make sane speculative
                    // collision check
                    
                    test_wall Walls[] = {
                      { V3(1.0f, 0.0f, 0.0f), MaxCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y,
                        MinCorner.y, MaxCorner.y },
                      { V3(-1.0f, 0.0f, 0.0f), MinCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y,
                        MinCorner.y, MaxCorner.y },
                      { V3(0.0f, 1.0f, 0.0f), MaxCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x,
                        MinCorner.x, MaxCorner.x },
                      { V3(0.0f, -1.0f, 0.0f), MinCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x,
                        MinCorner.x, MaxCorner.x }
                    };
                    
                    // NOTE(Egor): we determine if we are handling maximum distance test
                    // for case when we want to move form one space_entity to another
                    if(IsSet(TestEntity, EntityFlag_Traversable)) {
                      
                      real32 tMaxTest = tMax;
                      bool32 HitThis = false;
                      
                      v3 TestWallNormal = {};
                      
                      for(uint32 WallIndex = 0; WallIndex < ArrayCount(Walls); ++WallIndex) {
                        
                        test_wall *Wall = Walls + WallIndex;
                        
                        real32 tEpsilon = 0.001f;
                        if(Wall->DeltaX != 0.0f) {
                          
                          real32 tResult = (Wall->WallC - Wall->RelX) / Wall->DeltaX;
                          real32 Y = Wall->RelY + tResult * Wall->DeltaY;
                          
                          if((tResult >= 0.0f) && (tMaxTest < tResult)) {
                            
                            if((Y >= Wall->MinY) && (Y < Wall->MaxY)) {
                              
                              tMaxTest = Max(0.0f, tResult - tEpsilon); 
                              TestWallNormal = Wall->Normal;
                              HitThis = true;
                            }
                          }
                        }
                      } // for()
                      if(HitThis) {
                        
                        tMax = tMaxTest;
                        WallNormalMax = TestWallNormal;
                        HitEntityMax = TestEntity;
                        
                      }// if()
                    } // if()
                    else {
                      
                      real32 tMinTest = tMin;
                      bool32 HitThis = false;
                      
                      v3 TestWallNormal = {};
                      
                      for(uint32 WallIndex = 0; WallIndex < ArrayCount(Walls); ++WallIndex) {
                        
                        test_wall *Wall = Walls + WallIndex;
                        
                        real32 tEpsilon = 0.001f;
                        if(Wall->DeltaX != 0.0f) {
                          
                          real32 tResult = (Wall->WallC - Wall->RelX) / Wall->DeltaX;
                          real32 Y = Wall->RelY + tResult * Wall->DeltaY;
                          
                          if((tResult >= 0) && (tMinTest > tResult)) {
                            if((Y >= Wall->MinY) && (Y <= Wall->MaxY)) {
                              
                              tMinTest = Max(0.0f, tResult - tEpsilon); 
                              TestWallNormal = Wall->Normal;
                              HitThis = true;
                            }
                          }
                        }
                      } // for()
                      if(HitThis) {
                        
                        v3 TestP = Entity->P + PlayerDelta*tMinTest;
                        if(SpeculativeCollide(Entity, TestEntity, TestP)) {
                          
                          tMin = tMinTest;
                          WallNormalMin = TestWallNormal;
                          HitEntityMin = TestEntity;
                        }
                      }
                    } // else
                  }
                }
              }
            }
          }
        }
      }
      
      real32 tStop;
      sim_entity *HitEntity = 0;
      v3 WallNormal = {};
      if(tMin < tMax) {
        
        tStop = tMin;
        HitEntity = HitEntityMin;
        WallNormal = WallNormalMin;
      }
      else {
        
        tStop = tMax;
        HitEntity = HitEntityMax;
        WallNormal = WallNormalMax;
      }
      
      Entity->P += tStop*PlayerDelta;   
      // NOTE(Egor): update our movement resource
      DistanceRemaining -= tStop*PlayerDeltaLength;
      
      if(HitEntity) {
        bool32 StopsOnCollision = HandleCollision(GameState, Entity, HitEntity);
        
        // NOTE(Egor): reflecting vector calculation
        PlayerDelta = DesiredPosition - Entity->P; 
        if(StopsOnCollision) {
          
          PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
          Entity->dP = Entity->dP - 1*Inner(Entity->dP, WallNormal)*WallNormal;
        }
      }
      else {
        
        break;
      }
    }
    else {
      
      break;
    }
  }
  
  real32 Ground = 0.0f;
  
#if 1
  // NOTE(Egor): testing if entities overlaps
  {
    for(uint32 TestEntityIndex = 0;
        TestEntityIndex < SimRegion->EntityCount;
        ++TestEntityIndex) {
      
      sim_entity *TestEntity = SimRegion->Entities + TestEntityIndex;
      if(CanOverlap(GameState, Entity, TestEntity)) {
        
        if(EntitiesOverlap(Entity, TestEntity)) {
          HandleOverlap(GameState, Entity, TestEntity, dT, &Ground);
        }
      }
    }
  }
#endif
  
  // ---------------------------------------------------
  
  
  // NOTE(Egor): right now this is ground under the abstract entity location 
  // (half Dim.z abot the ground under the nominal foot of the player)
  Ground += Entity->P.z - GetEntityGroundPoint(Entity).z;
  if((Entity->P.z <= Ground) || 
     (IsSet(Entity, EntityFlag_ZSupported) && Entity->dP.z == 0.0f)) {
    
    Entity->P.z = Ground;
    Entity->dP.z = 0;
    AddFlag(Entity, EntityFlag_ZSupported);
  }
  else {
    
    ClearFlag(Entity, EntityFlag_ZSupported);
  }
  
  if(Entity->DistanceLimit != 0.0f) {
    
    Entity->DistanceLimit = DistanceRemaining;
  }
  
  // NOTE(Egor): updating facing directions
  if(Entity->dP.x != 0 || Entity->dP.y != 0) {
    if(AbsoluteValue(Entity->dP.x) > AbsoluteValue(Entity->dP.y)) {
      
      if(Entity->dP.x > 0) {
        
        Entity->FacingDirection = 0;        
      }
      else {
        
        Entity->FacingDirection = 2;      
      }
    }
    else if(AbsoluteValue(Entity->dP.x) < AbsoluteValue(Entity->dP.y)) {
      
      if(Entity->dP.y > 0) {
        Entity->FacingDirection = 1;
      }
      else {
        
        Entity->FacingDirection = 3;
      }
    }
  }
}
