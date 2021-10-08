#include "tss.hpp"

#include <linux/kvm.h>
#include <cstring>
#include "memory_layout.hpp"
#include "gdt.hpp"

namespace tinykvm {

struct AMD64_TSS
{
	uint32_t ign;  // 4
	uint64_t rsp0; // 12
	uint64_t rsp1; // 20
	uint64_t rsp2; // 28
	uint32_t ign2; // 32
	uint32_t ign3; // 36
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7; // 92 0x5C
	uint32_t ign4;
	uint32_t ign5;
	uint16_t ign6;
	uint16_t iomap_base;
} __attribute__((packed));

static constexpr uint16_t tss_sel = 0x30;


void setup_amd64_tss(
	uint64_t tss_addr, char* tss_ptr, char* gdt_ptr)
{
	auto& tss = *(AMD64_TSS *)tss_ptr;
	std::memset(&tss, 0, sizeof(tss));
	tss.ist1 = IST_ADDR + 0x2000;
	tss.ist2 = IST_ADDR + 0x1800;
	tss.rsp0 = IST_ADDR + 0x2000;
	tss.rsp1 = IST_ADDR + 0x2000;
	tss.rsp2 = IST_ADDR + 0x2000;
	tss.iomap_base = 104; // unused

	GDT_write_TSS_segment(gdt_ptr + tss_sel, tss_addr, sizeof(AMD64_TSS)-1);
}

void setup_amd64_tss_smp(char* smp_tss_ptr)
{
	auto* tss = (AMD64_TSS *)smp_tss_ptr;
	for (size_t c = 0; c < 78; c++) {
		/** XXX: TSS_SMP_STACK exception stack enough? */
		tss[c].rsp0 = IST_ADDR + TSS_SMP_STACK * (c + 1);
		tss[c].rsp1 = tss[c].rsp0;
		tss[c].rsp2 = tss[c].rsp0;
		tss[c].ist1 = tss[c].rsp0;
		tss[c].iomap_base = 104; // unused
	}
}

void setup_amd64_tss_regs(struct kvm_sregs& sregs, uint64_t tss_addr)
{
	struct kvm_segment seg = {
		.base = tss_addr,
		.limit = sizeof(AMD64_TSS)-1,
		.selector = tss_sel,
		.type = 11,
		.present = 1,
		.dpl = 3, /* User-mode */
		.db = 0,
		.s = 0, /* Gate */
		.l = 0, /* 64-bit */
		.g = 0, /* Byte granularity */
	};
	sregs.tr = seg;
}

}
