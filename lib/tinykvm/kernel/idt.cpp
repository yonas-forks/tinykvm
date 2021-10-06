#include "idt.hpp"

#include "../common.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <linux/kvm.h>
struct kvm_sregs;

namespace tinykvm {

// 64-bit IDT entry
struct IDTentry {
	uint16_t offset_1;  // offset bits 0..15
	uint16_t selector;  // a code segment selector in GDT or LDT
	uint8_t  ist;       // 3-bit interrupt stack table offset
	uint8_t  type_attr; // type and attributes, see below
	uint16_t offset_2;  // offset bits 16..31
	uint32_t offset_3;  // 32..63
	uint32_t zero2;
};
static_assert(sizeof(IDTentry) == 16, "AMD64 IDT entries are 16-bytes");

#define IDT_GATE_INTR 0x0e
#define IDT_CPL0      0x00
#define IDT_CPL3      0x60
#define IDT_PRESENT   0x80

struct IDT
{
	/* Just enough for CPU exceptions and 1 timer interrupt */
	std::array<IDTentry, 33> entry;
};

union addr_helper {
	uint64_t whole;
	struct {
		uint16_t lo16;
		uint16_t hi16;
		uint32_t top32;
	};
};

static void set_entry(
	IDTentry& idt_entry,
	uint64_t handler,
	uint16_t segment_sel,
	uint8_t  attributes)
{
	addr_helper addr { .whole = handler };
	idt_entry.offset_1  = addr.lo16;
	idt_entry.offset_2  = addr.hi16;
	idt_entry.offset_3  = addr.top32;
	idt_entry.selector  = segment_sel;
	idt_entry.type_attr = attributes;
	idt_entry.ist       = 1;
	idt_entry.zero2     = 0;
}

void set_exception_handler(void* area, uint8_t vec, uint64_t handler)
{
	auto& idt = *(IDT *)area;
	set_entry(idt.entry[vec], handler, 0x8, IDT_PRESENT | IDT_CPL0 | IDT_GATE_INTR);
	/* Use second IST for double faults */
	//idt.entry[vec].ist = (vec != 8) ? 1 : 2;
}

unsigned char interrupts[] = {
  0x10, 0x20, 0x4c, 0x20, 0x80, 0x20, 0x08, 0x00, 0x55, 0x20, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x3d, 0x9e, 0x00, 0x00, 0x00, 0x74, 0x0c, 0x3d,
  0x77, 0xf7, 0x01, 0x00, 0x74, 0x42, 0xe7, 0x00, 0x48, 0x0f, 0x07, 0x56,
  0x51, 0x52, 0x48, 0x81, 0xff, 0x02, 0x10, 0x00, 0x00, 0x75, 0x18, 0xb9,
  0x00, 0x01, 0x00, 0xc0, 0x89, 0xf0, 0x48, 0xc1, 0xee, 0x20, 0x89, 0xf2,
  0x0f, 0x30, 0x48, 0x31, 0xc0, 0x5a, 0x59, 0x5e, 0x48, 0x0f, 0x07, 0x66,
  0xe7, 0x00, 0xeb, 0xf5, 0xb8, 0x60, 0x00, 0x00, 0x00, 0x66, 0xe7, 0x00,
  0xc3, 0x48, 0xb8, 0x4c, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3,
  0x0f, 0x20, 0xd8, 0x0f, 0x22, 0xd8, 0x48, 0x0f, 0x07, 0x57, 0x0f, 0x20,
  0xd7, 0x66, 0xe7, 0x8e, 0x0f, 0x01, 0x3f, 0x5f, 0x48, 0x83, 0xc4, 0x08,
  0x48, 0xcf, 0x66, 0xe7, 0xa1, 0x48, 0xcf, 0x90, 0x66, 0xe7, 0x80, 0x48,
  0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x81, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x82, 0x48, 0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x83, 0x48,
  0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x84, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x85, 0x48, 0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x86, 0x48,
  0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x87, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x88, 0xeb, 0xaf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x89, 0x48,
  0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x8a, 0xeb, 0x9f, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x8b, 0xeb, 0x97, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x8c, 0xeb,
  0x8f, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x8d, 0xeb, 0x87, 0x90, 0x90, 0x90,
  0xe9, 0x74, 0xff, 0xff, 0xff, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x8f, 0x48,
  0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x90, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x91, 0xe9, 0x64, 0xff, 0xff, 0xff, 0x66, 0xe7, 0x92, 0x48,
  0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x93, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x94, 0x48, 0xcf, 0x90, 0x90, 0x90, 0xe9, 0x4d, 0xff, 0xff,
  0xff
};
unsigned int interrupts_len = 301;

const iasm_header& interrupt_header() {
	return *(const iasm_header*) &interrupts[0];
}

void setup_amd64_exception_regs(struct kvm_sregs& sregs, uint64_t addr)
{
	sregs.idt.base  = addr;
	sregs.idt.limit = sizeof(IDT) - 1;
}

void setup_amd64_exceptions(uint64_t addr, void* area, void* except_area)
{
	uint64_t offset = interrupt_header().vm64_exception;
	for (int i = 0; i <= 20; i++) {
		if (i == 15) continue;
		//printf("Exception handler %d at 0x%lX\n", i, offset);
		set_exception_handler(area, i, offset);
		offset += interrupt_header().vm64_except_size;
	}
	// Program the timer interrupt (which sends NMI)
	offset += interrupt_header().vm64_except_size;
	set_exception_handler(area, 32, offset);
	// Install exception handling code
	std::memcpy(except_area, interrupts, sizeof(interrupts));
}

TINYKVM_COLD()
void print_exception_handlers(const void* area)
{
	auto* idt = (IDT*) area;
	for (unsigned i = 0; i < idt->entry.size(); i++) {
		const auto& entry = idt->entry[i];
		const addr_helper addr {
			.lo16 = entry.offset_1,
			.hi16 = entry.offset_2,
			.top32 = entry.offset_3
		};
		printf("IDT %u: func=0x%lX sel=0x%X p=%d dpl=%d type=0x%X ist=%u\n",
			i, addr.whole, entry.selector, entry.type_attr >> 7,
			(entry.type_attr >> 5) & 0x3, entry.type_attr & 0xF, entry.ist);
	}
}

struct AMD64_Ex {
	const char* name;
	bool        has_code;
};
static constexpr std::array<AMD64_Ex, 34> exceptions =
{
	AMD64_Ex{"Divide-by-zero Error", false},
	AMD64_Ex{"Debug", false},
	AMD64_Ex{"Non-Maskable Interrupt", false},
	AMD64_Ex{"Breakpoint", false},
	AMD64_Ex{"Overflow", false},
	AMD64_Ex{"Bound Range Exceeded", false},
	AMD64_Ex{"Invalid Opcode", false},
	AMD64_Ex{"Device Not Available", false},
	AMD64_Ex{"Double Fault", true},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Invalid TSS", true},
	AMD64_Ex{"Segment Not Present", true},
	AMD64_Ex{"Stack-Segment Fault", true},
	AMD64_Ex{"General Protection Fault", true},
	AMD64_Ex{"Page Fault", true},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"x87 Floating-point Exception", false},
	AMD64_Ex{"Alignment Check", true},
	AMD64_Ex{"Machine Check", false},
	AMD64_Ex{"SIMD Floating-point Exception", false},
	AMD64_Ex{"Virtualization Exception", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Security Exception", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Execution Timeout", false},
};

const char* amd64_exception_name(uint8_t intr) {
	return exceptions.at(intr).name;
}
bool amd64_exception_code(uint8_t intr) {
	return exceptions.at(intr).has_code;
}

}
