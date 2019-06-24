internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source, v2 *SimP);

inline v2
GetSimSpaceP(sim_region *SimRegion, low_entity *Stored) {
  
  v2 Result = INVALID_P;
  if(!IsSet(&Stored->Sim, EntityFlag_NonSpatial)) {
    
    world_difference Diff = Subtract(SimRegion->World,
                                     &Stored->P, &SimRegion->Origin);
    Result = Diff.dXY;
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
      v2 SimP = GetSimSpaceP(SimRegion, Low);
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
AddEntity(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source, v2 *SimP) {
  
  sim_entity *Dest = AddEntityRaw(GameState, SimRegion, StorageIndex, Source);
  
  if(Dest) {
    
    if(SimP) {
      
      Dest->P = *SimP;
      Dest->Updatable = IsInRectangle(SimRegion->UpdateBounds, Dest->P);
    }
    else {
      Dest->P = GetSimSpaceP(SimRegion, Source);
    }
  }
  
  return Dest;
  
}


internal sim_region *
BeginSim(memory_arena *SimArena, game_state *GameState,  world *World, world_position Origin, rectangle2 Bounds) {
  
  sim_region *SimRegion = PushStruct(SimArena, sim_region);
  ZeroStruct(SimRegion->Hash);
  
  real32 UpdateSafetyMargin = 1.0f;
  
  SimRegion->World = World;
  SimRegion->Origin = Origin;
  SimRegion->UpdateBounds = Bounds;
  SimRegion->Bounds = AddRadiusTo(SimRegion->UpdateBounds,
                                  UpdateSafetyMargin, UpdateSafetyMargin);
  
  SimRegion->MaxEntityCount = 4096;
  SimRegion->EntityCount = 0;
  SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);
  
  
  
  world_position MinChunk = MapIntoChunkSpace(World, SimRegion->Origin,
                                              GetMinCorner(SimRegion->Bounds));
  world_position MaxChunk = MapIntoChunkSpace(World, SimRegion->Origin,
                                              GetMaxCorner(SimRegion->Bounds));
  
  for(int32 ChunkY = MinChunk.ChunkY; ChunkY <= MaxChunk.ChunkY; ++ChunkY) {
    for(int32 ChunkX = MinChunk.ChunkX; ChunkX <= MaxChunk.ChunkX; ++ChunkX) {
      
      world_chunk *Chunk = GetChunk(World, ChunkX, ChunkY, SimRegion->Origin.ChunkZ);
      if(Chunk) {
        
        for(world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next) { 
          for(uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex) {
            
            uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndex];
            low_entity *Low = GameState->LowEntity + LowEntityIndex;
            // NOTE(Egor): if entity is nonspatial we don't need to load it in sim region
            if(!IsSet(&Low->Sim, EntityFlag_NonSpatial)) {
              
              v2 SimSpaceP = GetSimSpaceP(SimRegion, Low);
              if(IsInRectangle(SimRegion->Bounds, SimSpaceP)) {
                
                AddEntity(GameState, SimRegion, LowEntityIndex, Low, &SimSpaceP);
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
      
      
      if(CameraFollowingEntity.High->P.X > (9.0f * World->TileSideInMeters)) {
        NewCameraP.AbsTileX += 17;
      }
      if(CameraFollowingEntity.High->P.X < -(9.0f * World->TileSideInMeters)) {
        NewCameraP.AbsTileX -= 17;
      }
      if(CameraFollowingEntity.High->P.Y > (5.0f * World->TileSideInMeters)) {
        NewCameraP.AbsTileY += 9;
      }
      if(CameraFollowingEntity.High->P.Y < -(5.0f * World->TileSideInMeters)) {
        NewCameraP.AbsTileY -= 9;
      }
      
#else
      NewCameraP = Stored->P;
      
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
MoveEntity(sim_region *SimRegion, sim_entity *Entity, real32 dT, move_spec *MoveSpec, v2 ddP) {
  
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
  
  v2 OldPlayerP = Entity->P;
  v2 PlayerDelta = (0.5f * ddP * Square(dT) +
                    Entity->dP*dT);
  Entity->dP = ddP*dT + Entity->dP;
  
  v2 NewPlayerP = OldPlayerP + PlayerDelta;
  
  real32 ddZ = -9.8f;
  Entity->Z = 0.5f * ddZ * Square(dT) + Entity->dZ*dT + Entity->Z;
  Entity->dZ = ddZ*dT + Entity->dZ;
  
  if(Entity->Z < 0) {
    Entity->Z = 0;
  }
  
  for(uint32 Iteration = 0; (Iteration < 4); ++Iteration) {
    
    v2 WallNormal = {};
    real32 tMin = 1.0f;
    sim_entity *HitEntity = 0;
    
    v2 DesiredPosition = Entity->P + PlayerDelta;
    
    // NOTE(Egor): check if Entity Collides and Spatial
    if(IsSet(Entity, EntityFlag_Collides) &&
       !IsSet(Entity, EntityFlag_NonSpatial)) {
      
      for(uint32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex) {
        
        sim_entity *TestEntity = SimRegion->Entities + TestEntityIndex;
        
        if(Entity != TestEntity) {
          
          // NOTE(Egor): check if Entity Collides and Spatial
          if(IsSet(TestEntity, EntityFlag_Collides) &&
             !IsSet(TestEntity, EntityFlag_NonSpatial)) {
            
            real32 DiameterW = TestEntity->Width + Entity->Width;
            real32 DiameterH = TestEntity->Height + Entity->Height;
            
            v2 MinCorner = -0.5f*v2{DiameterW, DiameterH};
            v2 MaxCorner = 0.5f*v2{DiameterW, DiameterH};
            
            v2 Rel = Entity->P - TestEntity->P;
            
            if(TestWall(&tMin, MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                        MinCorner.Y, MaxCorner.Y)) {
              
              WallNormal = v2{1.0f, 0.0f};
              HitEntity = TestEntity;
            }
            if(TestWall(&tMin, MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                        MinCorner.Y, MaxCorner.Y)) {
              
              WallNormal = v2{-1.0f, 0.0f};
              HitEntity = TestEntity;
            }
            if(TestWall(&tMin, MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                        MinCorner.X, MaxCorner.X)) {
              
              WallNormal = v2{0.0f, 1.0f};
              HitEntity = TestEntity;
            }
            if(TestWall(&tMin, MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                        MinCorner.X, MaxCorner.X)) {
              
              WallNormal = v2{0.0f, -1.0f};
              HitEntity = TestEntity;
            }
          }
        }
      }
    }
    
    
    Entity->P += tMin*PlayerDelta;    
    if(HitEntity) {
      
      // NOTE(Egor): reflecting vector calculation
      PlayerDelta = DesiredPosition - Entity->P; 
      PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
      Entity->dP = Entity->dP - 1*Inner(Entity->dP, WallNormal)*WallNormal;
      
      //      Entity->AbsTileZ += HitLow->dAbsTileZ; 
    }
    else {
      
      break;
    }
    
  }
  
  // NOTE(Egor): updating facing directions
  if(Entity->dP.X != 0 || Entity->dP.Y != 0) {
    if(AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y)) {
      
      if(Entity->dP.X > 0) {
        
        Entity->FacingDirection = 0;        
      }
      else {
        
        Entity->FacingDirection = 2;      
      }
    }
    else if(AbsoluteValue(Entity->dP.X) < AbsoluteValue(Entity->dP.Y)) {
      
      if(Entity->dP.Y > 0) {
        Entity->FacingDirection = 1;
      }
      else {
        
        Entity->FacingDirection = 3;
      }
    }
  }
}
