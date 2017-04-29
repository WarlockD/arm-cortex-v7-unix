#include "os.h"


extern volatile void* g_pfnVectors[]; // all the vectors

typedef struct nvec_callback {
	void (*callback)(void*,uint32_t*);
	void* data;
} nvec_callbacks[256];

extern const void* Default_Handler;


void nvic_init() {

}
