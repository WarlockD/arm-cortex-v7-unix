#include "param.h"
#include "conf.h"
#include "buf.h"
#include "user.h"
#include "inode.h"
#include "file.h"
#include "tty.h"
#define DEVMAX 10
#include "Utilities\Fatfs\src\ff.h"
#include <stm32746g_discovery_qspi.h>
#include "Utilities\Fatfs\src\ff_gen_drv.h"
#include "Utilities\Fatfs\src\drivers\sd_diskio.h"
#include <assert.h>
void trace_printf(const char* fmt, ...);

struct user u;
struct inode* rootdir=NULL;

struct bdevsw bdevsw[DEVMAX];
int	nblkdev=0;
struct cdevsw cdevsw[DEVMAX];
int	nchrdev=0;
int swapdev = -1;
int rootdev = -1;
struct fat_root {
	FIL fp;
	const char* path;
	size_t size;
};
struct fat_root v6_root;
FATFS fs;
struct enum_msg {
	int value;
	const char* value_str;
	const char* message;
};
#define EMSG(VALUE,MSG) { .value = VALUE, .value_str = #VALUE, .message= MSG },

static const struct enum_msg fresult_messages[] = {
	EMSG(FR_OK, "(0) Succeeded")
	EMSG(FR_DISK_ERR, "(1) A hard error occurred in the low level disk I/O layer")
	EMSG(FR_INT_ERR, "(2) Assertion failed")
	EMSG(FR_NOT_READY, "(3) The physical drive cannot work")
	EMSG(FR_NO_FILE, "(4) Could not find the file")
	EMSG(FR_NO_PATH, "(5) Could not find the path")
	EMSG(FR_INVALID_NAME, "(6) The path name format is invalid")
	EMSG(FR_DENIED, "(7) Access denied due to prohibited access or directory full")
	EMSG(FR_EXIST, "(8) Access denied due to prohibited access")
	EMSG(FR_INVALID_OBJECT, "(9) The file/directory object is invalid")
	EMSG(FR_WRITE_PROTECTED, "(10) The physical drive is write protected")
	EMSG(FR_INVALID_DRIVE, "(11) The logical drive number is invalid")
	EMSG(FR_NOT_ENABLED, "(12) The volume has no work area")
	EMSG(FR_NO_FILESYSTEM, "(13) There is no valid FAT volume")
	EMSG(FR_MKFS_ABORTED, "(14) The f_mkfs() aborted due to any parameter error")
	EMSG(FR_TIMEOUT, "(15) Could not get a grant to access the volume within defined period")
	EMSG(FR_LOCKED, "(16) The operation is rejected according to the file sharing policy")
	EMSG(FR_NOT_ENOUGH_CORE, "(17) LFN working buffer could not be allocated")
	EMSG(FR_TOO_MANY_OPEN_FILES, "(18) Number of open files > _FS_SHARE")
	EMSG(FR_INVALID_PARAMETER, "(19) Given parameter is invalid")
};

static FRESULT display_fresult(FRESULT fr){
	if(fr != FR_OK){
		for_each_array(m, fresult_messages){ // slower but failsafe
			if(m->value == fr) {
				trace_printf("FRESULT(%d,%s): %s\r\n", m->value, m->value_str, m->message);
				return fr;
			}
		}
		trace_printf("FRESULT(?,?): ?\r\n");
	}
	return fr;
}
#define CHECK_FRESULT(FR) display_fresult(FR)
//({ if((FR) != FR_OK) display_fresult(FR); (FR);   })

static void print_debug(const char* message, struct buf* bp,off_t n1, off_t n2) {
	trace_printf("%s: ", message);
	if(bp != NULL){
		trace_printf("blkno=%d ", bp->b_blkno);
	}
	trace_printf("n1=%d n2=%d\r\n", n1,n2);
}
/*------------------------------------------------------------/
/ Open or create a file in append mode
/------------------------------------------------------------*/

time_t ktimeget() { return 0; }
void ktimeset(time_t sec) { (void)sec; }
// just want this to work so we hack it
// 0x90040000 offset to user
// 0x90000000 offset to root
struct buf nor_tab;
static int nor_init = 0;
static QSPI_Info nor_info;
#define BDEV_OPEN_NAME(PREFIX) PREFIX##_open
#define BDEV_CLOSE_NAME(PREFIX) PREFIX##_close
#define BDEV_STRAT_NAME(PREFIX) PREFIX##_strat
#define BDEV_TAB_NAME(PREFIX) PREFIX##_tab
#define ASSIGN_BDEV(NAME) { \
	.d_open =  BDEV_OPEN_NAME(NAME), \
	.d_close = BDEV_CLOSE_NAME(NAME), \
	.d_strategy = BDEV_STRAT_NAME(NAME), \
	.d_tab = &BDEV_TAB_NAME(NAME), \
}

#define INSTALL_BDEV(DEV, NAME) do { \
	bdevsw[major(DEV)].d_open =  BDEV_OPEN_NAME(NAME); \
	bdevsw[major(DEV)].d_close =  BDEV_CLOSE_NAME(NAME); \
	bdevsw[major(DEV)].d_strategy =  BDEV_STRAT_NAME(NAME); \
	bdevsw[major(DEV)].d_tab =  &BDEV_TAB_NAME(NAME); \
} while(0)


int nor_open(dev_t dev,int mode) {
	if(!nor_init){
		BSP_QSPI_Init();
		nor_init = 1;
		BSP_QSPI_GetInfo(&nor_info);
		return 0;
	}
	return -1;

}
int nor_close(dev_t dev,int mode) {
	if(nor_init){
		BSP_QSPI_DeInit();
		nor_init = 0;
		return 0;
	}
	return -1;
}

int nor_strat(struct buf* bp ) {
	if(!nor_init) nor_open(bp->b_dev, 0);
	//size_t sz = (bp->b_bcount + BMASK) >> BSHIFT;
	off_t x = bp->b_blkno << BSHIFT;
	if (x > nor_info.FlashSize) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return -1;
	}
	bp->av_forw = NULL;
	if (bp->b_flags & B_READ)
		BSP_QSPI_Read((uint8_t*)bp->b_addr, x, BSIZE);
	else{
		trace_printf("NOR WRITE!\r\n");
		BSP_QSPI_Write((uint8_t*)bp->b_addr, x, BSIZE);
	}
	bp->b_resid = 0;
	iodone(bp);
	return 0;
}
int fat_fs_strat(struct buf* bp ) {
	FRESULT fr;
	off_t o = bp->b_blkno << BSHIFT;
	if (o > v6_root.size) {
		bp->b_flags |= B_ERROR;
		print_debug("offset to big",bp,o, v6_root.size);
		iodone(bp);
		return -1;
	}
	bp->av_forw = NULL;
	splbio();
	UINT osize;
	if(bp->b_flags & B_READ) {
		fr=f_read(&v6_root.fp, bp->b_addr, BSIZE,&osize);
	} else {
		fr=f_write(&v6_root.fp, bp->b_addr, BSIZE,&osize);
	}
	if(CHECK_FRESULT(fr) || osize != BSIZE) {
		bp->b_flags |= B_ERROR;
		print_debug("fr not ok",bp,fr, osize);
		return -1;
	}
	iodone(bp);
#if 0
		if (rftab.d_actf==0)
			rftab.d_actf = bp;
		else
			rftab.d_actl->av_forw = bp;
		rftab.d_actl = bp;
		if (rftab.d_active==0)
			rfstart();
#endif
	spl0();
	return 0;
}
extern UART_HandleTypeDef huart1;
void USART1_IRQHandler() //__uart_irq_handler(void)
{
#if 0
	  uint32_t isrflags   = READ_REG(khuart.Instance->ISR);
	  uint32_t cr1its     = READ_REG(khuart.Instance->CR1);
//	  uint32_t cr3its     = READ_REG(khuart.Instance->CR3);
	  uint32_t errorflags;
	  errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
	  if(errorflags == RESET && ((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET)){// no error
		    	volatile uint8_t chr = USART1->RDR;
		    	// get the char and bypass the handler
		    	queue_push(&(dbg_uart.rx), chr);
	  } else {
#endif
			HAL_UART_IRQHandler(&huart1);
}


int open_filler(dev_t dev, int mode) {(void)dev; (void)mode; return -1; }
struct buf fstab;

char sd_path[20];
int v6_setup() {
	FRESULT fr;
	//bdevsw[0].d_strategy=  fat_fs_strat;
	INSTALL_BDEV(0,nor);
#if 0
	bdevsw[0].d_strategy= nor_strat;
	bdevsw[0].d_open=  open_filler;
	bdevsw[0].d_close=  open_filler;
	bdevsw[0].d_tab = &fstab;
	nblkdev++;
#endif
	nchrdev=0;
	swapdev = 0;
	rootdev = 0;
#if 0

	// ok driver is set up
	fr=FATFS_LinkDriver(&SD_Driver, sd_path);
	assert(CHECK_FRESULT(fr) == FR_OK);
	/* Open or create a log file and ready to append */
	fr = f_mount(&fs, sd_path, 1);
	assert(CHECK_FRESULT(fr) == FR_OK);
	v6_root.path = "root.fs";
    fr = f_open(&v6_root.fp, v6_root.path, FA_WRITE | FA_READ);
    if (CHECK_FRESULT(fr)  == FR_OK) {
    	v6_root.size = f_size(&v6_root.fp);
    	trace_printf("root '%s' opened! size=%i\n", v6_root.path, v6_root.size);
    } else {
    	v6_root.size = 0;
    	trace_printf("root not opened! '%s' size=%i\n" ,v6_root.path, v6_root.size);
    }
#endif
    binit();
    cinit();
    iinit();
	// ok... this is intresting
    /* Open or create a log file and ready to append */
    // we are HERE! so now we have to manualy mount a root directory

	  // ok lets try to mount the root dir manualy
    rootdir = iget(0, (ino_t)ROOTINO);
    rootdir->i_count++;
    u.u_cdir = rootdir;
    rootdir->i_flag &= ~ILOCK; // be sure to unlock it
   // while(1);
    return 0;
}
