#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "vm.h"
#include "range.h"

#define KiB *1L*1024
#define MiB *1L*1024*1024
#define GiB *1L*1024*1024*1024
#define PAGE_SIZE_4K 4096

typedef uint64_t mm_gvirt_t;

typedef enum {
	MM_MMAP_TYPE_ALLOC,
	MM_MMAP_TYPE_MMAP,
} mm_mmap_type_t;

typedef enum {
	MM_MMAP_PROT_READ = VM_MMAP_PROT_READ,
	MM_MMAP_PROT_WRITE = VM_MMAP_PROT_WRITE,
	MM_MMAP_PROT_EXEC = VM_MMAP_PROT_EXEC,

	MM_MMAP_PROT_RE = MM_MMAP_PROT_READ | MM_MMAP_PROT_EXEC,
	MM_MMAP_PROT_RW = MM_MMAP_PROT_READ | MM_MMAP_PROT_WRITE,
	MM_MMAP_PROT_RWE = MM_MMAP_PROT_READ | MM_MMAP_PROT_WRITE | MM_MMAP_PROT_EXEC,
} mm_mmap_prot_t;

typedef struct {
	range_t range;
	char *hvirt;
	uint64_t size;
	mm_mmap_type_t type;
} mm_mmap_range_t;

typedef struct {
	vm_t *vm;

	mm_mmap_range_t *mmap_range_root;

	mm_gvirt_t stack_top, stack_bottom;
	size_t stack_size;
	void *stack;

	mm_gvirt_t heap_start, heap_end;
	size_t heap_size;
	void *heap;

	mm_gvirt_t kernel_start;
	uint64_t kernel_size;
	#define PAGE_ENTRY_COUNT 512
	struct kernel {
		uint64_t pml4[PAGE_ENTRY_COUNT];
		uint64_t pdpt[PAGE_ENTRY_COUNT];
		char tss[PAGE_SIZE_4K];
		char idt[PAGE_SIZE_4K];
		uint64_t gdt[3];
	} *kernel;
} mm_t;


void
mm_init(mm_t *mm, vm_t *vm);

void
mm_setup_kernel(mm_t *mm);

void
mm_setup_stack(mm_t *mm);

mm_gvirt_t
mm_get_stack_top(mm_t *mm);

void
mm_setup_heap(mm_t *mm, mm_gvirt_t heap_start);

int
mm_expand_heap(mm_t *mm, mm_gvirt_t new_heap_end);

void
mm_get_sysregs(mm_t *mm, vcpu_sysregs_t *sysregs);

void
mm_push(mm_t *mm, const void *data, size_t n);

void
mm_mmap(mm_t *mm, mm_gvirt_t gvirt, void *hvirt, size_t size, mm_mmap_prot_t prot, mm_mmap_type_t type);

void *
mm_gvirt_to_hvirt(mm_t *mm, mm_gvirt_t gvirt);

bool
mm_copy_from_user(mm_t *mm, void *dst, mm_gvirt_t src, size_t size);

bool
mm_copy_to_user(mm_t *mm, mm_gvirt_t dst, void *src, size_t size);

void
mm_dump_range_tree(mm_t *mm);

static inline void *
mm_stack_gvirt_to_hvirt(mm_t *mm, mm_gvirt_t gvirt)
{
	return mm->stack + (gvirt - mm->stack_bottom);
}

#if 0
static inline mm_gphys_t
mm_stack_hvirt_to_gphys(vm_t *vm, void *hvirt)
{
	return (uint64_t)hvirt - (uint64_t)vm->stack + vm->stack_top_gphys - vm->stack_size;
}
#endif
