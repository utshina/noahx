#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <elf.h>
#include "load.h"
#include "panic.h"

typedef struct {
	uint64_t entry;
	uint64_t phdr;
	uint64_t phent;
	uint64_t phnum;
} elf_t;

static void
push(mm_t *mm, uint64_t *rsp, const void *data, size_t n)
{
	uint64_t remainder = roundup(n, 8) - n;
	*rsp -= (n + remainder);
	void *sp = mm_stack_gvirt_to_hvirt(mm, *rsp);
	memcpy(sp, data, n);
	memset(sp + n, 0, remainder);
}

static void
push_strings(mm_t *mm, uint64_t *rsp, int count, char **vector, mm_gvirt_t *gvbuf)
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

	push(mm, rsp, buf, size);
	for (int i = 0; i < count; i++)
		gvbuf[i] = *rsp + offset[i];
	gvbuf[count] = 0;
}

static void
setup_stack(elf_t *elf, mm_t *mm, load_info_t *info)
{
	mm_gvirt_t *rsp = &info->stack;
	int argc = 1;
	char arg[] = "/bin/ls";
	char *argv[] = { arg, NULL, };
	char env[] = "USER=shina";
	char *envp[] = { env, NULL, };

	char random[16];
	push(mm, rsp, random, sizeof(random));
	mm_gvirt_t rand_gvirt = *rsp;

	int envc = 0;
	for (char **v = envp; *v != NULL; v++)
		envc++;
	mm_gvirt_t guest_envp[envc + 1];
	push_strings(mm, rsp, envc, envp, guest_envp);

	mm_gvirt_t guest_argv[argc + 1];
	push_strings(mm, rsp, argc, argv, guest_argv);

	Elf64_auxv_t auxv[] = {
		{ AT_BASE,  {0} },
		{ AT_ENTRY, { elf->entry } },
		{ AT_PHDR, { elf->phdr } },
		{ AT_PHENT, { elf->phent } },
		{ AT_PHNUM, { elf->phnum } },
		{ AT_PAGESZ, {PAGE_SIZE_4K} },
		{ AT_RANDOM, {rand_gvirt} },
		{ AT_NULL, {0} },
	};
	push(mm, rsp, auxv, sizeof(auxv));
	push(mm, rsp, guest_envp, sizeof(guest_envp));
	push(mm, rsp, guest_argv, sizeof(guest_argv));
	push(mm, rsp, &argc, sizeof(argc));
}

static mm_mmap_prot_t
conv_prot(Elf64_Word flags)
{
	mm_mmap_prot_t prot = 0;
	if (flags & PF_X) prot |= MM_MMAP_PROT_EXEC;
	if (flags & PF_W) prot |= MM_MMAP_PROT_WRITE;
	if (flags & PF_R) prot |= MM_MMAP_PROT_READ;
	return prot;
}

static void
map_elf_file(elf_t *elf, mm_t *mm, char *filename, load_info_t *info)
{
	int fd = open(filename, O_RDONLY, 0);
	if (fd < 0)
		panic("can't open file");

	struct stat statbuf;
	fstat(fd, &statbuf) == 0
		OR panic("can't stat");

	void *data = mmap(NULL, statbuf.st_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED)
		perror("mmap"), panic("mmap failed");

	Elf64_Ehdr *ehdr = data;
	uint8_t valid_ident[] = {0x7f, 'E', 'L', 'F', 0x02, 0x01, 0x01};
	memcmp(ehdr->e_ident, valid_ident, sizeof(valid_ident)) == 0
		OR panic("ELF header is invalid");
	if (!(ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN))
		panic("ELF is not a supported type");
	if (ehdr->e_machine != EM_X86_64)
		panic("ELF is not an x64 executable");

	info->entry = elf->entry = ehdr->e_entry;

	int n = ehdr->e_phnum;
	Elf64_Phdr *phdr = data + ehdr->e_phoff;

	mm_gvirt_t heap = 0;
	uint64_t load_base = 0;
	for (int i = 0; i < n; i++) {
		switch (phdr[i].p_type) {
		case PT_LOAD: {
			off_t offset = rounddown(phdr[i].p_offset, PAGE_SIZE_4K);
			mm_gvirt_t gvirt = rounddown(phdr[i].p_vaddr, PAGE_SIZE_4K);
			void *hvirt = data + offset;
			uint64_t size = roundup(gvirt + phdr[i].p_memsz, PAGE_SIZE_4K) - gvirt;
			mm_mmap_prot_t prot = conv_prot(phdr[i].p_flags);
			mm_mmap(mm, gvirt, hvirt, size, prot, MM_MMAP_TYPE_MMAP);

			void *file_end = data + phdr[i].p_offset + phdr[i].p_filesz;
			size_t remainder = hvirt + size - file_end;
			memset(file_end, 0, remainder);

			if (load_base == 0)
				load_base = phdr[i].p_vaddr - phdr[i].p_offset;
			heap = roundup(max(heap, gvirt + size), PAGE_SIZE_4K);
			break;
		}

		case PT_INTERP:
			printf("interp\n");
			break;
		}
	}

	info->heap = heap;
	elf->phdr = load_base + ehdr->e_phoff;
	elf->phent = ehdr->e_phentsize;
	elf->phnum = ehdr->e_phnum;
	close(fd);

}

void
ldr_load(mm_t *mm, int argc, char *argv[], load_info_t *info)
{
	elf_t elf;
	info->stack = mm_get_stack_top(mm);
	map_elf_file(&elf, mm, argv[0], info);
	setup_stack(&elf, mm, info);
}
