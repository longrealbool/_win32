inline move_spec
DefaultMoveSpec() {
  
  move_spec Result;
  Result.Speed = 1.0f;
  Result.Drag = 0.0f;
  Result.UnitMaxAccelVector = false;
  
  return Result;
}


inline void UpdateFamiliar(sim_region *SimRegion, sim_entity *Entity, real32 dt) {
  
  sim_entity *ClosestHero = 0;
  real32 ClosestHeroDSq = Square(10.0f);
  for(uint32 EntityIndex = 1; EntityIndex < SimRegion->EntityCount; ++EntityIndex) {
    
    sim_entity *TestEntity = SimRegion->Entities + EntityIndex;
    if(TestEntity->Type == EntityType_Hero) {
      
      real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
      
      if(ClosestHeroDSq > TestDSq && TestDSq > Square(3.0f)) {
        
        ClosestHero = TestEntity;
        ClosestHeroDSq = TestDSq;
      }
      
    }
    
  }
  v2 ddP = {};
  if(ClosestHero) {
    
    real32 Acceleration = 0.5f;
    real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
    ddP = (ClosestHero->P - Entity->P) * OneOverLength;
  }
  
  move_spec MoveSpec;
  MoveSpec.Speed = 50.0f;
  MoveSpec.Drag = 8.0f;
  MoveSpec.UnitMaxAccelVector = true;
  MoveEntity(SimRegion, Entity, dt, &MoveSpec, ddP); 
}



inline void 
UpdateMonster(sim_region *SimRegion, sim_entity *Entity, real32 dt) {
  
}

inline void
UpdateSword(sim_region *SimRegion, sim_entity *Entity, real32 dt) {
  
  move_spec MoveSpec;
  MoveSpec.Speed = 0.0f;
  MoveSpec.Drag = 0.0f;
  MoveSpec.UnitMaxAccelVector = true;
  
  v2 OldP = Entity->P;
  MoveEntity(SimRegion, Entity, dt, &MoveSpec, V2(0, 0));
  real32 DistanceTravelled = Length(Entity->P - OldP);
  
  Entity->DistanceRemaining -= DistanceTravelled;
  if(Entity->DistanceRemaining <= 0.0f) {
    
    //Assert(!"Need to make entities not to be there");
    
  }
  
  
}


