#ifndef _CONF_H_
#define _CONF_H_

#include <cstdint>
namespace os {
	static constexpr size_t CONFIG_LARGEST_FPAGE_SHIFT=16;
	static constexpr size_t CONFIG_SMALLEST_FPAGE_SHIFT=8;
	static constexpr size_t CONFIG_KIP_EXTRA_SIZE=128;
// stm32 fixed memory map space
	static constexpr uintptr_t CODE_BEGIN = 0x00000000;
	static constexpr uintptr_t SRAM_BEGIN = 0x20000000;
	static constexpr uintptr_t DEVICE_BEGIN = 0x40000000;
	static constexpr uintptr_t EXRAM_BEGIN = 0x60000000;
	static constexpr uintptr_t EXDEVICE_BEGIN = 0xA0000000;
	static constexpr uintptr_t PRIVDEVICEBUS_BEGIN = 0xE0000000;
	static constexpr uintptr_t VENDOR_BEGIN = 0xE0100000;

	static constexpr uintptr_t CODE_END = SRAM_BEGIN-1;
	static constexpr uintptr_t SRAM_END = DEVICE_BEGIN-1;
	static constexpr uintptr_t DEVICE_END = EXRAM_BEGIN-1;
	static constexpr uintptr_t EXRAM_END = EXDEVICE_BEGIN-1;
	static constexpr uintptr_t EXDEVICE_END = PRIVDEVICEBUS_BEGIN-1;
	static constexpr uintptr_t PRIVDEVICEBUS_END = VENDOR_BEGIN-1;
	static constexpr uintptr_t VENDOR_END = 0xFFFFFFFF;
	// true end of memory
	static constexpr uintptr_t TRUE_SRAM_END  = 0x20050000;

};

#endif
