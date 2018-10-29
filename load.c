#include <stdio.h>
#include <stdnoreturn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <elf.h>
#include "vmm.h"

#define OR ?:

typedef struct {
	Elf64_Ehdr *ehdr;
	uint64_t global_offset;
	uint64_t load_base;
} elf_t;

noreturn static int
panic(char *msg)
{
	fprintf(stderr, "loader: ");
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

void
map_file(elf_t *elf, vm_t *vm, char *filename)
{
	int fd = open(filename, O_RDONLY, 0);
	if (fd < 0)
		panic("can't open file");

	struct stat stat;
	fstat(fd, &stat) == 0 OR panic("fstat");

	void *data = mmap(NULL, stat.st_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED)
		panic("mmap");

	Elf64_Ehdr *ehdr = data;
	uint8_t valid_ident[] = {0x7f, 'E', 'L', 'F', 0x02, 0x01, 0x01};
	memcmp(ehdr->e_ident, valid_ident, sizeof(valid_ident)) == 0
		OR panic("ELF header is invalid");

	if (!(ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN))
		panic("ELF is not a supported type");
	if (ehdr->e_machine != EM_X86_64)
		panic("ELF is not an x64 executable");

	vm->regs.rip = ehdr->e_entry;
	printf("entry: %llx\n", vm->regs.rip);
	int n = ehdr->e_phnum;
	Elf64_Phdr *phdr = data + ehdr->e_phoff;
	for (int i = 0; i < n; i++, phdr++) {
		printf("p_type: %d\n", phdr->p_type);
		switch (phdr->p_type) {
		case PT_LOAD: {
			void *segment = (void *)rounddown((uint64_t)data + phdr->p_offset, PAGE_SIZE_4K);
			vmm_gphys_t vaddr = rounddown(phdr->p_vaddr, PAGE_SIZE_4K);
			uint64_t size = roundup(vaddr + phdr->p_memsz, PAGE_SIZE_4K) - vaddr;
			vmm_mmap(vm, segment, vaddr, size, VMM_MMAP_PROT_RWE);
			if (elf->load_base == 0)
				elf->load_base = phdr->p_vaddr - phdr->p_offset + elf->global_offset;
			vm->heap_gvirt = roundup(max(vm->heap_gvirt, vaddr + size), PAGE_SIZE_4K);
			printf("data: %p, offset:%x\n", data, phdr->p_offset);
			printf("%p: %02x\n", segment, ((char *)segment)[0x950]);
			break;
		}

		case PT_INTERP:
			printf("interp: %s\n", data + phdr->p_offset);
			break;
		}
	}
	void *heap = aligned_alloc(PAGE_SIZE_4K, 1 GiB);
	vmm_mmap(vm, heap, vm->heap_gvirt, 1 GiB, VMM_MMAP_PROT_RW);
	close(fd);
	elf->ehdr = ehdr;
}


static void
push_strings(vm_t *vm, int count, char **vector, vmm_gvirt_t *gvbuf)
{
	int size = 0;
	int offset[count + 1];

	offset[0] = 0;
	for (int i = 0; i < count; i++) {
		size += strlen(vector[i]) + 1;
		offset[i + 1] = size;
	}

	char buf[size];
	for (int i = 0; i < count; i++)
		strcpy(&buf[offset[i]], vector[i]);

	vmm_push(vm, buf, size);
	vmm_gvirt_t stack_gvirt = vm->regs.rsp;
	for (int i = 0; i < count; i++)
		gvbuf[i] = stack_gvirt + offset[i];
	gvbuf[count] = 0;
}

void
setup_stack(elf_t *elf, vm_t *vm)
{
	int argc = 1;
	char arg[] = "/bin/ls";
	char *argv[] = { arg, NULL, };
	char env[] = "USER=shina";
	char *envp[] = { env, NULL, };
	uint64_t null = 0;

	char random[16];
	vmm_push(vm, random, sizeof(random));
	vmm_gvirt_t rand_gvirt = vm->regs.rsp;

	int envc = 0;
	for (char **v = envp; *v != NULL; v++)
		envc++;
	vmm_gvirt_t guest_envp[envc + 1];
	push_strings(vm, envc, envp, guest_envp);

	vmm_gvirt_t guest_argv[argc + 1];
	push_strings(vm, argc, argv, guest_argv);

	Elf64_Ehdr *ehdr = elf->ehdr;
	Elf64_auxv_t auxv[] = {
		{ AT_BASE, 0 },
		{ AT_ENTRY, ehdr->e_entry + elf->global_offset },
		{ AT_PHDR, elf->load_base + ehdr->e_phoff },
		{ AT_PHENT, ehdr->e_phentsize },
		{ AT_PHNUM, ehdr->e_phnum },
		{ AT_PAGESZ, PAGE_SIZE_4K },
		{ AT_RANDOM, rand_gvirt },
		{ AT_NULL, 0 },
	};
	vmm_push(vm, auxv, sizeof(auxv));
	vmm_push(vm, guest_envp, sizeof(guest_envp));
	vmm_push(vm, guest_argv, sizeof(guest_argv));
	vmm_push(vm, &argc, sizeof(argc));
}

void
load_elf(vm_t *vm, char *filename)
{
	elf_t elf = {
		.ehdr = NULL,
		.global_offset = 0,
		.load_base = 0,
	};

	map_file(&elf, vm, filename);
	setup_stack(&elf, vm);
}
