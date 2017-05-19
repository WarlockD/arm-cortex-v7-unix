/*
 * ipc.hpp
 *
 *  Created on: May 18, 2017
 *      Author: Paul
 */

#ifndef F9_IPC_HPP_
#define F9_IPC_HPP_

#include "types.hpp"

namespace f9 {


	#define IPC_TI_MAP_GRANT 0x8
	#define IPC_TI_GRANT 	 0x2

	#define IPC_MR_COUNT	16
// http://stackoverflow.com/questions/1392059/algorithm-to-generate-bit-mask


	class ipc_msg_tag_t {
		using bit_label   	= bitops::bit_helper<uint32_t, 0 ,16>;
		using bit_flags		= bitops::bit_helper<uint32_t, 16,4>;
		using bit_typed  	= bitops::bit_helper<uint32_t, 20,6>;
		using bit_untyped	= bitops::bit_helper<uint32_t, 26,6>;
#if 0
		union _ipc_msg_tag_t{
			struct {
				/* Number of words */
				uint32_t n_untyped : 6;
				uint32_t n_typed : 6;

				uint32_t flags : 4;	/* Type of operation */
				uint16_t label;
			} ;
			uint32_t raw;
		} ;
#endif
		uint32_t _raw;
	public:
		ipc_msg_tag_t() : _raw{0} {}
		explicit ipc_msg_tag_t(uint32_t raw) : _raw(raw) {}
		ipc_msg_tag_t(uint32_t untyped, uint32_t typed, uint32_t  flags, uint16_t label)
			: _raw(bit_untyped::mask(untyped) | bit_typed::mask(typed) | bit_flags::mask(flags) | bit_label::mask(label)) {}
		uint32_t untyped() const { return bit_untyped::get(_raw); }
		uint32_t typed() const { return bit_typed::get(_raw); }
		uint32_t flags() const { return bit_flags::get(_raw); }
		uint32_t label() const { return bit_label::get(_raw); }
		void untyped(uint32_t val)  {  bit_untyped::set(_raw,val); }
		void typed(uint32_t val)  {  bit_typed::set(_raw,val); }
		void flags(uint32_t val)  {  bit_flags::set(_raw,val); }
		void label(uint32_t val)  {  bit_label::set(_raw,val); }
		uint32_t raw() const { return _raw; }
		void raw(uint32_t raw) { _raw = raw; }
	};

	class ipc_typed_item_t {
#if 0
		typedef union {
			struct {
				uint32_t	header : 4;
				uint32_t	dummy  : 28;
			} s;
			struct {
				uint32_t	header : 4;
				uint32_t	base  : 28;
			} map;
			uint32_t raw;
		} ipc_typed_item;
#endif
		using bit_header   	= bitops::bit_helper<uint32_t, 0 ,4>;
		using bit_map_base	= bitops::bit_helper<uint32_t, 4,28>;
		uint32_t _raw;
	public:
		ipc_typed_item_t() : _raw{0} {}
		explicit ipc_typed_item_t(uint32_t raw) : _raw(raw) {}
		ipc_typed_item_t(uint32_t header, uint32_t base)
				: _raw(bit_header::mask(header) | bit_map_base::mask(base)) {}
		uint32_t header() const { return bit_header::get(_raw); }
		void header(uint32_t v)  { return bit_header::set(_raw,v); }
		uint32_t base() const { return bit_map_base::get(_raw); }
		void base(uint32_t v)  { return bit_map_base::set(_raw,v); }
		uint32_t raw() const { return _raw; }
		void raw(uint32_t raw) { _raw = raw; }

	};


	void sys_ipc(uint32_t *param1);
	uint32_t ipc_deliver(void *data);

//	uint32_t ipc_read_mr(tcb_t *from, int i);
//	void ipc_write_mr(tcb_t *to, int i, uint32_t data);


}; /* namespace f9 */

#endif /* F9_IPC_HPP_ */
