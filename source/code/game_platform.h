#if !defined(GAME_PLATFORM_H)
/* ========================================================================
   ======================================================================== */

#ifdef __cplusplus
extern "C" {
#endif
  
#if !defined(COMPILER_MSVC) 
#define COMPILER_MSVC 0
#endif
  
#if !defined(COMPILER_LLVM) 
#define COMPILER_LLVM 0
#endif
  
#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC 
#define COMPILER_MSVC 1
#else
  // TODO(Egor): Right now we just assume that this is LLVM
  // we need to make a real check in the future
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif
  
  
#if COMPILER_MSVC
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif
  
  
#define internal static 
#define local_persist static 
#define global_variable static
  
#define Pi32 3.14159265359f
  
  
#if GAME_SLOW
  // TODO(Egor): Complete assertion macro 
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif
  
#define Align16(Value) ((Value + 0xF) & ~0xF)
  
#define InvalidCodePath Assert(!"Invalid Code Path");
#define InvalidDefaultCase default: {InvalidCodePath;} break
  
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
  
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
  // TODO(Egor): swap, min, max ... macros???
  
#define Min(A, B) (A < B ? A : B)
#define Max(A, B) (A > B ? A : B)
  
  
  
  // TODO(Egor): double-check the write ordering on CPU
#define WRITE_BARRIER _WriteBarrier(); _mm_sfence()
#define READ_BARRIER _ReadBarrier()
#define READ_WRITE_BARRIER _ReadWriteBarrier(); _mm_fence()
  
  
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

  
  typedef int8_t int8;
  typedef int16_t int16;
  typedef int32_t int32;
  typedef int64_t int64;
  typedef int32 bool32;
  
  typedef uint8_t uint8;
  typedef uint16_t uint16;
  typedef uint32_t uint32;
  typedef uint64_t uint64;
  
  typedef intptr_t intptr;
  typedef uintptr_t uintptr;
  
  typedef size_t memory_index;
  
  typedef float real32;
  typedef double real64;
  
#define real32Maximum FLT_MAX
  
  
  
  /*
      NOTE(Egor): Services that the platform layer provides to the game
  */
#if GAME_INTERNAL
  /* IMPORTANT(Egor):
  
     These are NOT for doing anything in the shipping game - they are
     blocking and the write doesn't protect against lost data!
  */
  typedef struct debug_read_file_result
  {
    uint32 ContentsSize;
    void *Contents;
  } debug_read_file_result;
  
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
  typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
  
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *Filename)
  typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
  
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *Filename, uint32 MemorySize, void *Memory)
  typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
  
  typedef struct debug_cycle_counter {
    
    uint64 CycleCount;
    uint32 HitCount;
    
  } debug_cycle_counter;
  
  enum {
    /* 0 */DebugCycleCounter_GameUpdateAndRender,
    /* 1 */DebugCycleCounter_RenderPushBuffer,
    /* 2 */DebugCycleCounter_DrawRectangleSlowly,
    /* 4 */DebugCycleCounter_ProcessPixel,
    
    DebugCycleCounter_Count, // NOTE(Egor): this should be the last
  };
  
  extern struct game_memory *DebugGlobalMemory;
  
#if _MSC_VER
  
#define BEGIN_TIMED_BLOCK(ID) uint64 StartCycleCount##ID = __rdtsc();
#define END_TIMED_BLOCK(ID) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID;  ++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount;
#define END_TIMED_BLOCK_COUNTED(ID, COUNT) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID;  DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount += COUNT;

#else
#endif
  
#endif
  
  /*
    NOTE(Egor): Services that the game provides to the platform layer.
    (this may expand in the future - sound on separate thread, etc.)
  */
  
  // FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
  
  // TODO(Egor): In the future, rendering _specifically_ will become a three-tiered abstraction!!!
#define BITMAP_BYTES_PER_PIXEL 4
  typedef struct game_offscreen_buffer
  {
    // NOTE(Egor): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
  } game_offscreen_buffer;
  
  typedef struct game_sound_output_buffer
  {
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
  } game_sound_output_buffer;
  
  typedef struct game_button_state
  {
    int HalfTransitionCount;
    bool32 EndedDown;
  } game_button_state;
  
  typedef struct game_controller_input
  {
    bool32 IsConnected;
    bool32 IsAnalog;    
    real32 StickAverageX;
    real32 StickAverageY;
    
    union
    {
      game_button_state Buttons[12];
      struct
      {
        game_button_state MoveUp;
        game_button_state MoveDown;
        game_button_state MoveLeft;
        game_button_state MoveRight;
        
        game_button_state ActionUp;
        game_button_state ActionDown;
        game_button_state ActionLeft;
        game_button_state ActionRight;
        
        game_button_state LeftShoulder;
        game_button_state RightShoulder;
        
        game_button_state Back;
        game_button_state Start;
        
        // NOTE(Egor): All buttons must be added above this line
        
        game_button_state Terminator;
      };
    };
  } game_controller_input;
  
  typedef struct game_input
  {
    game_button_state MouseButtons[5];
    int32 MouseX, MouseY, MouseZ;
    
    bool32 ExecutableReloaded;
    real32 dtForFrame;
    
    game_controller_input Controllers[5];
  } game_input;
  
  

  // NOTE(Egor): work queue callback pointer declaration and defining body macro
  struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *Queue, void *Data)
  typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);
  
  typedef void platform_add_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
  typedef void platform_complete_all_work(platform_work_queue *Queue);
  
  typedef struct game_memory
  {
    bool32 IsInitialized;
    
    uint64 PermanentStorageSize;
    void *PermanentStorage; // NOTE(Egor): REQUIRED to be cleared to zero at startup
    
    uint64 TransientStorageSize;
    void *TransientStorage; // NOTE(Egor): REQUIRED to be cleared to zero at startup
    
    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
    
    platform_work_queue *RenderQueue;
    platform_work_queue *LowPriorityQueue;
    
    platform_add_entry *PlatformAddEntry;
    platform_complete_all_work *PlatformCompleteAllWork;
    
    
    
#if GAME_INTERNAL
    
    debug_cycle_counter Counters[DebugCycleCounter_Count];
#endif
    
    
  } game_memory;
  
#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
  typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
  
  // NOTE(Egor): At the moment, this has to be a very fast function, it cannot be
  // more than a millisecond or so.
  // TODO(Egor): Reduce the pressure on this function's performance by measuring it
  // or asking about it, etc.
#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
  typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
  
  inline uint32 SafeTruncateUInt64(uint64 Value) {
    // TODO(Egor): Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return(Result);
  }
  
  inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
  {
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
  }
  
  
#ifdef __cplusplus
}
#endif

#define GAME_PLATFORM_H
#endif
