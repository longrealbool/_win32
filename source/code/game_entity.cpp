
inline void 
UpdateMonster(game_state *GameState, entity Entity, real32 dt) {
  
}

inline void
UpdateSword(game_state *GameState, entity Entity, real32 dt) {
  
  move_spec MoveSpec;
  MoveSpec.Speed = 0.0f;
  MoveSpec.Drag = 0.0f;
  MoveSpec.UnitMaxAccelVector = true;
  
  v2 OldP = Entity.High->P;
  MoveEntity(GameState, Entity, dt, &MoveSpec, V2(0, 0));
  real32 DistanceTravelled = Length(Entity.High->P - OldP);
  
  Entity.Low->Sim.DistanceRemaining -= DistanceTravelled;
  if(Entity.Low->Sim.DistanceRemaining <= 0.0f) {
    
    ChangeEntityLocation(&GameState->WorldArena, GameState->World,
                         Entity.LowIndex, Entity.Low, &Entity.Low->Sim.P, 0);
  }
  
  
}

inline void UpdateFamiliar(game_state *GameState, entity Entity, real32 dt) {
  
  entity ClosestHero = {};
  real32 ClosestHeroDSq = Square(10.0f);
  for(uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex) {
    
    entity TestEntity = EntityFromHighIndex(GameState, HighEntityIndex);
    if(TestEntity.Low->Sim.Type == EntityType_Hero) {
      
      real32 TestDSq = LengthSq(TestEntity.High->P - Entity.High->P);
      if(ClosestHeroDSq > TestDSq && TestDSq > Square(3.0f)) {
        
        ClosestHero = TestEntity;
        ClosestHeroDSq = TestDSq;
      }
      
    }
    
  }
  v2 ddP = {};
  if(ClosestHero.High) {
    
    real32 Acceleration = 0.5f;
    real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
    ddP = (ClosestHero.High->P - Entity.High->P) * OneOverLength;
  }
  
  move_spec MoveSpec;
  MoveSpec.Speed = 50.0f;
  MoveSpec.Drag = 8.0f;
  MoveSpec.UnitMaxAccelVector = true;
  MoveEntity(GameState, Entity, dt, &MoveSpec, ddP); 
}

