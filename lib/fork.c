// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pte_t *pt = (pte_t *)uvpd[PDX(addr)];
	if (!pt)
		panic("pgfault: page table not availible");
	pte_t *pt_entry = (pte_t *)pt[PTX(addr)];
	if (!(err == FEC_WR))
		panic("pgfault: faulting access was not a write");
	if (!pt_entry || !(*pt_entry & PTE_COW))
		panic("pgfault: faulting access was not to a copy-on-write page");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if ((r = sys_page_alloc(0, PFTEMP, 
				PTE_W|PTE_U|PTE_P)) < 0)
		panic("pgfault: %e at sys_page_alloc", r);
	memmove(PFTEMP, addr, PGSIZE);
	if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_W|PTE_U|PTE_P)) < 0)
		panic("pgfault: %e at sys_page_map", r);
	if ((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("pgfault: %e at sys_page_unmap", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.	
	void *addr = (void *)(pn * PGSIZE);
	pte_t *pt = (pte_t *)uvpd[PDX(addr)];
	if (!pt)
		panic("pgfault: page table not availible");
	pte_t *pt_entry = (pte_t *)pt[PTX(addr)];
	if (!pt_entry)
		panic("pgfault: faulting access was not to a copy-on-write page");
	if (*pt_entry & (PTE_P|PTE_U)) {
		// Read-only
		if (!(*pt_entry & PTE_W) && !(*pt_entry & PTE_COW))
			sys_page_map(0, addr, envid, addr, PTE_U|PTE_P);

		if (*pt_entry & PTE_W && *pt_entry & PTE_COW)
				panic("duppage: %x both copy-on-write and writable");

		// TODO
		sys_page_unmap(0, addr);
		sys_page_alloc(envid, addr, PTE_COW|PTE_U|PTE_P);
		sys_page_map(envid, addr, 0, addr, PTE_COW|PTE_U|PTE_P);
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	int r;
	envid_t envid;
	// Setup page fault handler
	set_pgfault_handler(pgfault);
	// Create a child
	envid = sys_exofork();
	if (envid < 0)
		panic("fork: %e at sys_exofork", envid);
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		set_pgfault_handler(pgfault);
		return 0;
	}
	// Copy address space 
//	int i, j;
//	duppage(envid, (unsigned)uvpt / PGSIZE);
//	for (i = 0; i != NPDENTRIES; ++i) {
//		duppage(envid, uvpd[i] / PGSIZE);
//		for (j = 0; j != NPTENTRIES; ++j) {
//			pte_t *pt = (pte_t *)uvpd[i];
//			duppage(envid, pt[j] / PGSIZE);
//		}
//	}
	uintptr_t addr;
	for (addr = 0; addr < USTACKTOP; addr += PGSIZE)
		duppage(envid, addr / PGSIZE);
	// Set child status
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("fork: %e at sys_env_set_status", envid);
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
