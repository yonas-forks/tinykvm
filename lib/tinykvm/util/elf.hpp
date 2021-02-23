#pragma once
#include "elf.h"

namespace tinykvm {

	template <typename T>
	inline const T* elf_offset(std::string_view binary, intptr_t ofs) {
		return (const T*) &binary.at(ofs);
	}
	inline const auto* elf_header(std::string_view binary) {
		return elf_offset<Elf64_Ehdr> (binary, 0);
	}
	inline bool validate_header(const Elf64_Ehdr* hdr)
	{
		if (hdr->e_ident[EI_MAG0] != 0x7F ||
			hdr->e_ident[EI_MAG1] != 'E'  ||
			hdr->e_ident[EI_MAG2] != 'L'  ||
			hdr->e_ident[EI_MAG3] != 'F')
			return false;
		return hdr->e_ident[EI_CLASS] == ELFCLASS64;
	}

}
