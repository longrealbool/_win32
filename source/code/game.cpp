#include "game.h"
#include "game_tile.cpp"



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
           real32 RealX, real32 RealY,
           uint32 AlignX = 0, uint32 AlignY = 0) {
  
  RealX -= (real32)AlignX;
  RealY -= (real32)AlignY;
  
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

internal void
ChangeEntityResidence(game_state *GameState, uint32 EntityIndex, entity_residence Residence) {
  
  if(Residence == EntityResidence_High) {
    
    if(GameState->EntityResidency[EntityIndex] != EntityResidence_High) {
      
      high_entity *HighEntity = &GameState->HighEntity[EntityIndex];
      dormant_entity *DormantEntity = &GameState->DormantEntity[EntityIndex];
      
      tile_map_difference Diff = Subtract(GameState->World->TileMap,
                                          &DormantEntity->P, &GameState->CameraP);
      
      HighEntity->P = Diff.dXY;
      HighEntity->AbsTileZ = DormantEntity->P.AbsTileZ;
      HighEntity->dP = V2(0,0);
      HighEntity->FacingDirection = 0;
      
    }
  }
  
  GameState->EntityResidency[EntityIndex] = Residence;
}


inline entity
GetEntity(game_state *GameState, entity_residence Residence, uint32 Index) {
  
  entity Entity = {};
  
  if((Index > 0) && (Index < GameState->EntityCount)) {
    
    if(GameState->EntityResidency[Index] < Residence) {
      
      ChangeEntityResidence(GameState, Index, Residence);
      Assert(GameState->EntityResidency[Index] >= Residence);
    }
    
    Entity.Residence = Residence;
    Entity.High = &GameState->HighEntity[Index];
    Entity.Low = &GameState->LowEntity[Index];
    Entity.Dormant = &GameState->DormantEntity[Index];
  }
  
  return Entity;
}



internal void
MovePlayer(game_state *GameState, entity Entity, real32 dT, v2 ddP) {
  
  
  tile_map *TileMap = GameState->World->TileMap;
  
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
  
  /*
  uint32 MinTileX = Min(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
  uint32 MinTileY = Min(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
  uint32 MaxTileX = Max(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
  uint32 MaxTileY = Max(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
  
  uint32 EntityTileWidth = CeilReal32ToInt32(Entity.High->Width / TileMap->TileSideInMeters);
  uint32 EntityTileHeight = CeilReal32ToInt32(Entity.High->Height / TileMap->TileSideInMeters);
  
  MinTileX -= EntityTileWidth;
  MinTileY -= EntityTileWidth;
  MaxTileX += EntityTileWidth;
  MaxTileY += EntityTileWidth;
  
  uint32 AbsTileZ = Entity.High->P.AbsTileZ;
  */
  
  real32 tRemaining = 1.0f;
  
  for(uint32 Iteration = 0; (Iteration < 4) && (tRemaining > 0.0f); ++Iteration) {
    
    v2 WallNormal = {};
    real32 tMin = 1.0f;
    uint32 HitEntityIndex = 0;
    
    for(uint32 EntityIndex = 1; EntityIndex < GameState->EntityCount; ++EntityIndex) {
      
      entity TestEntity = GetEntity(GameState, EntityResidence_High, EntityIndex);
      if(TestEntity.High != Entity.High) {
        
        if(TestEntity.Dormant->Collides) {
          
          real32 DiameterW = TestEntity.Dormant->Width + Entity.Dormant->Width;
          real32 DiameterH = TestEntity.Dormant->Height + Entity.Dormant->Height;
          
          v2 MinCorner = -0.5f*v2{DiameterW, DiameterH};
          v2 MaxCorner = 0.5f*v2{DiameterW, DiameterH};
          
          v2 Rel = Entity.High->P - TestEntity.High->P;
          
          if(TestWall(&tMin, MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                      MinCorner.Y, MaxCorner.Y)) {
            
            WallNormal = v2{1.0f, 0.0f};
            HitEntityIndex = EntityIndex;
          }
          if(TestWall(&tMin, MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                      MinCorner.Y, MaxCorner.Y)) {
            
            WallNormal = v2{-1.0f, 0.0f};
            HitEntityIndex = EntityIndex;
          }
          if(TestWall(&tMin, MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                      MinCorner.X, MaxCorner.X)) {
            
            WallNormal = v2{0.0f, 1.0f};
            HitEntityIndex = EntityIndex;
          }
          if(TestWall(&tMin, MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                      MinCorner.X, MaxCorner.X)) {
            
            WallNormal = v2{0.0f, -1.0f};
            HitEntityIndex = EntityIndex;
          }
        }
      }
    }
    
    Entity.High->P += tMin*PlayerDelta;    
    if(HitEntityIndex) {
      
      // NOTE(Egor): reflecting vector calculation
      PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
      tRemaining -= tMin*tRemaining;
      Entity.High->dP = Entity.High->dP - 1*Inner(Entity.High->dP, WallNormal)*WallNormal;
      
      entity HitEntity = GetEntity(GameState, EntityResidence_Dormant, HitEntityIndex);
      
      Entity.High->AbsTileZ += HitEntity.Dormant->dAbsTileZ; 
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
  
  Entity.Dormant->P = MapIntoTileSpace(TileMap, GameState->CameraP, Entity.High->P);
  
  
}


internal uint32
AddEntity(game_state *GameState, entity_type EntityType) {
  
  uint32 EntityIndex = GameState->EntityCount++;
  
  Assert(GameState->EntityCount < ArrayCount(GameState->DormantEntity));
  Assert(GameState->EntityCount < ArrayCount(GameState->LowEntity));
  Assert(GameState->EntityCount < ArrayCount(GameState->HighEntity));
  
  GameState->EntityResidency[EntityIndex] = EntityResidence_Dormant;
  GameState->DormantEntity[EntityIndex] = {};
  GameState->LowEntity[EntityIndex] = {};
  GameState->HighEntity[EntityIndex] = {};
  
  GameState->DormantEntity[EntityIndex].Type = EntityType;
  
  return EntityIndex;
}

internal uint32
AddPlayer(game_state *GameState) {
  
  uint32 EntityIndex = AddEntity(GameState, EntityType_Hero);
  
  entity Entity = GetEntity(GameState, EntityResidence_Dormant,  EntityIndex);
  
  Entity.Dormant->P.AbsTileX = 5;
  Entity.Dormant->P.AbsTileY = 2;
  Entity.Dormant->P.Offset_.X = 0.0f;
  Entity.Dormant->P.Offset_.Y = 0.0f;
  Entity.Dormant->Collides = true;
  
  Entity.Dormant->Height = 0.5f;
  Entity.Dormant->Width = 1.0f;
  
  ChangeEntityResidence(GameState, EntityIndex, EntityResidence_High);
  
  // NOTE(Egor): if camera doesn't follows any entity, make it follow this
  if(GetEntity(GameState, EntityResidence_Dormant,
               GameState->CameraFollowingEntityIndex).Residence == 
     EntityResidence_NoneExistant) {
    
    GameState->CameraFollowingEntityIndex = EntityIndex;
  }
  
  return EntityIndex;
}

internal uint32
AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  uint32 EntityIndex = AddEntity(GameState, EntityType_Wall);
  entity Entity = GetEntity(GameState, EntityResidence_Dormant,  EntityIndex);
  
  Entity.Dormant->P.AbsTileX = AbsTileX;
  Entity.Dormant->P.AbsTileY = AbsTileY;
  Entity.Dormant->P.AbsTileZ = AbsTileZ;
  Entity.Dormant->Height = GameState->World->TileMap->TileSideInMeters;
  Entity.Dormant->Width = GameState->World->TileMap->TileSideInMeters;
  Entity.Dormant->Collides = true;
  
  return EntityIndex;
}


internal void 
SetCamera(game_state *GameState, tile_map_position NewCameraP) {
  
  tile_map *TileMap = GameState->World->TileMap;
  
  tile_map_difference dCameraP = Subtract(TileMap, &NewCameraP,  &GameState->CameraP); 
  GameState->CameraP = NewCameraP;
  v2 EntityOffsetForFrame = -dCameraP.dXY;
  
  uint32 TileSpanX = 17*3;
  uint32 TileSpanY = 9*3;
  
  rectangle2 CameraInBound = RectCenterDim(V2(0,0),
                                           TileMap->TileSideInMeters*V2((real32)TileSpanX,
                                                                        (real32)TileSpanY));
  
  for(uint32 EntityIndex = 1; EntityIndex < ArrayCount(GameState->HighEntity); ++EntityIndex) {
    
    if(GameState->EntityResidency[EntityIndex] == EntityResidence_High) {
      high_entity *High = GameState->HighEntity + EntityIndex;
      
      High->P += EntityOffsetForFrame;
      
      if(!IsInReactangle(CameraInBound, High->P)) {
        
        ChangeEntityResidence(GameState, EntityIndex, EntityResidence_Dormant);
      }
    }
  }
  // TODO(Egor): if entity is in bound, and not high, we should make it high
  
  uint32 MinTileX = NewCameraP.AbsTileX - TileSpanX/2;
  uint32 MinTileY = NewCameraP.AbsTileY - TileSpanY/2;
  uint32 MaxTileX = NewCameraP.AbsTileX + TileSpanX/2;
  uint32 MaxTileY = NewCameraP.AbsTileY + TileSpanY/2;
  
  for(uint32 EntityIndex = 1; EntityIndex < ArrayCount(GameState->DormantEntity); ++EntityIndex) {
    
    if(GameState->EntityResidency[EntityIndex] == EntityResidence_Dormant) {
      
      dormant_entity *DormantEntity = GameState->DormantEntity + EntityIndex;
      
      if((NewCameraP.AbsTileZ == DormantEntity->P.AbsTileZ) &&
         (DormantEntity->P.AbsTileX >= MinTileX) &&
         (DormantEntity->P.AbsTileY >= MinTileY) &&
         (DormantEntity->P.AbsTileX <= MaxTileX) &&
         (DormantEntity->P.AbsTileY <= MaxTileY))   {
        
        ChangeEntityResidence(GameState, EntityIndex, EntityResidence_High);
      }
    }
  }
}



extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
         (ArrayCount(Input->Controllers[0].Buttons)));
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  
  
  
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if(!Memory->IsInitialized)
  {
    
    AddEntity(GameState, EntityType_Null);
    // TODO(Egor): This may be more appropriate to do in the platform layer

    
    
    GameState->Backdrop = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//background.bmp");
    
    hero_bitmaps *Bitmap = GameState->Hero;
    
    Bitmap->HeroHead = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//figurine.bmp");
    Bitmap->HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_right.bmp");
    Bitmap->AlignX = 51;
    Bitmap->AlignY = 112;
    
    Bitmap[1] = Bitmap[0];
    Bitmap[1].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_up.bmp");
    
    Bitmap[2] = Bitmap[0];
    Bitmap[2].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_left.bmp");
    
    Bitmap[3] = Bitmap[0];
    Bitmap[3].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_down.bmp");
    
    
    

    
    
    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                    (uint8 *)Memory->PermanentStorage + sizeof(game_state));
    
    uint32 TilesPerHeight = 9;
    uint32 TilesPerWidth = 17;
    
    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;
    World->TileMap = PushStruct(&GameState->WorldArena, tile_map);
    
    
    tile_map *TileMap = World->TileMap;
    
    TileMap->ChunkShift = 4;
    TileMap->ChunkDim = (1 << TileMap->ChunkShift);
    TileMap->ChunkMask = TileMap->ChunkDim - 1;
    
    TileMap->TileChunkCountX = 128;
    TileMap->TileChunkCountY = 128;
    TileMap->TileChunkCountZ = 2;
    
    TileMap->TileSideInMeters = 1.4f;
    
    
    TileMap->TileChunks = PushArray(&GameState->WorldArena,
                                    TileMap->TileChunkCountZ*
                                    TileMap->TileChunkCountX*
                                    TileMap->TileChunkCountY,
                                    tile_chunk);
    
    #if 0
    uint32 EntityIndex = AddEntity(GameState);
    InitializePlayer(GameState, EntityIndex);
    #endif
    
    uint32 ScreenY = 0;
    uint32 ScreenX = 0;
    uint32 AbsTileZ = 0;
    
    bool32 DoorToRight = false;
    bool32 DoorToLeft = false;
    bool32 DoorToTop = false;
    bool32 DoorToBottom = false;    
    bool32 DoorUp = false;
    bool32 DoorDown = false;
    
    uint32 CounterOfTwo = 0;
    
    for(uint32 ScreenIndex = 0; ScreenIndex < 2; ++ScreenIndex) {
      
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
        if(AbsTileZ == 0) 
          DoorUp = true;
        else if(AbsTileZ == 1) 
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
          
          int32 AbsTileY = ScreenY*TilesPerHeight + TileY;
          int32 AbsTileX = ScreenX*TilesPerWidth + TileX;
          
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
          
          SetTileValue(&GameState->WorldArena, TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
         
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
        
        if(AbsTileZ == 1) 
          AbsTileZ = 0;
        else
          AbsTileZ = 1;
      }
      else if(RandomChoice == 0) {
        ScreenX++;
      }
      else if(RandomChoice == 1) {
        ScreenY++;
      }
      
    }
    
    tile_map_position NewCameraP = {};
    NewCameraP.AbsTileY = 9/2;
    NewCameraP.AbsTileX = 17/2;
    SetCamera(GameState, NewCameraP);
    
    Memory->IsInitialized = true;
  }
  
  world *World = GameState->World;
  tile_map *TileMap = World->TileMap;
  
  
  // TODO(Egor): put this into renderer in the future
  int32 TileSideInPixels = 60;
  real32 MetersToPixels = TileSideInPixels/TileMap->TileSideInMeters;
  
  // NOTE(Egor): save 
  
  
  for(int ControllerIndex = 0;
      ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
  {
    
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    entity ControllingEntity = GetEntity(GameState, EntityResidence_High, 
                                         GameState->PlayerIndexForController[ControllerIndex]);
    if(ControllingEntity.Residence != EntityResidence_NoneExistant) {
      
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
        if(Controller->Start.EndedDown) {
          ControllingEntity.High->dZ = 5.0f;
        }

          
      }
      
      MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddPlayer);
    }
    else {
      if(Controller->Back.EndedDown) {
        
        uint32 EntityIndex = AddPlayer(GameState);
        GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
      }
    }
  }
  
  v2 EntityOffsetForFrame = {};
  entity CameraFollowingEntity = GetEntity(GameState, EntityResidence_High, GameState->CameraFollowingEntityIndex);
  
  if(CameraFollowingEntity.Residence != EntityResidence_NoneExistant) {
    
    tile_map_position NewCameraP = GameState->CameraP;
    
#if 0
    
    NewCameraP.AbsTileZ = CameraFollowingEntity.Dormant->P.AbsTileZ;
    
    if(CameraFollowingEntity.High->P.X > (9.0f * TileMap->TileSideInMeters)) {
      NewCameraP.AbsTileX += 17;
    }
    if(CameraFollowingEntity.High->P.X < -(9.0f * TileMap->TileSideInMeters)) {
      NewCameraP.AbsTileX -= 17;
    }
    if(CameraFollowingEntity.High->P.Y > (5.0f * TileMap->TileSideInMeters)) {
      NewCameraP.AbsTileY += 9;
    }
    if(CameraFollowingEntity.High->P.Y < -(5.0f * TileMap->TileSideInMeters)) {
      NewCameraP.AbsTileY -= 9;
    }
    
#else
    
    NewCameraP.AbsTileZ = CameraFollowingEntity.Dormant->P.AbsTileZ;
    
    if(CameraFollowingEntity.High->P.X > (1.0f * TileMap->TileSideInMeters)) {
      NewCameraP.AbsTileX += 1;
    }
    if(CameraFollowingEntity.High->P.X < -(1.0f * TileMap->TileSideInMeters)) {
      NewCameraP.AbsTileX -= 1;
    }
    if(CameraFollowingEntity.High->P.Y > (1.0f * TileMap->TileSideInMeters)) {
      NewCameraP.AbsTileY += 1;
    }
    if(CameraFollowingEntity.High->P.Y < -(1.0f * TileMap->TileSideInMeters)) {
      NewCameraP.AbsTileY -= 1;
    }
#endif
    
    SetCamera(GameState, NewCameraP);

  }
  //
  // NOTE(Egor): rudimentary render starts below
  //
  
  
  
  
  //DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height,
  //              1.0f, 0.0f, 1.0f);
  DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);

  real32 CenterX = 0.5f*Buffer->Width;
  real32 CenterY = 0.5f*Buffer->Height;
  
#if 0
  for(int32 ViewRow = -6; ViewRow < 6; ++ViewRow) {
    for(int32 ViewColumn = -10; ViewColumn < 10; ++ViewColumn) {
      
      uint32 Column = GameState->CameraP.AbsTileX + ViewColumn;
      uint32 Row = GameState->CameraP.AbsTileY + ViewRow;
      
      //    uint32 TestColumn = Column & 255;
      //    uint32 TestRow = Row & 255;
      
      uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);
      real32 Gray = 0.5f;
      
      if(TileID > 1) {
        
        if(TileID == 2 || TileID == 3) {
          
          Gray = 1.0f;
        }
        
        if(TileID == 4 || TileID == 5) {
          Gray = 0.25f;
        }
        
        if(Row == GameState->CameraP.AbsTileY && Column == GameState->CameraP.AbsTileX) {
          Gray = 0.0f;
        }
        
        v2 TileSide = { 0.5f*TileSideInPixels, 0.5f*TileSideInPixels};
        v2 Cen = 
        { CenterX  + ((real32)ViewColumn * TileSideInPixels) - MetersToPixels*GameState->CameraP.Offset_.X,
          CenterY  - ((real32)ViewRow * TileSideInPixels) + MetersToPixels*GameState->CameraP.Offset_.Y };
        
        v2 Min = Cen - 0.9f*TileSide;
        v2 Max = Cen + 0.9f*TileSide;
        
        // NOTE: (Egor) in windows bitmap Y coord is growing downwards
        DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
        if(TileID == 3) {
          SetTileValue(&GameState->WorldArena, TileMap, Column, Row, GameState->CameraP.AbsTileZ, 2);
          DrawRectangle(Buffer, Cen - V2(6,6), Cen + V2(6,6), 1.0f, 0.0f, 0.0f);
          Gray = 1.0f;
        }
      }
    }
  }
  
#endif
  
  for(uint32 EntityIndex = 1; EntityIndex < GameState->EntityCount; ++EntityIndex) {
    
    if(GameState->EntityResidency[EntityIndex] == EntityResidence_High) {
      
      high_entity *HighEntity = &GameState->HighEntity[EntityIndex];
      dormant_entity *DormantEntity = &GameState->DormantEntity[EntityIndex];
      
      real32 ddZ = -9.8f;
      real32 dT = Input->dtForFrame;
      HighEntity->P += EntityOffsetForFrame;
      
      HighEntity->Z = 0.5f * ddZ * Square(dT) + HighEntity->dZ*dT + HighEntity->Z;
      HighEntity->dZ = ddZ*dT + HighEntity->dZ;

      if(HighEntity->Z < 0) {
        HighEntity->Z = 0;
      }
      real32 Z = -HighEntity->Z*MetersToPixels;
      
      
      real32 PlayerR = 1.0f;
      real32 PlayerG = 0.0f;
      real32 PlayerB = 0.0f;
      
      real32 PlayerGroundPointX = CenterX + HighEntity->P.X*MetersToPixels;
      real32 PlayerGroundPointY = CenterY - HighEntity->P.Y*MetersToPixels;
      
      v2 PlayerLeftTop = 
      { PlayerGroundPointX - 0.5f*DormantEntity->Width*MetersToPixels ,
        PlayerGroundPointY - 0.5f*DormantEntity->Height*MetersToPixels};
      
      v2 EntityWidthHeight = {DormantEntity->Width, DormantEntity->Height};
      
      if(DormantEntity->Type == EntityType_Hero) {
        
        DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MetersToPixels*EntityWidthHeight,
                      PlayerR, PlayerG, PlayerB); 
        
        hero_bitmaps *Hero = &GameState->Hero[HighEntity->FacingDirection];
        DrawBitmap(Buffer, &Hero->HeroTorso, PlayerGroundPointX, PlayerGroundPointY + Z, Hero->AlignX, Hero->AlignY);
        DrawBitmap(Buffer, &Hero->HeroHead, PlayerGroundPointX, PlayerGroundPointY + Z, Hero->AlignX, Hero->AlignY);
        DrawBitmap(Buffer, &Hero->HeroCape, PlayerGroundPointX, PlayerGroundPointY + Z, Hero->AlignX, Hero->AlignY);
      }
      else {
        
        DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MetersToPixels*EntityWidthHeight,
                      PlayerR, PlayerG, PlayerB); 
      }
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