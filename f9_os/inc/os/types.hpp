/*
 * types.hpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#ifndef OS_TYPES_HPP_
#define OS_TYPES_HPP_


#include <cstdint>
#include <cstddef>
#include <functional>
#include <type_traits>

// types
namespace os {
	typedef uintptr_t memptr_t;
	using ptr_t = uint32_t;
	//using memptr_t = uintptr_t;
	using l4_thread_t = uint32_t;


	template<typename ALIGNT, typename SIZET>
	static constexpr inline ALIGNT ALIGNED(SIZET size,ALIGNT align) {
		return (size / align) + ((size & (align - 1)) != 0);
	}

	namespace l4 {

		// ipc
		static constexpr size_t  IPC_TI_MAP_GRANT =0x8;
		static constexpr size_t  IPC_TI_GRANT =02;
		static constexpr size_t  IPC_MR_COUNT =16;
		static constexpr size_t L4_NILTHREAD	=	0;
		static constexpr size_t L4_ANYTHREAD	=	0xFFFFFFFF;

		union ipc_msg_tag_t {
			struct {
				/* Number of words */
				uint32_t n_untyped : 6;
				uint32_t n_typed : 6;

				uint32_t flags : 4;	/* Type of operation */
				uint16_t label;
			} s;
			uint32_t raw;
		};

		union ipc_typed_item_t {
			struct {
				uint32_t	header : 4;
				uint32_t	dummy  : 28;
			} s;
			struct {
				uint32_t	header : 4;
				uint32_t	base  : 28;
			} map;
			uint32_t raw;
		} ;

		union ipc_time_t {
			uint16_t raw;
			struct {
				uint32_t	m : 10;
				uint32_t	e : 5;
				uint32_t	a : 1;
			} period;
			struct {
				uint32_t	m : 10;
				uint32_t	c : 1;
				uint32_t	e : 4;
				uint32_t	a : 1;
			} point;
		};

		/*
		 * KIP page. based on L4 X.2 Reference manual Rev. 7
		 */

		/*
		 * NOTE: kip_mem_desc_t differs from L4 X.2 standard
		 */
		 struct kip_mem_desc{
			uint32_t 	base;	/* Last 6 bits contains poolid */
			uint32_t	size;	/* Last 6 bits contains tag */
		};

		 union kip_apiversion{
			struct {
				uint8_t  version;
				uint8_t  subversion;
				uint8_t  reserved;
			} s;
			uint32_t raw;
		} ;

		 union kip_apiflags {
			struct {
				uint32_t  reserved : 28;
				uint32_t  ww : 2;
				uint32_t  ee : 2;
			} s;
			uint32_t raw;
		} ;

		 union  kip_memory_info{
			struct {
				uint16_t memory_desc_ptr;
				uint16_t n;
			} s;
			uint32_t raw;
		};

		 union kip_threadinfo {
			struct {
				uint32_t user_base;
				uint32_t system_base;
			} s;
			uint32_t raw;
		} ;

		struct kip_t {
			/* First 256 bytes of KIP are compliant with L4 reference
			 * manual version X.2 and built in into flash (lower kip)
			 */
			uint32_t kernel_id;
			kip_apiversion api_version;
			kip_apiflags api_flags;
			uint32_t kern_desc_ptr;

			uint32_t reserved1[17];

			kip_memory_info memory_info;

			uint32_t reserved2[20];

			uint32_t utcb_info;		/* Unimplemented */
			uint32_t kip_area_info;		/* Unimplemented */

			uint32_t reserved3[2];

			uint32_t boot_info;		/* Unimplemented */
			uint32_t proc_desc_ptr;		/* Unimplemented */
			uint32_t clock_info;		/* Unimplemented */
			kip_threadinfo thread_info;
			uint32_t processor_info;	/* Unimplemented */

			/* Syscalls are ignored because we use SVC/PendSV instead of
			 * mapping SC into thread's address space
			 */
			uint32_t syscalls[12];
		};
		static constexpr size_t UTCB_SIZE	=	128;

		struct utcb_t {
		/* +0w */
			l4_thread_t t_globalid;
			uint32_t	processor_no;
			uint32_t 	user_defined_handle;	/* NOT used by kernel */
			l4_thread_t	t_pager;
		/* +4w */
			uint32_t	exception_handler;
			uint32_t	flags;		/* COP/PREEMPT flags (not used) */
			uint32_t	xfer_timeouts;
			uint32_t	error_code;
		/* +8w */
			l4_thread_t	intended_receiver;
			l4_thread_t	sender;
			uint32_t	thread_word_1;
			uint32_t	thread_word_2;
		/* +12w */
			uint32_t	mr[8];		/* MRs 8-15 (0-8 are laying in
							   r4..r11 [thread's context]) */
		/* +20w */
			uint32_t	br[8];
		/* +28w */
			uint32_t	reserved[4];
		/* +32w */
		}__attribute__ ((aligned(UTCB_SIZE)));

	};
};

#endif /* OS_TYPES_HPP_ */