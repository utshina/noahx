#include <stdbool.h>
#include <WinHvPlatform.h>

#define KiB *1L*1024
#define MiB *1L*1024*1024
#define GiB *1L*1024*1024*1024
#define PAGE_SIZE 4096

typedef uint64_t guest_virtuall_t;
typedef uint64_t guest_physical_t;

struct kernel {
	char pml4[PAGE_SIZE];
	char pdpt[PAGE_SIZE];
	char tss[PAGE_SIZE];
	char idt[PAGE_SIZE];
	uint64_t gdt[3];
};

typedef struct {
	WHV_PARTITION_HANDLE handle;
	UINT32 vcpu_id;
	WHV_RUN_VP_EXIT_CONTEXT exit_context;

	char *text;
	struct kernel *kernel;

	guest_physical_t text_guest_physical;
	uint64_t text_size;

	guest_physical_t kernel_start_guest_physical;
	uint64_t kernel_size;
} vm_t;

void vmm_init(void);
void vmm_create_vm(vm_t *vm);
void vmm_setup_vm(vm_t *vm);
void vmm_create_vcpu(vm_t *vm);
bool vmm_run(vm_t *vm);

static const uint64_t CR0_PE = 1ULL << 0;
static const uint64_t CR0_PG = 1ULL << 31;

static const uint64_t CR4_PSE = 1ULL << 4;
static const uint64_t CR4_PAE = 1ULL << 5;
static const uint64_t CR4_PGE = 1ULL << 7;

static const uint64_t EFER_LME = 1ULL << 8;
static const uint64_t EFER_LMA = 1ULL << 10;
