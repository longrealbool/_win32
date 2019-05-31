// TODO(Egor): I need to think about what SAFE_MARGIN is
#define CHUNK_SAFE_MARGIN (INT32_MAX / 16) * 4
#define CHUNK_UNITIALIZED INT32_MAX


internal void
InitializeWorld(world *World, real32 TileSideInMeters) {
  
  World->ChunkShift = 4;
  World->ChunkDim = (1 << World->ChunkShift);
  World->ChunkMask = World->ChunkDim - 1;
  
  World->TileSideInMeters = TileSideInMeters;
  
  for(uint32 ChunkIndex = 0;
      ChunkIndex < ArrayCount(World->ChunkHash); ++ChunkIndex) {
    
    World->ChunkHash[ChunkIndex].ChunkX = 0;
    
  }
}

inline world_chunk *
GetChunk(world *TileMap, int32 ChunkX, int32 ChunkY, int32 ChunkZ,
             memory_arena *Arena = 0) {
  
  Assert(ChunkX > -CHUNK_SAFE_MARGIN);
  Assert(ChunkY > -CHUNK_SAFE_MARGIN);
  Assert(ChunkZ > -CHUNK_SAFE_MARGIN);
  
  Assert(ChunkX < CHUNK_SAFE_MARGIN);
  Assert(ChunkY < CHUNK_SAFE_MARGIN);
  Assert(ChunkZ < CHUNK_SAFE_MARGIN);
         
  // TODO(Egor): make a better hash function, lol
  uint32 HashValue = 113*ChunkX + 7*ChunkY + 3*ChunkZ;
  uint32 HashSlot = HashValue & (ArrayCount(TileMap->ChunkHash) - 1);
  Assert(HashSlot < ArrayCount(TileMap->ChunkHash));
  
  world_chunk *Chunk = TileMap->ChunkHash + HashSlot;
  do {
    
    if(Chunk->ChunkX == ChunkX &&
       Chunk->ChunkY == ChunkY &&
       Chunk->ChunkZ == ChunkZ) {
      
      break;
    } 

    // NOTE(Egor): if our initial slot is initialized, and there isn't chained chunk
    if(Arena && (Chunk->ChunkX != 0 && (!Chunk->NextInHash))) {
      
      Chunk->NextInHash = PushStruct(Arena, world_chunk);
      Chunk = Chunk->NextInHash;
      Chunk->ChunkX = CHUNK_UNITIALIZED; 
    }
    
    // TODO(Egor): check if I garanteed that newly allocated world_chunk will be zeroed
    // NOTE(Egor): initialize initial or newly created chained chunk
    if(Arena && Chunk->ChunkX == CHUNK_UNITIALIZED) {
      
      Chunk->ChunkX = ChunkX;
      Chunk->ChunkY = ChunkY;
      Chunk->ChunkZ = ChunkZ;

      Chunk->NextInHash = 0;
      
      break;
    }
    
    Chunk = Chunk->NextInHash;
  }while(Chunk);
  
  return Chunk;
}

#if 0
inline world_chunk_position
GetChunkPos(world *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_chunk_position Result;
  
  Result.ChunkX = AbsTileX >> TileMap->ChunkShift;
  Result.ChunkY = AbsTileY >> TileMap->ChunkShift;
  Result.ChunkZ = AbsTileZ;
  
  Result.TileX = AbsTileX & TileMap->ChunkMask;
  Result.TileY = AbsTileY & TileMap->ChunkMask;
  
  return Result;
  
}
#endif


////
// TODO(Egor): maybe I should transfer this into another file

inline void
RecanonicalizeCoord(world *TileMap, int32 *Tile, real32 *TileRel) {
  
  real32 RelCoord = *TileRel; // DEBUG
  
  int32 Offset = RoundReal32ToInt32(*TileRel /TileMap->TileSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset*TileMap->TileSideInMeters;
  
  // NOTE: (Egor) the world is toroidal, we allow overflow and underflow 
  // in a natural C uint32 way
  Assert(*Tile >= 0);
  
  Assert(*TileRel >= -TileMap->TileSideInMeters*0.5f);
  Assert(*TileRel <= TileMap->TileSideInMeters*0.5f);
  
}



// TODO (Egor): we cannot move faster than 1 tile map in on gameLoop
inline world_position
MapIntoTileSpace(world *TileMap, world_position BasePos, v2 Offset) {
  
  world_position Result = BasePos;
  
  Result.Offset_ += Offset;
  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset_.X);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset_.Y);
  
  return Result;
}



inline world_difference 
Subtract(world *World, world_position *A, world_position *B) {
  
  world_difference Result;
  
  v2 dTileXY = V2((real32)A->AbsTileX - (real32)B->AbsTileX,
                  (real32)A->AbsTileY - (real32)B->AbsTileY);
  
  real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;
  
  Result.dXY = World->TileSideInMeters*dTileXY + (A->Offset_ - B->Offset_);
  // NOTE(Egor): Z is not a real coordinate right now
  Result.dZ = World->TileSideInMeters*dTileZ;
  
  return Result;
}

inline bool32
AreOnTheSameTile(world_position *A, world_position *B) {
  
  bool32 Result = (A->AbsTileX == B->AbsTileX &&
                   A->AbsTileY == B->AbsTileY &&
                   A->AbsTileZ == B->AbsTileZ);
  
  return Result;
  
}

inline world_position
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_position Result = {};
  
  Result.AbsTileX = AbsTileX;
  Result.AbsTileY = AbsTileY;
  Result.AbsTileZ = AbsTileZ;
  
  return Result;
  
  
}
