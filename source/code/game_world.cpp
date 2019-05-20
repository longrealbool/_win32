// TODO(Egor): I need to think about what SAFE_MARGIN is
#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX / 16) * 4


internal void
InitializeTileMap(tile_map *TileMap, real32 TileSideInMeters) {
  
  TileMap->ChunkShift = 4;
  TileMap->ChunkDim = (1 << TileMap->ChunkShift);
  TileMap->ChunkMask = TileMap->ChunkDim - 1;
  
  TileMap->TileSideInMeters = TileSideInMeters;
  
  for(uint32 TileChunkIndex = 0;
      TileChunkIndex < ArrayCount(TileMap->TileChunkHash); ++TileChunkIndex) {
    
    TileMap->TileChunkHash[TileChunkIndex].TileChunkX = 0;
    
  }
}

inline tile_chunk *
GetTileChunk(tile_map *TileMap, int32 TileChunkX, int32 TileChunkY, int32 TileChunkZ,
             memory_arena *Arena = 0) {
  
  Assert(TileChunkX > -TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkY > -TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkZ > -TILE_CHUNK_SAFE_MARGIN);
  
  Assert(TileChunkX < TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkY < TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkZ < TILE_CHUNK_SAFE_MARGIN);
         
  // TODO(Egor): make a better hash function, lol
  uint32 HashValue = 19*TileChunkX + 7*TileChunkY + 3*TileChunkZ;
  uint32 HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);
  Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));
  
  tile_chunk *Chunk = TileMap->TileChunkHash + HashSlot;
  do {
    
    if(Chunk->TileChunkX == TileChunkX &&
       Chunk->TileChunkY == TileChunkY &&
       Chunk->TileChunkZ == TileChunkZ) {
      
      break;
    } 

    // NOTE(Egor): if our initial slot is initialized, and there isn't chained chunk
    if(Arena && (Chunk->TileChunkX != 0 && (!Chunk->NextInHash))) {
      
      Chunk->NextInHash = PushStruct(Arena, tile_chunk);
      Chunk = Chunk->NextInHash;
      Chunk->TileChunkX = 0; 
    }
    
    // TODO(Egor): check if I garanteed that newly allocated tile_chunk will be zeroed
    // NOTE(Egor): initialize initial or newly created chained chunk
    if(Arena && Chunk->TileChunkX == 0) {
      
      Chunk->TileChunkX = TileChunkX;
      Chunk->TileChunkY = TileChunkY;
      Chunk->TileChunkZ = TileChunkZ;

      Chunk->NextInHash = 0;
      
      break;
    }
    
    Chunk = Chunk->NextInHash;
  }while(Chunk);
  
  return Chunk;
}

inline tile_chunk_position
GetTileChunkPos(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  tile_chunk_position Result;
  
  Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
  Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
  Result.TileChunkZ = AbsTileZ;
  
  Result.TileX = AbsTileX & TileMap->ChunkMask;
  Result.TileY = AbsTileY & TileMap->ChunkMask;
  
  return Result;
  
}


////
// TODO(Egor): maybe I should transfer this into another file

inline void
RecanonicalizeCoord(tile_map *TileMap, int32 *Tile, real32 *TileRel) {
  
  real32 RelCoord = *TileRel; // DEBUG
  
  int32 Offset = RoundReal32ToInt32(*TileRel /TileMap->TileSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset*TileMap->TileSideInMeters;
  
  // NOTE: (Egor) the tile_map is toroidal, we allow overflow and underflow 
  // in a natural C uint32 way
  Assert(*Tile >= 0);
  
  Assert(*TileRel >= -TileMap->TileSideInMeters*0.5f);
  Assert(*TileRel <= TileMap->TileSideInMeters*0.5f);
  
}



// TODO (Egor): we cannot move faster than 1 tile map in on gameLoop
inline tile_map_position
MapIntoTileSpace(tile_map *TileMap, tile_map_position BasePos, v2 Offset) {
  
  tile_map_position Result = BasePos;
  
  Result.Offset_ += Offset;
  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset_.X);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset_.Y);
  
  return Result;
}



inline tile_map_difference 
Subtract(tile_map *TileMap, tile_map_position *A, tile_map_position *B) {
  
  tile_map_difference Result;
  
  v2 dTileXY = V2((real32)A->AbsTileX - (real32)B->AbsTileX,
                  (real32)A->AbsTileY - (real32)B->AbsTileY);
  
  real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;
  
  Result.dXY = TileMap->TileSideInMeters*dTileXY + (A->Offset_ - B->Offset_);
  // NOTE(Egor): Z is not a real coordinate right now
  Result.dZ = TileMap->TileSideInMeters*dTileZ;
  
  return Result;
}

inline bool32
AreOnTheSameTile(tile_map_position *A, tile_map_position *B) {
  
  bool32 Result = (A->AbsTileX == B->AbsTileX &&
                   A->AbsTileY == B->AbsTileY &&
                   A->AbsTileZ == B->AbsTileZ);
  
  return Result;
  
}

inline tile_map_position
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  tile_map_position Result = {};
  
  Result.AbsTileX = AbsTileX;
  Result.AbsTileY = AbsTileY;
  Result.AbsTileZ = AbsTileZ;
  
  return Result;
  
  
}
