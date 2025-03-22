// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

extern char end[]; // first address after kernel loaded from ELF file

int* allocated[512];
int numalloc = 0;
int freelistsize = 0;
// Initialize free list of physical pages.
void
kinit(void)
{
  char *p;

  initlock(&kmem.lock, "kmem");
  p = (char*)PGROUNDUP((uint)end);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || (uint)v >= PHYSTOP) 
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);
  for(int i = 0; i < numalloc; i++) {
    if(allocated[i] == (int*)v) {
      for(int j = i; j < numalloc-1;j++) {
        allocated[j] = allocated[j+1];
      }
      numalloc--;
      break;
    }
  }
  acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  freelistsize++;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *curr;
  struct run *prev;
  //struct run *head;
  int rand = xv6_rand();
  rand = rand % freelistsize;
  
  acquire(&kmem.lock);
  //head = kmem.freelist;
  curr = kmem.freelist;
  prev = kmem.freelist;
  if(curr) {
    //cprintf("%d\n",freelistsize);	  
    for(int i = 0; i < rand; i++) {
      prev = curr;
      curr = curr->next;
    }
    allocated[numalloc] = (int*)curr;
    if(rand == 0) {
      kmem.freelist = curr->next;
    }
    prev->next = curr->next;
    numalloc++;
    freelistsize--;
  }
  release(&kmem.lock);
  return (char*)curr;
}

//dumps allocated frames
//frames if array of allocated frames, numframes is number of frames to dump
int
dump_allocated(int *frames, int numframes)
{
  if(frames == NULL) {
    return -1;
  }
  if(numframes < 0) {
    return -1;
  }
  if(numframes > numalloc) {
    return -1;
  }
  for(int i = 0; i < numframes; i++) {
    frames[i] = (int)allocated[numframes-1-i];
  }
  return 0;
}
