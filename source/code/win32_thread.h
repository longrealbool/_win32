// TODO(Egor): double-check the write ordering on CPU
#define WRITE_BARRIER _WriteBarrier(); _mm_sfence()
#define READ_BARRIER _ReadBarrier()


struct platform_work_queue_entry {
  
  platform_work_queue_callback *Callback;
  void *Data;
};


struct platform_work_queue {
  
  uint32 volatile WorksFinished;
  uint32 volatile NextEntryToExecute;
  uint32 volatile EntryCount;
  
  uint32 MaxEntryCount;
  HANDLE Semaphore;
  
  platform_work_queue_entry Entries[1024];
};




// TODO(Egor): move code below to .cpp file


// NOTE(Egor): this is how we add entry to the list
void Win32AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data) {
  
  Assert(Queue->EntryCount < ArrayCount(Queue->Entries));
  
  platform_work_queue_entry *Entry = Queue->Entries + Queue->EntryCount;
  Entry->Data = Data;
  Entry->Callback = Callback;
  WRITE_BARRIER;
  
  ++Queue->EntryCount;
  ReleaseSemaphore(Queue->Semaphore, 1, 0);
}


internal bool32
Win32DoNextQueueEntry(platform_work_queue *Queue) {
  
  bool32 GoToSleep = true;
  
  uint32 OriginalNextEntryToExecute = Queue->NextEntryToExecute;
  if(OriginalNextEntryToExecute < Queue->EntryCount) {
    
    // NOTE(Egor): if another thread beat this thread for increment, just do nothing
    uint32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToExecute,
                                              OriginalNextEntryToExecute + 1,
                                              OriginalNextEntryToExecute);
    
    if(Index == OriginalNextEntryToExecute) {
      
      READ_BARRIER;
      InterlockedIncrement((LONG volatile *)&Queue->WorksFinished);
      
      GoToSleep = false;
    }
  }
  
  return GoToSleep;
}

internal void Win32CompleteAllWork(platform_work_queue *Queue) {
  
  while(Queue->WorksFinished != Queue->EntryCount) {
    
    Win32DoNextQueueEntry(Queue);
  }
}


struct win32_thread_info {
  
  platform_work_queue *Queue;
  uint32 LogicalThreadIndex;
};

DWORD WINAPI 
ThreadProc(LPVOID lpParamer) {
  
  win32_thread_info *ThreadInfo = (win32_thread_info *)lpParamer;
  
  for(;;) {
    
    if(Win32DoNextQueueEntry(ThreadInfo->Queue))
      WaitForSingleObjectEx(ThreadInfo->Queue->Semaphore, INFINITE, false);
    
  }
}


