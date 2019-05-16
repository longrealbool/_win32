
#define TILE_CHUNK_SAFE_MARGIN 256  


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
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ,
             memory_arena *Arena = 0) {
  
  Assert(TileChunkX > TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkY > TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkZ > TILE_CHUNK_SAFE_MARGIN);
  
  Assert(TileChunkX < UINT32_MAX - TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkY < UINT32_MAX - TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkZ < UINT32_MAX - TILE_CHUNK_SAFE_MARGIN);
         
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
      // WARNING: POTENTIAL ERROR
      //Chunk->TileChunkX = 0; 
      Chunk = Chunk->NextInHash;
    }
    
    // TODO(Egor): check if I garanteed that newly allocated tile_chunk will be zeroed
    // NOTE(Egor): initialize initial or newly created chained chunk
    if(Arena && Chunk->TileChunkX == 0) {
      
      Chunk->TileChunkX = TileChunkX;
      Chunk->TileChunkY = TileChunkY;
      Chunk->TileChunkZ = TileChunkZ;
      
      uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
      Chunk->Tiles = PushArray(Arena, TileCount, uint32);
      for(uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex) {
        
        Chunk->Tiles[TileIndex] = 1;
      }
      
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

inline uint32
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY) {
  Assert(TileMap);
  Assert( (TileX >= 0) && (TileX < TileMap->ChunkDim) &&
         (TileY >= 0) && (TileY < TileMap->ChunkDim));
  
  uint32 TileMapValue = TileChunk->Tiles[TileMap->ChunkDim*TileY + TileX];
  return TileMapValue;
}




internal uint32
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY) {
  
  uint32 Result = 0;
  if(TileChunk && TileChunk->Tiles) {
    Result = GetTileValueUnchecked(TileMap, TileChunk, TileX, TileY);
  }
  
  return Result;
}


internal uint32
GetTileValue(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  tile_chunk_position ChunkPos = GetTileChunkPos(TileMap, AbsTileX, AbsTileY, AbsTileZ); 
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX,
                                       ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
  uint32 TileValue = GetTileValue(TileMap, TileChunk, ChunkPos.TileX, ChunkPos.TileY);
  
  return TileValue;
}

internal uint32
GetTileValue(tile_map* TileMap, tile_map_position *Pos) {
  
  uint32 TileValue = GetTileValue(TileMap, Pos->AbsTileX, Pos->AbsTileY, Pos->AbsTileZ);
  
  return TileValue;
}



internal void
SetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY, uint32 TileValue) {
  
  if(TileChunk && TileChunk->Tiles) {
    TileChunk->Tiles[TileMap->ChunkDim*TileY + TileX] = TileValue;
  }
}

internal void 
SetTileValue(memory_arena *Arena, tile_map* TileMap,
             uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue) {
  
  tile_chunk_position ChunkPos = GetTileChunkPos(TileMap, AbsTileX, AbsTileY, AbsTileZ); 
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY,
                                       ChunkPos.TileChunkZ, Arena);
  SetTileValue(TileMap, TileChunk, ChunkPos.TileX, ChunkPos.TileY, TileValue);
}


internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position *TestPos) {
  
  bool32 IsEmpty  = false;
  uint32 TileValue = GetTileValue(TileMap, TestPos->AbsTileX, TestPos->AbsTileY, TestPos->AbsTileZ);
  IsEmpty = (TileValue == 1 ||
             TileValue == 4 ||
             TileValue == 5);
  if(!IsEmpty) {
    tile_chunk_position ChunkPos = GetTileChunkPos(TileMap, TestPos->AbsTileX, TestPos->AbsTileY, TestPos->AbsTileZ); 
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX,
                                         ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
    SetTileValue(TileMap, TileChunk, ChunkPos.TileX, ChunkPos.TileY, 3);
  }
  
  return IsEmpty;
}

internal bool32
IsTileValueEmpty(uint32 TileValue) {
  
  bool32 IsEmpty = (TileValue == 1 ||
                    TileValue == 4 ||
                    TileValue == 5);
  
  return IsEmpty;
  
}

inline bool32
IsTileMapTileEmpty(tile_map *TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY) {
  
  bool32 IsEmpty  = false;
  
  if(TileChunk) {
    // NOTE(egor): we check a tile value inside tile chunk of sixe 256x256
    uint32 TileMapValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
    IsEmpty = IsTileValueEmpty(TileMapValue);
    
  }
  
  return IsEmpty;
}

////
// TODO(Egor): maybe I should transfer this into another file

inline void
RecanonicalizeCoord(tile_map *TileMap, uint32 *Tile, real32 *TileRel) {
  
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
