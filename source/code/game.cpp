#include "game.h"
#include "game_world.cpp"
#include "game_sim_region.cpp"
#include "game_entity.cpp"



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




inline v2
GetCameraSpaceP(game_state *GameState, low_entity *LowEntity) {
  
  world_difference Diff = Subtract(GameState->World,
                                   &LowEntity->P, &GameState->CameraP);
  return Diff.dXY;
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
  LowEntity->Sim.Type = EntityType;
  
  // TODO(Egor): this is awfull
  if(P) {
    LowEntity->P = *P;
    ChangeEntityLocation(&GameState->WorldArena, GameState->World, LowEntityIndex, 0, P);
  }
  else {
    
    LowEntity->P = NullPosition();
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
  
  Entity.Low->Sim.Height = GameState->World->TileSideInMeters;
  Entity.Low->Sim.Width = GameState->World->TileSideInMeters;
  Entity.Low->Sim.Collides = true;
  
  return Entity;
}

inline void
InitHitPoints(low_entity *LowEntity, uint32 HitPointCount) {
  
  Assert(HitPointCount <= ArrayCount(LowEntity->Sim.HitPoint));
  
  LowEntity->Sim.HitPointMax = HitPointCount;
  
  for(uint32 i = 0; i < HitPointCount; ++i) {
    
    hit_point *HitPoint = LowEntity->Sim.HitPoint + i;
    HitPoint->Flags = 0;
    HitPoint->FilledAmount = SUB_HIT_POINT;
  }
  
}

internal add_low_entity_result
AddSword(game_state *GameState) {
  
  add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Sword, 0);
  
  Entity.Low->Sim.Height = 0.5f;
  Entity.Low->Sim.Width = 1.0f;
  Entity.Low->Sim.Collides = false;
  
  return Entity;
}

internal add_low_entity_result
AddPlayer(game_state *GameState) {
  
  world_position P = GameState->CameraP;
  add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Hero, &P);
  
  Entity.Low->Sim.Collides = true;
  Entity.Low->Sim.Height = 0.5f;
  Entity.Low->Sim.Width = 1.0f;
  InitHitPoints(Entity.Low, 3);
  
  add_low_entity_result Sword = AddSword(GameState);
  
  Entity.Low->Sim.Sword.Index = Sword.Index;
  
  
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
  
  Entity.Low->Sim.Height = 0.5f;
  Entity.Low->Sim.Width = 1.0f;
  Entity.Low->Sim.Collides = true;
  InitHitPoints(Entity.Low, 3);
  
  return Entity;
}



internal add_low_entity_result
AddFamiliar(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX,
                                                   AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Familiar, &P);
  
  Entity.Low->Sim.Height = 0.5f;
  Entity.Low->Sim.Width = 1.0f;
  Entity.Low->Sim.Collides = false;
  
  return Entity;
}

#if 0
internal void 
SimCameraRegion(game_state *GameState) {
  
  world *World = GameState->World;
  
  uint32 TileSpanX = 17*3;
  uint32 TileSpanY = 9*3;
  
  rectangle2 CameraBounds = RectCenterDim(V2(0,0),
                                          World->TileSideInMeters*V2((real32)TileSpanX,
                                                                     (real32)TileSpanY));
  sim_region *SimRegion = BeginSim(SimArena, GameState->World, GameState->CameraP, CameraBounds);
  EndSim(SimRegion, GameState);
  
}
#endif


inline void
PushPiece(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
          v2 Offset, real32 OffsetZ, real32 OffsetZC, v2 Align, v2 Dim, v4 Color) {
  
  Assert(Group->Count < ArrayCount(Group->Pieces));
  entity_visible_piece *Piece = Group->Pieces + Group->Count++;
  Piece->Bitmap = Bitmap;
  Piece->Offset = Group->GameState->MetersToPixels*V2(Offset.X, -Offset.Y) - Align;
  Piece->OffsetZ = Group->GameState->MetersToPixels*OffsetZ;
  Piece->OffsetZC = OffsetZC;
  
  Piece->Dim = Dim;
  
  Piece->R = Color.R;
  Piece->G = Color.G;
  Piece->B = Color.B;
  Piece->A = Color.A;
}

internal void 
PushBitmap(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
           v2 Offset, real32 OffsetZ, real32 OffsetZC, v2 Align, real32 A = 1.0f)
{
  
  PushPiece(Group, Bitmap, Offset, OffsetZ, OffsetZC,  Align, V2(0,0), V4(1.0f, 1.0f, 1.0f, A));
}

internal void 
PushRect(entity_visible_piece_group *Group,
         v2 Offset, real32 OffsetZ, real32 OffsetZC,
         v2 Dim, v4 Color)

{
  
  PushPiece(Group, 0, Offset, OffsetZ, OffsetZC, V2(0,0), Dim,
            Color);
}


internal void
DrawHitpoints(sim_entity *Entity, entity_visible_piece_group *PieceGroup) {
  
  if(Entity->HitPointMax) {
    
    v2 HitPointDim = {0.2f , 0.2f};
    real32 SpacingX = 2.0f * HitPointDim.X;
    real32 FirstX = (Entity->HitPointMax - 1)*0.5f*SpacingX;
    v2 HitP = {(Entity->HitPointMax - 1)*-0.5f*SpacingX, -0.4f};
    v2 dHitP = {SpacingX, 0};
    
    for(uint32 HPIndex = 0; HPIndex < Entity->HitPointMax; ++HPIndex) {
      
      hit_point *HitPoint = Entity->HitPoint + HPIndex;
      
      v4 Color = {1.0f , 0.0f, 0.0f, 1.0f};
      if(HitPoint->FilledAmount == 0) {
        
        Color = V4(0.2f, 0.2f, 0.2f, 1.0f);
      }
      
      PushRect(PieceGroup, HitP, 0.0f, 0.0f, HitPointDim, Color); 
      HitP += dHitP;
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
    
    AddLowEntity(GameState, EntityType_Null, 0);
    // TODO(Egor): This may be more appropriate to do in the platform layer
    
    
    
    GameState->Backdrop = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//background.bmp");
    GameState->Sword = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//dagger.bmp");
    GameState->Tree = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//tree.bmp");
    
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
    
    
    //    GameState->Tree = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//tree00.bmp");
    
    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                    (uint8 *)Memory->PermanentStorage + sizeof(game_state));
    
    uint32 TilesPerHeight = 9;
    uint32 TilesPerWidth = 17;
    
    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;
    InitializeWorld(World, 1.4f);
    
    int32 TileSideInPixels = 60;
    GameState->MetersToPixels = TileSideInPixels/World->TileSideInMeters;
    
    int32 ScreenBaseX = 0;
    int32 ScreenBaseY = 0;
    int32 ScreenBaseZ = 0;
    int32 ScreenY = ScreenBaseY;
    int32 ScreenX = ScreenBaseX;
    int32 AbsTileZ = ScreenBaseZ;
    
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
    AddFamiliar(GameState, CameraTileX, CameraTileY - 2, CameraTileZ);
    
    
    
    Memory->IsInitialized = true;
  }
  
  world *World = GameState->World;
  
  // TODO(Egor): put this into renderer in the future
  real32 MetersToPixels = GameState->MetersToPixels;
  
  for(int ControllerIndex = 0;
      ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
  {
    
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    controlled_entity *Controlled = GameState->ControlledEntities + ControllerIndex;
    
    if(Controlled->EntityIndex == 0) {
      
      if(Controller->Back.EndedDown) {
        
        Controlled = {};
        Controlled->EntityIndex = AddPlayer(GameState).Index;
      }
    }
    else {
      
      Controlled->ddP = {};
      if(Controller->IsAnalog)
      {
        // analog movement
        Controlled->ddP = v2{Controller->StickAverageX, Controller->StickAverageY};
      }
      else
      {
        
        if(Controller->MoveUp.EndedDown) {
          Controlled->ddP.Y = 1.0f;
        }
        if(Controller->MoveRight.EndedDown) {
          Controlled->ddP.X = 1.0f;
        }
        if(Controller->MoveDown.EndedDown) {
          Controlled->ddP.Y = -1.0f;
        }
        if(Controller->MoveLeft.EndedDown) {
          Controlled->ddP.X = -1.0f;
        }
      }
      
      if(Controller->Start.EndedDown) {
        Controlled->dZ = 5.0f;
      }
      
      Controlled->dSword = {};
      if(Controller->ActionUp.EndedDown) {
        
        Controlled->dSword = V2(0.0f, 1.0f);
      }
      if(Controller->ActionDown.EndedDown) {
        
        Controlled->dSword = V2(0.0f, -1.0f);
      }
      if(Controller->ActionLeft.EndedDown) {
        
        Controlled->dSword = V2(-1.0f, 0.0f);
      }
      if(Controller->ActionRight.EndedDown) {
        
        Controlled->dSword = V2(1.0f, 0.0f);
      }
    }
  }
  
  
  uint32 TileSpanX = 17*3;
  uint32 TileSpanY = 9*3;
  
  rectangle2 CameraBounds = RectCenterDim(V2(0,0),
                                          World->TileSideInMeters*V2((real32)TileSpanX,
                                                                     (real32)TileSpanY));
  
  
  memory_arena SimArena;
  InitializeArena(&SimArena, Memory->TransientStorageSize, Memory->TransientStorage);
  
  sim_region *SimRegion = BeginSim(&SimArena, GameState, GameState->World, GameState->CameraP, CameraBounds);
  
  
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
  PieceGroup.GameState = GameState;
  
  sim_entity *Entity = SimRegion->Entities;
  for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex) {
    
    PieceGroup.Count = 0;
    
    real32 dt = Input->dtForFrame;
    
    hero_bitmaps *Hero = &GameState->Hero[Entity->FacingDirection];    
    switch(Entity->Type) {
      
      case EntityType_Sword: {
        
        UpdateSword(SimRegion, Entity, dt);
        PushBitmap(&PieceGroup, &GameState->Sword, V2(0, 0), 0, 1.0f, V2(26, 37));
        
      } break;
      case EntityType_Hero: {
        
        for(uint32 Index = 0; Index < ArrayCount(GameState->ControlledEntities); ++Index) {
        
          controlled_entity *Controlled = GameState->ControlledEntities + Index;
          
          if(Controlled->EntityIndex == Entity->StorageIndex) {
            
            Entity->dZ = Controlled->dZ;
            
            move_spec MoveSpec = DefaultMoveSpec();
            MoveSpec.Speed = 50.0f;
            MoveSpec.Drag = 8.0f;
            MoveSpec.UnitMaxAccelVector = true;
            
            MoveEntity(SimRegion, Entity, Input->dtForFrame,
                       &MoveSpec, Controlled->ddP);
            
            if((Controlled->dSword.X != 0.0f) || (Controlled->dSword.Y != 0.0f)) {
              
              sim_entity *Sword = Entity->Sword.Ptr;
              if(Sword) {
                
                Sword->P = Entity->P;
                Sword->DistanceRemaining = 5.0f;
                Sword->dP = Controlled->dSword * 5.0f;
              }
            }
          }
        }
        
        PushBitmap(&PieceGroup, &Hero->HeroTorso, V2(0, 0), 0, 1.0f, Hero->Align);
        PushBitmap(&PieceGroup, &Hero->HeroHead, V2(0, 0), 0, 1.0f, Hero->Align);
        PushBitmap(&PieceGroup, &Hero->HeroCape, V2(0, 0), 0, 1.0f, Hero->Align);
        
        DrawHitpoints(Entity, &PieceGroup);
      } break;
      case EntityType_Wall: {
        
        PushBitmap(&PieceGroup, &GameState->Tree, V2(0, 0), 0, 1.0f, V2(30, 60));
      } break;
      case EntityType_Monster: {
        
        UpdateMonster(SimRegion, Entity, dt);
        PushBitmap(&PieceGroup, &Hero->HeroHead, V2(0, 0), 0, 1.0f, Hero->Align);
        DrawHitpoints(Entity, &PieceGroup);
      } break;
      case EntityType_Familiar: {
        
        
        UpdateFamiliar(SimRegion, Entity, dt);
        PushBitmap(&PieceGroup, &Hero->HeroHead, V2(0, 0), 1.0f, 0, Hero->Align);
      } break;
      default: {
        InvalidCodePath;
      }
    }  
    
    
    real32 ddZ = -9.8f;
    Entity->Z = 0.5f * ddZ * Square(dt) + Entity->dZ*dt + Entity->Z;
    Entity->dZ = ddZ*dt + Entity->dZ;
    
    if(Entity->Z < 0) {
      Entity->Z = 0;
    }
    real32 EntityZ = -Entity->Z*MetersToPixels;
    
    real32 EntityGroundPointX = CenterX + Entity->P.X*MetersToPixels;
    real32 EntityGroundPointY = CenterY - Entity->P.Y*MetersToPixels;
    
#if 0
    v2 PlayerLeftTop = 
    { PlayerGroundPointX - 0.5f*LowEntity->Width*MetersToPixels ,
      PlayerGroundPointY - 0.5f*LowEntity->Height*MetersToPixels}
    ;
    v2 EntityWidthHeight = {LowEntity->Width, LowEntity->Height};
    
    
#endif
    
    for(uint32 PieceIndex = 0; PieceIndex < PieceGroup.Count; ++PieceIndex) {
      
      entity_visible_piece *Piece = PieceGroup.Pieces + PieceIndex;
      
      v2 Cen = {
        EntityGroundPointX + Piece->Offset.X,
        EntityGroundPointY + Piece->Offset.Y + Piece->OffsetZ + EntityZ*Piece->OffsetZC
      };
      
      if(Piece->Bitmap) {
        
        DrawBitmap(Buffer, Piece->Bitmap, Cen.X, Cen.Y);
      }
      else {
        
        v2 HalfDim = Piece->Dim*MetersToPixels*0.5f;
        DrawRectangle(Buffer, Cen - HalfDim, Cen + HalfDim,
                      Piece->R, Piece->G, Piece->B); 
      }
      
    }
  }
  // NOTE(Egor): Ending the simulation
  EndSim(SimRegion, GameState);
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