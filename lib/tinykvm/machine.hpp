#pragma once
#include "common.hpp"
#include "forward.hpp"
#include "memory.hpp"
#include <array>
#include <vector>

namespace tinykvm {

struct Machine
{
	using address_t = uint64_t;
	using syscall_t = void(*)(Machine&);
	using unhandled_syscall_t = void(*)(Machine&, unsigned);

	template <typename... Args>
	long vmcall(const char*, Args&&...);
	template <typename... Args> constexpr
	long vmcall(address_t, Args&&...);

	void setup_argv(const std::vector<std::string>& args,
					const std::vector<std::string>& env = {});
	void setup_linux(const std::vector<std::string>& args,
					const std::vector<std::string>& env = {});
	long run(unsigned timeout = 10);
	long step_one();
	void stop(bool = true);
	bool stopped() const noexcept { return m_stopped; }
	void reset();

	void copy_to_guest(address_t addr, const void*, size_t);

	template <typename T>
	uint64_t stack_push(__u64& sp, const T&);
	uint64_t stack_push(__u64& sp, const void*, size_t);
	uint64_t stack_push(__u64& sp, const std::string&);

	tinykvm_x86regs registers() const;
	void set_registers(const tinykvm_x86regs&);
	std::pair<__u64, __u64> get_fsgs() const;
	void set_tls_base(__u64 baseaddr);
	void print_registers();

	void install_syscall_handler(unsigned idx, syscall_t h) { m_syscalls.at(idx) = h; }
	void install_unhandled_syscall_handler(unhandled_syscall_t h) { m_unhandled_syscall = h; }
	void system_call(unsigned);

	std::string_view io_data() const;
	std::string_view memory_at(uint64_t a, size_t s) const { return memory.view(a, s); }
	char* rw_memory_at(uint64_t a, size_t s) { return memory.safely_at(a, s); }
	bool memory_safe_at(uint64_t a, size_t s) const { return memory.safely_within(a, s); }

	address_t start_address() const noexcept { return this->m_start_address; }
	address_t stack_address() const noexcept { return this->m_stack_address; }
	address_t exit_address() const noexcept { return this->m_exit_address; }
	void set_exit_address(address_t addr) { this->m_exit_address = addr; }

	uint64_t address_of(const char*) const;

	Machine(const std::vector<uint8_t>& binary, const MachineOptions&);
	Machine(std::string_view binary, const MachineOptions&);
	~Machine();

private:
	static constexpr uint64_t GDT_ADDR = 0x1600;
	static constexpr uint64_t TSS_ADDR = 0x1700;
	static constexpr uint64_t IDT_ADDR = 0x1800;
	static constexpr uint64_t EXCEPT_ASM_ADDR = 0x2000;
	static constexpr uint64_t IST_ADDR = 0x3000;
	static constexpr uint64_t PT_ADDR  = 0x4000;
	struct vCPU {
		void init(Machine&);
		void print_address_info(uint64_t addr);
		tinykvm_x86regs registers() const;
		void assign_registers(const struct tinykvm_x86regs&);

		int fd;
		struct kvm_run *kvm_run;
	};
	template <typename... Args> constexpr
	tinykvm_x86regs setup_call(uint64_t addr, Args&&... args);
	void setup_registers(tinykvm_x86regs&);
	void setup_argv(__u64&, const std::vector<std::string>&, const std::vector<std::string>&);
	void setup_linux(__u64&, const std::vector<std::string>&, const std::vector<std::string>&);
	int install_memory(uint32_t idx, vMemory mem);
	void elf_loader(const MachineOptions&);
	void elf_load_ph(const MachineOptions&, const void*);
	void setup_long_mode();
	void handle_exception(uint8_t intr);
	long run_once();

	int   fd;
	bool  m_stopped = false;
	vCPU  vcpu;
	void* m_userdata = nullptr;

	std::array<syscall_t, TINYKVM_MAX_SYSCALLS> m_syscalls {nullptr};
	unhandled_syscall_t m_unhandled_syscall = [] (auto&, unsigned) {};

	std::string_view m_binary;
	uint64_t m_exit_address;
	uint64_t m_stack_address;
	uint64_t m_start_address;

	vMemory memory; // guest memory
	MemRange mmio_scall; // syscall MMIO slot
	MemRange ptmem; // page tables

	static int kvm_fd;
};

#include "machine_inline.hpp"
}
