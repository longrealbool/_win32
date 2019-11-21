internal v2
ConvertToBottomUpAlign(loaded_bitmap *Bitmap, v2 Align) {
  
  Align.y = (Bitmap->Height - 1) - Align.y;
  return Align;
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
DEBUGLoadBMP(char *FileName, int32 AlignX, int32 AlignY)
{
  
  loaded_bitmap Result = {};
  debug_read_file_result ReadResult = DEBUGReadEntireFile(FileName);
  
  if(ReadResult.ContentsSize != 0) {
    
    bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
    
    // NOTE(Egor, bitmap_format): be aware that bitmap can have negative height for
    // top-down pictures, and there can be compression
    uint32 *Pixel = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
    Result.Memory = Pixel;
    Result.Height = Header->Height;
    Result.Width = Header->Width;
    
    
    Assert(Result.Height > 0);
    
    // NOTE(Egor): compute the ratios for eradicating meters to pixels from pipeline
    Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);
    // NOTE(Egor): this is constant, and it should stay this way
    real32 PtM = 1.0f / 42.0f;
//    Result.NativeHeight = Result.Height * PtM;
    
    
    // NOTE(Egor): alignment
    v2 Align = V2i(AlignX, (Result.Height - 1) - AlignY);
    v2 AlignPercentage;
    AlignPercentage.x = SafeRatio0(Align.x, (real32)Result.Width); 
    AlignPercentage.y = SafeRatio0(Align.y, (real32)Result.Height);
    Result.AlignPercentage = AlignPercentage;
    
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
    
    int32 AlphaShiftUp = 24;
    int32 AlphaShiftDown = (int32)AlphaScan.Index;
    int32 RedShiftUp = 16;
    int32 RedShiftDown = (int32)RedScan.Index;
    int32 GreenShiftUp = 8;
    int32 GreenShiftDown = (int32)GreenScan.Index;
    int32 BlueShiftUp = 0;
    int32 BlueShiftDown = (int32)BlueScan.Index;
    
    Assert(AlphaScan.Found);    
    Assert(RedScan.Found);
    Assert(GreenScan.Found);
    Assert(BlueScan.Found);
    
    uint32 *SourceDest = Pixel;
    
    for(int32 Y = 0; Y < Header->Height; ++Y) {
      for(int32 X = 0; X < Header->Width; ++X) {
        
        uint32 C = *SourceDest;
        
        v4 Texel = V4((real32)((C & RedMask) >> RedShiftDown),
                      (real32)((C & GreenMask) >> GreenShiftDown),
                      (real32)((C & BlueMask) >> BlueShiftDown),
                      (real32)((C & AlphaMask) >> AlphaShiftDown));
        
        // NOTE(Egor): when we will grab this data we want to get our color's in 
        // premultiplied format, so we store them in:
        
        // sRGB (non-premultiplied) --to_linear--> LinearRGB * LinearAlpha
        // --to_sRGB--> sRGB (premultiplied)
        
        Texel = SRGB255ToLinear1(Texel);
        
        Texel.rgb *= Texel.a;
        
        Texel = Linear1ToSRGB255(Texel);
        // when we will take it back from sRGB space to linear
        // they were already in premultiplied format
        
        *SourceDest = (((uint32)(Texel.a + 0.5f) << 24) |
                       ((uint32)(Texel.r + 0.5f) << 16) |
                       ((uint32)(Texel.g + 0.5f) << 8)  |
                       ((uint32)(Texel.b + 0.5f) << 0));
        
        SourceDest++;
      }
    }
    
    
    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
#if 0
    
    // NOTE(Egor): bitmap was loaded bottom to top, so we reverse pitch (make him negative),
    // and shift Memory ptr to top of the image (bottom of the array)
    Result.Memory = (uint8 *)Result.Memory + (Result.Height-1)*(Result.Pitch);
    Result.Pitch = -Result.Width*BITMAP_BYTES_PER_PIXEL;
#endif
  }
  
  return Result;
}

internal loaded_bitmap
DEBUGLoadBMP( char *FileName) {
  
  loaded_bitmap Result = DEBUGLoadBMP(FileName, 0, 0);
  Result.AlignPercentage = V2(0.5f, 0.5f);
  return Result;
  
}


internal game_assets *
AllocateGameAssets(memory_arena *Arena, uint32 Size,
                   transient_state *TranState) {
  
  game_assets *Assets = PushStruct(Arena, game_assets);
  SubArena(&Assets->Arena, Arena, Size); 
  Assets->TranState = TranState;
  
  Assets->BitmapCount = AID_Count;
  Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);
  
  loaded_bitmap *Stones = Assets->Stones;
  loaded_bitmap *Grass = Assets->Grass;
  loaded_bitmap *Tuft = Assets->Tuft;
  
  Grass[0] = DEBUGLoadBMP(  "..//..//test//grass00.bmp");
  Grass[1] = DEBUGLoadBMP(  "..//..//test//grass01.bmp");
  Stones[0] = DEBUGLoadBMP(  "..//..//test//ground00.bmp");    
  Stones[1] = DEBUGLoadBMP(  "..//..//test//ground01.bmp");    
  Stones[2] = DEBUGLoadBMP(  "..//..//test//ground02.bmp");    
  Stones[3] = DEBUGLoadBMP(  "..//..//test//ground03.bmp");    
  Tuft[0] = DEBUGLoadBMP(  "..//..//test//tuft00.bmp");    
  Tuft[1] = DEBUGLoadBMP(  "..//..//test//tuft00.bmp");    
  Tuft[2] = DEBUGLoadBMP(  "..//..//test//tuft00.bmp");    
  
  hero_bitmaps *Bitmap = Assets->Hero;
  
  Bitmap->HeroHead = DEBUGLoadBMP("..//source//assets//figurine.bmp", 51, 112);
  Bitmap->HeroCape = DEBUGLoadBMP("..//source//assets//arrow_right.bmp", 51, 112);
  Bitmap->Align = ConvertToBottomUpAlign(&Bitmap->HeroHead,V2(51, 112));
  
  Bitmap[1] = Bitmap[0];
  Bitmap[1].HeroCape = DEBUGLoadBMP("..//source//assets//arrow_up.bmp", 51, 112);
  
  Bitmap[2] = Bitmap[0];
  Bitmap[2].HeroCape = DEBUGLoadBMP("..//source//assets//arrow_left.bmp", 51, 112);
  
  Bitmap[3] = Bitmap[0];
  Bitmap[3].HeroCape = DEBUGLoadBMP("..//source//assets//arrow_down.bmp", 51, 112);
  
  return Assets;
};






struct load_bitmap_work {
  
  game_assets *Assets;
  loaded_bitmap *Bitmap;
  bitmap_id ID;
  asset_state FinalState;
  char *FileName;
  bool32 HasAlignment;
  int32 AlignX;
  int32 AlignY;
  
  task_with_memory *Task;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork) {
  
  
  load_bitmap_work *Work = (load_bitmap_work *)Data;  
  Assert(Work);
  
  
  if(Work->HasAlignment) {
    
    *Work->Bitmap = DEBUGLoadBMP(Work->FileName, Work->AlignX, Work->AlignY);
  }
  else {
    
    *Work->Bitmap = DEBUGLoadBMP(Work->FileName);
  }
  
  WRITE_BARRIER;
  
  Work->Assets->Bitmaps[Work->ID.Value].Bitmap = Work->Bitmap;
  Work->Assets->Bitmaps[Work->ID.Value].State = Work->FinalState;
  
  EndTask(Work->Task);
}


internal void
LoadBitmap(game_assets *Assets, bitmap_id ID) {
  
  if(CompareExchangeUInt32((uint32 *)&Assets->Bitmaps[ID.Value].State, GAS_Unloaded, GAS_Queued) ==
     GAS_Unloaded) {
    
    task_with_memory *Task = BeginTask(Assets->TranState);
    
    if(Task) {
      
      load_bitmap_work *Work = PushStruct(&Task->Arena, load_bitmap_work);
      
      Work->Assets = Assets;
      Work->ID = ID;
      Work->FileName = "";
      Work->Task = Task;
      Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
      Work->HasAlignment = false;
      Work->FinalState = GAS_Loaded;
      
      switch(ID.Value) {
        
        case AID_Backdrop: {
          
          Work->FileName = "..//source//assets//background.bmp";
        } break;
        
        case AID_Sword: {
          
          Work->FileName = "..//source//assets//dagger.bmp";      
        } break;
        
        case AID_Tree: {
          
          Work->FileName = "..//..//test//tree00.bmp";
        } break;
        
        InvalidDefaultCase;
      }
      
      PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadBitmapWork, Work);
    }
  }
}

internal loaded_bitmap *
GetBitmap(game_assets *Assets, bitmap_id ID) {
  
  loaded_bitmap *Result = Assets->Bitmaps[ID.Value].Bitmap;
  return Result;
};

internal bitmap_id
GetFirstBitmapID(game_assets *Assets, asset_type_id ID) {
  
  bitmap_id Result = {(uint32)ID};
  return Result;
};



  
  
  
  