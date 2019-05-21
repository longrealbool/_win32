

#if 0
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

#endif