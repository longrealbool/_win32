#include "game.h"
#include "game_world.cpp"



internal void
GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
  int16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
  
  int16 *SampleOut = SoundBuffer->Samples;
  for(int SampleIndex = 0;
      SampleIndex < SoundBuffer->SampleCount;
      ++SampleIndex)
  {
    // TODO(Egor): Draw this out for people
#if 0
    real32 SineValue = sinf(GameState->tSine);
    int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
    int16 SampleValue = 0;
#endif
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    
#if 0
    GameState->tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
    if(GameState->tSine > 2.0f*Pi32)
    {
      GameState->tSine -= 2.0f*Pi32;
    }
#endif
  }
}

#pragma pack(push, 1)
struct bitmap_header {
  uint16 FileType;     /* File type, always 4D42h ("BM") */
  uint32 FileSize;     /* Size of the file in bytes */
  uint16 Reserved1;    /* Always 0 */
  uint16 Reserved2;    /* Always 0 */
  uint32 BitmapOffset; /* Starting position of image data in bytes */
  uint32 Size;            /* Size of this header in bytes */
  int32 Width;           /* Image width in pixels */
  int32 Height;          /* Image height in pixels */
  uint16 Planes;          /* Number of color planes */
  uint16 BitsPerPixel;    /* Number of bits per pixel */
  /* Fields added for Windows 3.x follow this line */
  uint32 Compression;
  uint32 SizeOfBitmap;
  int32 HorzResolution;
  int32 VertResolution;
  uint32 ColorUser;
  uint32 ColorsImportant;
  /* bitmap winNT color masks */
  uint32 RedMask;
  uint32 GreenMask;
  uint32 BlueMask;
};
#pragma pack(pop)




internal loaded_bitmap
DEBUGLoadBMP(debug_platform_read_entire_file *ReadEntireFile,
             thread_context *Thread, char *FileName)
{
  loaded_bitmap LoadedBitmap = {};
  debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
  
  if(ReadResult.ContentsSize != 0) {
    
    bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
    
    // NOTE(Egor, bitmap_format): be aware that bitmap can have negative height for
    // top-down pictures, and there can be compression
    uint32 *Pixel = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
    LoadedBitmap.Pixels = Pixel;
    LoadedBitmap.Height = Header->Height;
    LoadedBitmap.Width = Header->Width;
    
    // NOTE(Egor, bitmap_loading): we have to account all color masks in header,
    // in order to correctly load bitmaps saved with different software
    uint32 RedMask = Header->RedMask;
    uint32 GreenMask = Header->GreenMask;
    uint32 BlueMask = Header->BlueMask;
    uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
    
    bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
    bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
    bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
    bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
    
    int32 AlphaShift = 24 - (int32)AlphaScan.Index;
    int32 RedShift = 16 - (int32)RedScan.Index;
    int32 GreenShift = 8 - (int32)GreenScan.Index;
    int32 BlueShift = 0 - (int32)BlueScan.Index;
    
    Assert(AlphaScan.Found);    
    Assert(RedScan.Found);
    Assert(GreenScan.Found);
    Assert(BlueScan.Found);
    
    uint32 *SourceDest = Pixel;
    
    for(int32 Y = 0; Y < Header->Height; ++Y) {
      for(int32 X = 0; X < Header->Width; ++X) {
        
        uint32 C = *SourceDest;

        *SourceDest = (RotateLeft(C & AlphaMask, AlphaShift) |
                       RotateLeft(C & RedMask, RedShift) |
                       RotateLeft(C & GreenMask, GreenShift) |
                       RotateLeft(C & BlueMask, BlueShift));
        
        SourceDest++;
      }
    }
    
  }
  
  return LoadedBitmap;
}


internal void
DrawBitmap(game_offscreen_buffer *Buffer,
           loaded_bitmap *Bitmap,
           real32 RealX, real32 RealY) {
  
  int32 MinX = RoundReal32ToInt32(RealX);
  int32 MinY = RoundReal32ToInt32(RealY);
  int32 MaxX = MinX + Bitmap->Width;
  int32 MaxY = MinY + Bitmap->Height;
  
  
  
  int32 SourceOffsetX = 0;
  // clipping value to actual size of the backbuffer
  if(MinX < 0) {
    SourceOffsetX = -MinX;
    MinX = 0;
  }
  int32 SourceOffsetY = 0;
  if(MinY < 0) { 
    SourceOffsetY = -MinY;
    MinY = 0;
  }
  // we will write up to, but not including to final row and column
  if(MaxX > Buffer->Width) MaxX = Buffer->Width;
  if(MaxY > Buffer->Height) MaxY = Buffer->Height;
  
  // NOTE(Egor): just example
  // uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
  
  uint32 *SourceRow = (uint32 *)Bitmap->Pixels + (Bitmap->Height-1)*(Bitmap->Width);
  SourceRow += -SourceOffsetY*Bitmap->Width + SourceOffsetX;
  
  // go to line to draw
  uint8 *DestRow = ((uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*Buffer->BytesPerPixel);
  
  for(int Y = MinY; Y < MaxY; ++Y)
  {
    uint32 *Dest = (uint32 *)DestRow;
    uint32 *Source = SourceRow;
    for(int X = MinX; X < MaxX; ++X)
    {
      
      real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;
      real32 Rs = (real32)((*Source >> 16) & 0xFF);
      real32 Gs = (real32)((*Source >> 8) & 0xFF);
      real32 Bs = (real32)((*Source >> 0) & 0xFF);
      
      
      real32 Rd = (real32)((*Dest >> 16) & 0xFF);
      real32 Gd = (real32)((*Dest >> 8) & 0xFF);
      real32 Bd = (real32)((*Dest >> 0) & 0xFF);
      
      real32 R = (1.0f - A)*Rd + A*Rs;
      real32 G = (1.0f - A)*Gd + A*Gs;
      real32 B = (1.0f - A)*Bd + A*Bs;
      
      
      *Dest = (((uint32)(R + 0.5f) << 16) |
               ((uint32)(G + 0.5f) << 8) |
               ((uint32)(B + 0.5f) << 0));
      
      Dest++;
      Source++;
    }
    DestRow += Buffer->Pitch;
    SourceRow -= Bitmap->Width;
    
  }
  
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax,
              real32 R, real32 G, real32 B)
{
  
  int32 MinX = RoundReal32ToInt32(vMin.X);
  int32 MinY = RoundReal32ToInt32(vMin.Y);
  int32 MaxX = RoundReal32ToInt32(vMax.X);
  int32 MaxY = RoundReal32ToInt32(vMax.Y);
  
  // clipping value to actual size of the backbuffer
  if(MinX < 0) MinX = 0;
  if(MinY < 0) MinY = 0;
  // we will write up to, but not including to final row and column
  if(MaxX > Buffer->Width) MaxX = Buffer->Width;
  if(MaxY > Buffer->Height) MaxY = Buffer->Height;
  
  uint32 Color = ((RoundReal32ToUInt32(R * 255.0f) << 16) |
                  (RoundReal32ToUInt32(G * 255.0f) << 8) |
                  (RoundReal32ToUInt32(B * 255.0f) << 0));
  
  
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
  
  // go to line to draw
  uint8 *Row = ((uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*Buffer->BytesPerPixel);
  // TODO: color
  for(int Y = MinY; Y < MaxY; ++Y)
  {
    uint32 *Pixel = (uint32 *)Row;
    for(int X = MinX; X < MaxX; ++X)
    {
      *Pixel++ = Color;
    }
    Row += Buffer->Pitch;
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

inline low_entity *
GetLowEntity(game_state *GameState, uint32 LowIndex) {
  
  low_entity *Result = 0;
  entity Entity = {};
  
  if((LowIndex > 0) && (LowIndex < GameState->LowEntityCount)) {
    
    Result = GameState->LowEntity + LowIndex;
  }
  
  return Result;
}

inline v2
GetCameraSpaceP(game_state *GameState, low_entity *LowEntity) {
  
  world_difference Diff = Subtract(GameState->World,
                                   &LowEntity->P, &GameState->CameraP);
  return Diff.dXY;
}

internal high_entity *
MakeEntityHighFrequency(game_state *GameState, low_entity *LowEntity,
                        uint32 LowIndex, v2 CameraSpaceP) {
  
  high_entity *Result = 0;
  
  Assert(LowEntity->HighEntityIndex == 0);
  if(LowEntity->HighEntityIndex == 0) {
    
    if(GameState->HighEntityCount < ArrayCount(GameState->HighEntity)) {
      
      uint32 HighIndex = GameState->HighEntityCount++;
      Result = &GameState->HighEntity[HighIndex];
      
      Result->P = CameraSpaceP;
      Result->ChunkZ = LowEntity->P.ChunkZ;
      Result->dP = V2(0,0);
      Result->FacingDirection = 0;
      Result->LowEntityIndex = LowIndex;
      
      LowEntity->HighEntityIndex = HighIndex;
      
    }
    else {
      
      InvalidCodePath;
    }
  }
  
  return Result;
}

inline high_entity *
MakeEntityHighFrequency(game_state *GameState, uint32 LowIndex) {
  
  high_entity *Result = 0;
  
  low_entity *LowEntity = GameState->LowEntity + LowIndex;
  if(LowEntity->HighEntityIndex) {
    
    Result = GameState->HighEntity + LowEntity->HighEntityIndex;
  }
  else {
    
    v2 CameraSpaceP = GetCameraSpaceP(GameState, LowEntity);
    Result = MakeEntityHighFrequency(GameState, LowEntity, LowIndex, CameraSpaceP);
  }
  
  return Result;
}


inline entity
GetHighEntity(game_state *GameState, uint32 LowIndex) {
  
  entity Result = {};
  
  
  if((LowIndex > 0) && (LowIndex < GameState->LowEntityCount)) {
    
    Result.LowIndex = LowIndex;
    Result.Low = GetLowEntity(GameState, LowIndex);
    Result.High = MakeEntityHighFrequency(GameState, LowIndex);
  }
  
  return Result;
}


inline void
MakeEntityLowFrequency(game_state *GameState, uint32 LowIndex) {
  
  low_entity *LowEntity = &GameState->LowEntity[LowIndex];
  uint32 HighIndex = LowEntity->HighEntityIndex;
  
  if(HighIndex) {
    
    uint32 LastHighIndex = GameState->HighEntityCount - 1;
    
    if(HighIndex != LastHighIndex) {
      
      high_entity *LastEntity = GameState->HighEntity + LastHighIndex;
      high_entity *DeletedEntity = GameState->HighEntity + HighIndex;
      
      *DeletedEntity = *LastEntity;
      GameState->LowEntity[LastEntity->LowEntityIndex].HighEntityIndex = HighIndex;
    }
    
    --GameState->HighEntityCount;
    LowEntity->HighEntityIndex = 0;
  }
}

inline bool32
ValidateEntityPairs(game_state *GameState) {
  
  bool32 Valid = true;
  for(uint32 HighEntityIndex = 1;
      HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex) {
    
    high_entity *HighEntity = GameState->HighEntity + HighEntityIndex;
    Valid = Valid && 
      (GameState->LowEntity[HighEntity->LowEntityIndex].HighEntityIndex == HighEntityIndex); 
  }
  
  return Valid;
}

internal void 
OffsetAndCheckFrequencyByArea(game_state *GameState, v2 Offset, rectangle2 CameraInBound) {
  
  for(uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount;) {
    
    high_entity *High = GameState->HighEntity + HighEntityIndex;
    High->P += Offset;
    if(IsInRectangle(CameraInBound, High->P)) {
      
      ++HighEntityIndex;
    }
    else {
      
      Assert(GameState->LowEntity[High->LowEntityIndex].HighEntityIndex == HighEntityIndex);
      MakeEntityLowFrequency(GameState, High->LowEntityIndex);
    }
  }
}

struct add_low_entity_result {
  
  low_entity *Low;
  uint32 Index;
};

internal add_low_entity_result
AddLowEntity(game_state *GameState, entity_type EntityType, world_position *P) {
  
  Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntity));
  
  uint32 LowEntityIndex = GameState->LowEntityCount++;
  
  low_entity *LowEntity = GameState->LowEntity + LowEntityIndex;
  *LowEntity = {};
  LowEntity->Type = EntityType;
  
  // TODO(Egor): this is awfull
  if(P) {
    LowEntity->P = *P;
    ChangeEntityLocation(&GameState->WorldArena, GameState->World, LowEntityIndex, 0, P);
  }
  
  add_low_entity_result Result;
  Result.Low = LowEntity;
  Result.Index = LowEntityIndex;
  return Result;
}


internal add_low_entity_result
AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX,
                                                   AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Wall, &P);
  
  Entity.Low->Height = GameState->World->TileSideInMeters;
  Entity.Low->Width = GameState->World->TileSideInMeters;
  Entity.Low->Collides = true;
  
  return Entity;
}


internal add_low_entity_result
AddPlayer(game_state *GameState) {
  
  world_position P = GameState->CameraP;
  add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Hero, &P);
  
  Entity.Low->Collides = true;
  
  Entity.Low->Height = 0.5f;
  Entity.Low->Width = 1.0f;
  
  MakeEntityHighFrequency(GameState, Entity.Index);
  
  // NOTE(Egor): if camera doesn't follows any entity, make it follow this
  if(GameState->CameraFollowingEntityIndex == 0) {
    
    GameState->CameraFollowingEntityIndex = Entity.Index;
  }
  
  return Entity;
}

internal add_low_entity_result
AddMonster(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX,
                                                   AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Monster, &P);
  
  Entity.Low->Height = 0.5f;
  Entity.Low->Width = 1.0f;
  Entity.Low->Collides = true;
  
  return Entity;
}

internal add_low_entity_result
AddFamiliar(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX,
                                                   AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Familiar, &P);
  
  Entity.Low->Height = 0.5f;
  Entity.Low->Width = 1.0f;
  Entity.Low->Collides = false;
  
  return Entity;
}


internal void
MovePlayer(game_state *GameState, entity Entity, real32 dT, v2 ddP) {
  
  
  world *World = GameState->World;
  
  real32 ddLengthSq = LengthSq(ddP);
  
  if(ddLengthSq > 1.0f) {
    
    ddP *= (1.0f/SquareRoot(ddLengthSq));
  }
  
  real32 PlayerSpeed = 50.0f;
  
  ddP *= PlayerSpeed;
  
  ddP += -Entity.High->dP*8.0f;
  
  v2 OldPlayerP = Entity.High->P;
  v2 PlayerDelta = (0.5f * ddP * Square(dT) +
                    Entity.High->dP*dT);
  Entity.High->dP = ddP*dT + Entity.High->dP;
  
  v2 NewPlayerP = OldPlayerP + PlayerDelta;
  
  for(uint32 Iteration = 0; (Iteration < 4); ++Iteration) {
    
    v2 WallNormal = {};
    real32 tMin = 1.0f;
    uint32 HitHighEntityIndex = 0;
    
    v2 DesiredPosition = Entity.High->P + PlayerDelta;
    
    for(uint32 TestHighEntityIndex = 1; TestHighEntityIndex < GameState->HighEntityCount; ++TestHighEntityIndex) {
      
      if(TestHighEntityIndex != Entity.Low->HighEntityIndex) {
        
        // TODO(Egor): I should decide once and for all, if I want to keep this type 
        // of entity bundle
        entity TestEntity;
        TestEntity.High = GameState->HighEntity + TestHighEntityIndex;
        TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
        TestEntity.Low = GameState->LowEntity + TestEntity.LowIndex;
        
        if(TestEntity.Low->Collides) {
          
          real32 DiameterW = TestEntity.Low->Width + Entity.Low->Width;
          real32 DiameterH = TestEntity.Low->Height + Entity.Low->Height;
          
          v2 MinCorner = -0.5f*v2{DiameterW, DiameterH};
          v2 MaxCorner = 0.5f*v2{DiameterW, DiameterH};
          
          v2 Rel = Entity.High->P - TestEntity.High->P;
          
          if(TestWall(&tMin, MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                      MinCorner.Y, MaxCorner.Y)) {
            
            WallNormal = v2{1.0f, 0.0f};
            HitHighEntityIndex = TestHighEntityIndex;
          }
          if(TestWall(&tMin, MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                      MinCorner.Y, MaxCorner.Y)) {
            
            WallNormal = v2{-1.0f, 0.0f};
            HitHighEntityIndex = TestHighEntityIndex;
          }
          if(TestWall(&tMin, MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                      MinCorner.X, MaxCorner.X)) {
            
            WallNormal = v2{0.0f, 1.0f};
            HitHighEntityIndex = TestHighEntityIndex;
          }
          if(TestWall(&tMin, MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                      MinCorner.X, MaxCorner.X)) {
            
            WallNormal = v2{0.0f, -1.0f};
            HitHighEntityIndex = TestHighEntityIndex;
          }
        }
      }
    }
    
    Entity.High->P += tMin*PlayerDelta;    
    if(HitHighEntityIndex) {
      
      // NOTE(Egor): reflecting vector calculation
      PlayerDelta = DesiredPosition - Entity.High->P; 
      PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
      Entity.High->dP = Entity.High->dP - 1*Inner(Entity.High->dP, WallNormal)*WallNormal;
      
      high_entity *HitHigh = GameState->HighEntity + HitHighEntityIndex;
      low_entity *HitLow = GameState->LowEntity + HitHigh->LowEntityIndex;
      //      Entity.High->AbsTileZ += HitLow->dAbsTileZ; 
    }
    else {
      
      break;
    }
    
  }
  
  // NOTE(Egor): updating facing directions
  if(Entity.High->dP.X != 0 || Entity.High->dP.Y != 0) {
    if(AbsoluteValue(Entity.High->dP.X) > AbsoluteValue(Entity.High->dP.Y)) {
      
      if(Entity.High->dP.X > 0) {
        
        Entity.High->FacingDirection = 0;        
      }
      else {
        
        Entity.High->FacingDirection = 2;      
      }
    }
    else if(AbsoluteValue(Entity.High->dP.X) < AbsoluteValue(Entity.High->dP.Y)) {
      
      if(Entity.High->dP.Y > 0) {
        Entity.High->FacingDirection = 1;
      }
      else {
        
        Entity.High->FacingDirection = 3;
      }
    }
  }
  
  // TODO(Egor): think about should I bundle this, and if so, how
  world_position NewP = MapIntoChunkSpace(World, GameState->CameraP, Entity.High->P);
  ChangeEntityLocation(&GameState->WorldArena, World,
                       Entity.LowIndex, &Entity.Low->P, &NewP);
  Entity.Low->P = NewP;
  
  
}

internal void 
SetCamera(game_state *GameState, world_position NewCameraP) {
  
  world *World = GameState->World;
  
  Assert(ValidateEntityPairs(GameState));
  
  world_difference dCameraP = Subtract(World, &NewCameraP,  &GameState->CameraP); 
  GameState->CameraP = NewCameraP;
  
  uint32 TileSpanX = 17*3;
  uint32 TileSpanY = 9*3;
  
  rectangle2 CameraInBound = RectCenterDim(V2(0,0),
                                           World->TileSideInMeters*V2((real32)TileSpanX,
                                                                      (real32)TileSpanY));
  v2 EntityOffsetForFrame = -dCameraP.dXY;
  OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraInBound);
  
  Assert(ValidateEntityPairs(GameState));
  
  world_position MinChunk = MapIntoChunkSpace(World, NewCameraP, GetMinCorner(CameraInBound));
  world_position MaxChunk = MapIntoChunkSpace(World, NewCameraP, GetMaxCorner(CameraInBound));
  
  for(int32 ChunkY = MinChunk.ChunkY; ChunkY <= MaxChunk.ChunkY; ++ChunkY) {
    for(int32 ChunkX = MinChunk.ChunkX; ChunkX <= MaxChunk.ChunkX; ++ChunkX) {
      
      world_chunk *Chunk = GetChunk(World, ChunkX, ChunkY, NewCameraP.ChunkZ);
      if(Chunk) {
        
        for(world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next) { 
          for(uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex) {
            
            uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndex];
            low_entity *LowEntity = GameState->LowEntity + LowEntityIndex;
            if(LowEntity->HighEntityIndex == 0) {
              
              v2 CameraSpaceP = GetCameraSpaceP(GameState, LowEntity);
              if(IsInRectangle(CameraInBound, CameraSpaceP)) {
                
                //                MakeEntityHighFrequency(GameState, LowEntity, EntityIndex, CameraSpaceP);
                MakeEntityHighFrequency(GameState, LowEntity, LowEntityIndex, CameraSpaceP);
              }
            }
          }
        }
      }
    }
  }
  
  Assert(ValidateEntityPairs(GameState));
}



extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
         (ArrayCount(Input->Controllers[0].Buttons)));
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  
  
  
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if(!Memory->IsInitialized)
  {
    
    AddLowEntity(GameState, EntityType_Null, 0);
    GameState->HighEntityCount = 1;
    // TODO(Egor): This may be more appropriate to do in the platform layer
    
    
    
    GameState->Backdrop = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//background.bmp");
    
    hero_bitmaps *Bitmap = GameState->Hero;
    
    Bitmap->HeroHead = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//figurine.bmp");
    Bitmap->HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_right.bmp");
    Bitmap->Align = V2(51, 112);
    
    Bitmap[1] = Bitmap[0];
    Bitmap[1].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_up.bmp");
    
    Bitmap[2] = Bitmap[0];
    Bitmap[2].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_left.bmp");
    
    Bitmap[3] = Bitmap[0];
    Bitmap[3].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_down.bmp");
    
    GameState->Tree = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//tree.bmp");
    //    GameState->Tree = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//tree00.bmp");
    
    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                    (uint8 *)Memory->PermanentStorage + sizeof(game_state));
    
    uint32 TilesPerHeight = 9;
    uint32 TilesPerWidth = 17;
    
    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;
    InitializeWorld(World, 1.4f);
    
#if 0
    uint32 EntityIndex = AddEntity(GameState);
    InitializePlayer(GameState, EntityIndex);
#endif
    
    uint32 ScreenBaseX = 0;
    uint32 ScreenBaseY = 0;
    uint32 ScreenBaseZ = 0;
    uint32 ScreenY = ScreenBaseY;
    uint32 ScreenX = ScreenBaseX;
    uint32 AbsTileZ = ScreenBaseZ;
    
    bool32 DoorToRight = false;
    bool32 DoorToLeft = false;
    bool32 DoorToTop = false;
    bool32 DoorToBottom = false;    
    bool32 DoorUp = false;
    bool32 DoorDown = false;
    
    uint32 CounterOfTwo = 0;
    
    for(uint32 ScreenIndex = 0; ScreenIndex < 4; ++ScreenIndex) {
      
      // TODO(Egor, TileMap): get again through logic of creating tilemap
      uint32 RandomChoice;
      
      bool32 NonValid = 1; // WARNING(Egor): TEST (!!!)
      //if(DoorUp || DoorDown) {
      if(NonValid) {
        RandomChoice = RollTheDice() % 2;
      }
      else {
        RandomChoice = RollTheDice() % 3;
      }
      
      // NOTE(Egor): keep track of is there a need to create another stairwell
      bool32 CreatedZDoor = false;
      if (RandomChoice == 2) {
        
        CreatedZDoor = true;
        if(AbsTileZ == ScreenBaseZ) 
          DoorUp = true;
        else
          DoorDown = true;
        
      }
      else if(RandomChoice == 0) {
        DoorToRight = true;
      }
      else {
        DoorToTop = true;
      }
      
      
      for(uint32 TileY = 0; TileY < TilesPerHeight; ++TileY) {
        for(uint32 TileX = 0; TileX < TilesPerWidth; ++TileX) {
          
          uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;
          uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
          
          uint32 TileValue = 1;
          
          if(TileX == 0 || TileX == (TilesPerWidth - 1)) {
            TileValue = 2;
            
            if(DoorToRight && TileX == TilesPerWidth - 1) {
              if(TileY == TilesPerHeight/2)
                TileValue = 1;
            }
            
            if(DoorToLeft && TileX == 0 ) {
              if(TileY == TilesPerHeight/2)
                TileValue = 1;
            }
            
          }
          if(TileY == 0 || TileY == (TilesPerHeight - 1)) {
            TileValue = 2;
            
            if(DoorToTop && TileY == TilesPerHeight - 1) {
              if(TileX == TilesPerWidth/2)
                TileValue = 1;
            }
            
            if(DoorToBottom && TileY == 0) {
              if(TileX == TilesPerWidth/2)
                TileValue = 1;
            }
            
          }
          
          if(TileY == 4 && TileX == 8) {
            if(DoorUp) 
              TileValue = 4;
            if(DoorDown)
              TileValue = 5;
          }
          
          if(TileValue == 2) {
            
            AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
          }
        }
      }
      
      DoorToLeft = DoorToRight;
      DoorToBottom = DoorToTop;    
      DoorToRight = false;
      DoorToTop = false;
      
      if(CreatedZDoor) {
        
        DoorDown = !DoorDown;
        DoorUp = !DoorUp;
      }
      else {
        
        DoorUp = false;
        DoorDown = false;
      }
      
      if(RandomChoice == 2) {
        
        if(AbsTileZ == ScreenBaseZ) 
          AbsTileZ = ScreenBaseZ + 1;
        else
          AbsTileZ = ScreenBaseZ;
      }
      else if(RandomChoice == 0) {
        ScreenX++;
      }
      else if(RandomChoice == 1) {
        ScreenY++;
      }
      
    }
    
    world_position NewCameraP = {};
    uint32 CameraTileX = ScreenBaseX*TilesPerWidth + 17/2;
    uint32 CameraTileY = ScreenBaseY*TilesPerHeight + 9/2;
    uint32 CameraTileZ = ScreenBaseZ;
    NewCameraP = ChunkPositionFromTilePosition(GameState->World, CameraTileX, CameraTileY,
                                               CameraTileZ);
    
    AddMonster(GameState, CameraTileX + 2, CameraTileY + 2, CameraTileZ);
    AddFamiliar(GameState, CameraTileX + 3, CameraTileY + 3, CameraTileZ);
    SetCamera(GameState, NewCameraP);
    
    
    
    Memory->IsInitialized = true;
  }
  
  world *World = GameState->World;
  
  // TODO(Egor): put this into renderer in the future
  int32 TileSideInPixels = 60;
  real32 MetersToPixels = TileSideInPixels/World->TileSideInMeters;
  
  // NOTE(Egor): save 
  
  
  for(int ControllerIndex = 0;
      ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
  {
    
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    uint32 LowIndex = GameState->PlayerIndexForController[ControllerIndex];
    
    if(LowIndex == 0) {
      
      if(Controller->Back.EndedDown) {
        
        uint32 EntityIndex = AddPlayer(GameState).Index;
        GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
      }
    }
    else {
      
      entity ControllingEntity = GetHighEntity(GameState, LowIndex);
      v2 ddPlayer = {};
      if(Controller->IsAnalog)
      {
        // analog movement
        ddPlayer = v2{Controller->StickAverageX, Controller->StickAverageY};
      }
      else
      {
        
        if(Controller->MoveUp.EndedDown) {
          ddPlayer.Y = 1.0f;
        }
        if(Controller->MoveRight.EndedDown) {
          ddPlayer.X = 1.0f;
        }
        if(Controller->MoveDown.EndedDown) {
          ddPlayer.Y = -1.0f;
        }
        if(Controller->MoveLeft.EndedDown) {
          ddPlayer.X = -1.0f;
        }
      }
      
      if(Controller->Start.EndedDown) {
        ControllingEntity.High->dZ = 5.0f;
      }
      
      MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddPlayer);
    }
  }
  
  
  entity CameraFollowingEntity = GetHighEntity(GameState, GameState->CameraFollowingEntityIndex);
  
  if(CameraFollowingEntity.High) {
    
    world_position NewCameraP = GameState->CameraP;
    
#if 0
    
    NewCameraP.AbsTileZ = CameraFollowingEntity.Low->P.AbsTileZ;
    
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
    
    NewCameraP = CameraFollowingEntity.Low->P;
    
#endif
    
    SetCamera(GameState, NewCameraP);
    
  }
  //
  // NOTE(Egor): rudimentary render starts below
  //
  
  
  //#if NO_ASSETS
#if 1
  DrawRectangle(Buffer, V2(0.0f, 0.0f), V2((real32)Buffer->Width, (real32)Buffer->Height),
                0.5f, 0.5f, 0.5f);
#else
  DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);
#endif
  
  real32 CenterX = 0.5f*Buffer->Width;
  real32 CenterY = 0.5f*Buffer->Height;
  
  
  entity_visible_piece_group PieceGroup;
  for(uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex) {
    
    
    high_entity *HighEntity = GameState->HighEntity + HighEntityIndex;
    low_entity *LowEntity = GameState->LowEntity + HighEntity->LowEntityIndex;
    
    entity Entity;
    Entity.LowIndex = HighEntity->LowEntityIndex;
    Entity.High = HighEntity;
    Entity.Low = LowEntity;
    
    real32 dt = Input->dtForFrame;
    
    //    UpdateEntity(GameState, Entity, dt);
    
    real32 ddZ = -9.8f;
    HighEntity->Z = 0.5f * ddZ * Square(dt) + HighEntity->dZ*dt + HighEntity->Z;
    HighEntity->dZ = ddZ*dt + HighEntity->dZ;
    
    if(HighEntity->Z < 0) {
      HighEntity->Z = 0;
    }
    real32 EntityZ = -HighEntity->Z*MetersToPixels;
    
    
    real32 PlayerR = 1.0f;
    real32 PlayerG = 0.0f;
    real32 PlayerB = 0.0f;
    
    

    
    
    hero_bitmaps *Hero = &GameState->Hero[HighEntity->FacingDirection];    
    switch(LowEntity->Type) {
      
      case EntityType_Hero: {
        
        PushPiece(&PieceGroup, &Hero->HeroTorso, V2(0, 0), 0, Hero->Align);
        PushPiece(&PieceGroup, &Hero->HeroHead, V2(0, 0), 0, Hero->Align);
        PushPiece(&PieceGroup, &Hero->HeroCape, V2(0, 0), 0, Hero->Align);
      } break;
      case EntityType_Wall: {
        
        PushPiece(&PieceGroup, &GameState->Tree, V2(0, 0), V2(30, 60));
      } break;
      case EntityType_Monster: {
        
        PushPiece(&PieceGroup, &Hero->HeroHead, V2(0, 0), Hero->Align);
      } break;
      case EntityType_Familiar: {
        
        PushPiece(&PieceGroup, &Hero->HeroHead, V2(0, 0), Hero->Align);
      } break;
      default: {
        InvalidCodePath;
      }
    }  
    
    real32 EntityGroundPointX = CenterX + HighEntity->P.X*MetersToPixels;
    real32 EntityGroundPointY = CenterY - HighEntity->P.Y*MetersToPixels;
    
#if 0
    v2 PlayerLeftTop = 
    { PlayerGroundPointX - 0.5f*LowEntity->Width*MetersToPixels ,
      PlayerGroundPointY - 0.5f*LowEntity->Height*MetersToPixels}
    ;
    v2 EntityWidthHeight = {LowEntity->Width, LowEntity->Height};
    
    DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MetersToPixels*EntityWidthHeight,
                  PlayerR, PlayerG, PlayerB); 
#endif
    
    for(uint32 PieceIndex = 0; PieceIndex < PieceGroup.Count; ++PieceIndex) {
      
      entity_visible_piece *Piece = PieceGroup.Pieces + PieceIndex;
      DrawBitmap(Buffer, Piece->Bitmap,
                 EntityGroundPointX + Piece->Offset.X,
                 EntityGroundPointY + Piece->Offset.Y + Piece->OffsetZ + EntityZ);
      
    }
  }
}
  
extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  GameOutputSound(GameState, SoundBuffer, 400);
}



#if 0
internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
  // TODO(Egor): Let's see what the optimizer does
  
  uint8 *Row = (uint8 *)Buffer->Memory;    
  for(int Y = 0;
      Y < Buffer->Height;
      ++Y)
  {
    uint32 *Pixel = (uint32 *)Row;
    for(int X = 0;
        X < Buffer->Width;
        ++X)
    {
      uint8 Blue = (uint8)(X + BlueOffset);
      uint8 Green = (uint8)(Y + GreenOffset);
      
      *Pixel++ = ((Green << 16) | Blue);
    }
    
    Row += Buffer->Pitch;
  }
}
#endif