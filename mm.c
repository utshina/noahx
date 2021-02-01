#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include "panic.h"
#include "mm.h"
#include "thread.h"
#include "range.h"

static inline uint64_t
poffset(void *p1, void *p2)
{
	return (uint64_t)p2 - (uint64_t)p1;
}

static inline vm_gphys_t
gvirt_to_gphys(mm_gvirt_t gvirt)
{
	return (vm_gphys_t)gvirt;
}

static inline vm_gphys_t
kernel_hvirt_to_gphys(mm_t *mm, void *hvirt)
{
	return gvirt_to_gphys(mm->kernel_start) + poffset(mm->kernel, hvirt);
}

static mm_mmap_range_t *
alloc_mmap_range(mm_gvirt_t start, void *hvirt, size_t size, mm_mmap_type_t type)
{
	mm_mmap_range_t *mmap_range = (mm_mmap_range_t *)malloc(sizeof(mm_mmap_range_t));
	if (mmap_range == NULL)
		panic("out of memory");
	range_init(&mmap_range->range,  start, start + size);
	mmap_range->hvirt = hvirt;
	mmap_range->size = size;
	mmap_range->type = type;
	return mmap_range;
}

static inline range_root_t
get_mmap_range_root(mm_t *mm)
{
	return (range_root_t)&mm->mmap_range_root;
}

void
mm_mmap(mm_t *mm, mm_gvirt_t gvirt, void *hvirt, size_t size, mm_mmap_prot_t prot, mm_mmap_type_t type)
{
	assert(gvirt < mm->kernel_start);

	vm_mmap(mm->vm, gvirt_to_gphys(gvirt), hvirt, size, (vm_mmap_prot_t)prot);

	mm_mmap_range_t *mmap_range = alloc_mmap_range(gvirt, hvirt, size, type);
	range_insert(get_mmap_range_root(mm), &mmap_range->range);
}


static void *
range_to_hvirt(range_t *r, mm_gvirt_t gvirt)
{
	mm_mmap_range_t *mmap_range = (mm_mmap_range_t *)r;
	return (char *)mmap_range->hvirt + (gvirt - mmap_range->range.start);
}

void *
mm_gvirt_to_hvirt(mm_t *mm, mm_gvirt_t gvirt)
{
	range_t *r = range_search_one(get_mmap_range_root(mm), gvirt);
	if (r == NULL)
		return NULL;
	return range_to_hvirt(r, gvirt);
}

bool
mm_copy_from_user(mm_t *mm, void *dst, mm_gvirt_t src, size_t size)
{
	range_t *r = range_search_one(get_mmap_range_root(mm), src);
	if (r == NULL)
		return false;
	if (r->end < src + size)
		panic("cross the page boundary");
	void *src_hvirt = range_to_hvirt(r, src);
	memcpy(dst, src_hvirt, size);
	return true;
}

bool
mm_copy_to_user(mm_t *mm, mm_gvirt_t dst, void *src, size_t size)
{
	range_t *r = range_search_one(get_mmap_range_root(mm), dst);
	if (r == NULL)
		return false;
	if (r->end < dst + size)
		panic("cross the page boundary");
	void *dst_hvirt = range_to_hvirt(r, dst);
	memcpy(dst_hvirt, src, size);
	return true;
}

void
mm_dump_range_tree(mm_t *mm)
{
	range_print(get_mmap_range_root(mm));
}

void
mm_get_sysregs(mm_t *mm, vcpu_sysregs_t *sysregs)
{
	sysregs->cr3 = kernel_hvirt_to_gphys(mm, mm->kernel->pml4);
	sysregs->gdt_base = kernel_hvirt_to_gphys(mm, mm->kernel->gdt);
	sysregs->gdt_limit = 0x17;
	sysregs->idt_base = kernel_hvirt_to_gphys(mm, mm->kernel->idt);
	sysregs->idt_limit = PAGE_SIZE_4K - 1;
	sysregs->tss_base = kernel_hvirt_to_gphys(mm, mm->kernel->tss);
	sysregs->tss_limit = PAGE_SIZE_4K - 1; // 0xffff ?
}

mm_gvirt_t
mm_get_stack_top(mm_t *mm)
{
	return mm->stack_top;
}

int
mm_expand_heap(mm_t *mm, mm_gvirt_t new_heap_end)
{
	new_heap_end = roundup(new_heap_end, PAGE_SIZE_4K);
	assert(new_heap_end > mm->heap_end);
	size_t size_diff = new_heap_end - mm->heap_end;
	void *heap = mmap(NULL, size_diff, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (!heap)
		panic("out of memory");
	mm_mmap(mm, mm->heap_end, heap, size_diff,
		MM_MMAP_PROT_READ | MM_MMAP_PROT_WRITE,
		MM_MMAP_TYPE_ALLOC);
	mm->heap_size += size_diff;
	mm->heap_end = new_heap_end;
	return new_heap_end;
}

void
mm_setup_heap(mm_t *mm, mm_gvirt_t heap_start)
{
	mm->heap_size = PAGE_SIZE_4K;
	mm->heap_start = heap_start;
	mm->heap_end = heap_start + mm->heap_size;
	mm->heap = mmap(NULL, mm->heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (!mm->heap)
		panic("out of memory");
	mm_mmap(mm, mm->heap_start, mm->heap, mm->heap_size,
		MM_MMAP_PROT_READ | MM_MMAP_PROT_WRITE,
		MM_MMAP_TYPE_ALLOC);
}

void
mm_setup_stack(mm_t *mm)
{
	mm->stack_size = 64 KiB;
	mm->stack_top = mm->kernel_start;
	mm->stack_bottom = mm->stack_top - mm->stack_size;
	mm->stack = mmap(NULL, mm->stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (!mm->stack)
		panic("out of memory");
	mm_mmap(mm, mm->stack_bottom, mm->stack, mm->stack_size,
		MM_MMAP_PROT_READ | MM_MMAP_PROT_WRITE,
		MM_MMAP_TYPE_ALLOC);
}

void
mm_setup_kernel(mm_t *mm)
{
	mm->kernel_start = 255 GiB;
	mm->kernel_size = roundup(sizeof(*mm->kernel), PAGE_SIZE_4K);
	mm->kernel = (__typeof(mm->kernel))aligned_alloc(PAGE_SIZE_4K, mm->kernel_size);
	if (!mm->kernel)
		panic("out of memory");
	memset(mm->kernel, 0, mm->kernel_size);
	vm_mmap(mm->vm, gvirt_to_gphys(mm->kernel_start),
		mm->kernel, mm->kernel_size,
		VM_MMAP_PROT_READ | VM_MMAP_PROT_WRITE);

	const uint64_t PTE_P  = 1ULL << 0;
	const uint64_t PTE_RW = 1ULL << 1;
	const uint64_t PTE_US = 1ULL << 2;
	const uint64_t PTE_PS = 1ULL << 7;
	const uint64_t PML4E_FLAG = PTE_P | PTE_RW | PTE_US;
	const uint64_t PDPTE_FLAG = PTE_P | PTE_RW | PTE_US | PTE_PS;
	mm->kernel->pml4[0] = kernel_hvirt_to_gphys(mm, mm->kernel->pdpt) | PML4E_FLAG;
	const uint64_t pdpte_count = mm->kernel_start / (1 GiB);
	for (uint64_t i = 0; i < pdpte_count; i++)
		mm->kernel->pdpt[i] = i GiB | PDPTE_FLAG;

	mm->kernel->gdt[0] = 0;
	mm->kernel->gdt[1] = 0x00a0fa000000ffff; // user code
	mm->kernel->gdt[2] = 0x00c0f2000000ffff; // user data
}


void
mm_init(mm_t *mm, vm_t *vm)
{
	mm->vm = vm;
	mm->mmap_range_root = NULL;
}
