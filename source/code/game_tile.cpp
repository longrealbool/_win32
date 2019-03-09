


inline tile_chunk *
GetTileChunk(tile_map *TileMap, tile_chunk_position *ChunkPos) {
  //NOTE(egor): get the specific tileChunk (256x256 tiles)
  
  tile_chunk *TileChunk = 0;
  
  if((ChunkPos->TileChunkX < TileMap->TileChunkCountX) &&
     (ChunkPos->TileChunkY < TileMap->TileChunkCountY) &&
     (ChunkPos->TileChunkZ < TileMap->TileChunkCountZ)) {
    
    TileChunk = &TileMap->TileChunks[(ChunkPos->TileChunkZ*TileMap->TileChunkCountY*TileMap->TileChunkCountX +
                                      ChunkPos->TileChunkY*TileMap->TileChunkCountX +
                                      ChunkPos->TileChunkX)];
  }
  return TileChunk;
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
  
  tile_chunk_position TileChunkPos = GetTileChunkPos(TileMap, AbsTileX, AbsTileY, AbsTileZ); 
  tile_chunk *TileChunk = GetTileChunk(TileMap, &TileChunkPos);
  uint32 TileValue = GetTileValue(TileMap, TileChunk, TileChunkPos.TileX, TileChunkPos.TileY);
  
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
  
  tile_chunk_position TileChunkPos = GetTileChunkPos(TileMap, AbsTileX, AbsTileY, AbsTileZ); 
  tile_chunk *TileChunk = GetTileChunk(TileMap, &TileChunkPos);
  
  Assert(TileChunk);
  
  if(!TileChunk->Tiles) {
    
    uint32 NumberOfTilesInTileChunk = TileMap->ChunkDim*TileMap->ChunkDim;
    
    // NOTE(Egor): we had allocated memory for all tilechunks already, 
    // we just need allocate memory for Tiles the particular one, 
    // we want to write right now
    TileChunk->Tiles =
      PushArray(Arena, NumberOfTilesInTileChunk, uint32);
    
    for(uint32 TileIndex = 0; TileIndex < NumberOfTilesInTileChunk; ++TileIndex) {
      TileChunk->Tiles[TileIndex] = 1;
    }
    
  }
  
  SetTileValue(TileMap, TileChunk, TileChunkPos.TileX, TileChunkPos.TileY, TileValue);
}


internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position *TestPos) {
  
  bool32 IsEmpty  = false;
  uint32 TileValue = GetTileValue(TileMap, TestPos->AbsTileX, TestPos->AbsTileY, TestPos->AbsTileZ);
  IsEmpty = (TileValue == 1 ||
             TileValue == 4 ||
             TileValue == 5);
  if(!IsEmpty) {
    tile_chunk_position TileChunkPos = GetTileChunkPos(TileMap, TestPos->AbsTileX, TestPos->AbsTileY, TestPos->AbsTileZ); 
    tile_chunk *TileChunk = GetTileChunk(TileMap, &TileChunkPos);
    SetTileValue(TileMap, TileChunk, TileChunkPos.TileX, TileChunkPos.TileY, 3);
  }
  
  return IsEmpty;
}

inline bool32
IsTileMapTileEmpty(tile_map *TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY) {
  
  bool32 IsEmpty  = false;
  
  if(TileChunk) {
    // NOTE(egor): we check a tile value inside tile chunk of sixe 256x256
    uint32 TileMapValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
    IsEmpty = (TileMapValue == 1 ||
               TileMapValue == 4 ||
               TileMapValue == 5);
    
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
CanonicalizePosition(tile_map *TileMap, tile_map_position *Pos) {
  
  tile_map_position Result = *Pos;
  
  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.X);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Y);
  
  *Pos = Result;
  
  return Result;
}




inline bool32
AreOnTheSameTile(tile_map_position *A, tile_map_position *B) {
  
  bool32 Result = (A->AbsTileX == B->AbsTileX &&
                   A->AbsTileY == B->AbsTileY &&
                   A->AbsTileZ == B->AbsTileZ);
  
  return Result;
  
}
