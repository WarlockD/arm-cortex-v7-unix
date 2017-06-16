#include <cstdint>
#include <cstddef>

#include <os\printk.h>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <stm32f7xx.h>
#include <assert.h>
#include <embxx\container\StaticQueue.h>
#include <usrlib.h>
#include <cmath>

//#include "buf.h"
//#include "user.h"
//#include "tty.h"
struct tty;

#define LOGTIME
#ifdef LOGTIME
#include <time.h>
#include <sys\time.h>
#include <sys\times.h>

#endif
#define QUEUE_LEN 512
#define QUEUE_OK		0x0
#define	QUEUE_OVERFLOW		0x1
#define	QUEUE_EMPTY		0x2



static int empty_write(const char* str,size_t size)  { return 0; }

#define HAS_OPTION(O) (printk_options & (O))
//static printk_options_t printk_options = static_cast<printk_options_t>(0);
static printk_write_t _printk_write = empty_write;


using default_queue_t = embxx::container::StaticQueue<uint8_t,64> ;
static embxx::container::StaticQueue<uint8_t,512>  g_buffer; // have to be big for the inital logging

extern UART_HandleTypeDef huart1;
void usart_sync_write(const uint8_t* data, size_t length) {
	while(HAL_BUSY!=HAL_UART_Transmit(&huart1, (uint8_t*)data, length,500));
}
void usart_async_write(const uint8_t* data, size_t length) {
	while(HAL_BUSY!=HAL_UART_Transmit_IT(&huart1, (uint8_t*)data, length));
}
void usart_sync_puts(const char* str){
	usart_sync_write((const uint8_t*)str,strlen(str));
}
void usart_async_puts(const char* str){
	usart_async_write((const uint8_t*)str,strlen(str));
}

static void g_queue_flush() {
	if(_printk_write == empty_write) return;
	if(!g_buffer.empty()){
		g_buffer.linearise();
		const char* p = reinterpret_cast<const char*>(g_buffer.clbegin());
		size_t written = _printk_write(p,g_buffer.size());
		assert(written == g_buffer.size());
		g_buffer.clear();
	}
}
template<typename B>
static void buffer_flush(B& buf) {
	//usart_sync_puts("FLUSH: ");
	//usart_sync_write((uint8_t*)buf.data(),buf.count());
	//usart_sync_puts("\r\n");
	if(_printk_write == empty_write) {
#if 0
		while(!buf.empty()) {
			while(g_buffer.full()) { __BKPT(); } // catch errors
			g_buffer.push_back(buf.front());
			g_buffer.pop_front();
		}
#endif
	} else {
		buf.linearise();
		const char* p = reinterpret_cast<const char*>(buf.clbegin());
		size_t written = _printk_write(p,buf.size());
		while(written != buf.size()) { __BKPT(); } ; // catch errors
	}
	buf.clear();
}
template<typename B,typename T>
static void push_buffer(B& buf,  T&& c){
	buf.push_back(c);
	if(buf.full() || c == '\n') buffer_flush(buf);
}

template<typename B>
static void push_buffer(B& buf, const char* c){
	while(*c) push_buffer(buf,*c++);
}


typedef struct params_s {
    int len;
    int num1;
    int num2;
    char pad_character;
    int do_padding;
    int left_flag;
} params_t;

static int g_panic_mode = 0;


extern "C" void panic_mode() {
	if(g_panic_mode) return;
	g_panic_mode = 1;
}

extern "C" void printk_setup(printk_write_t outwrite,printk_options_t options){
	assert(outwrite);
	std::swap(_printk_write, outwrite);
	if(outwrite == empty_write) {
		g_queue_flush(); // flush it
	}
}


extern "C" void putsk(const char* str){
	_printk_write(str,strlen(str));
}
extern "C" void writek(const uint8_t* data, size_t len){
	// ignores line options
	_printk_write((const char*)data,len);
}
#if 0

static void padding( const int l_flag, params_t *par)
{
    if (par->do_padding && l_flag && (par->len < par->num1))
        for (int i=par->len; i<par->num1; i++)
        	printk_putchar( par->pad_character);
}

static void outs( const char* lp, params_t *par)
{
    /* pad on left if needed                         */
    par->len = strlen( lp);
    padding( !(par->left_flag), par);
    /* Move string to the buffer                     */
    while (*lp && (par->num2)--)     putck( *lp++);

    /* Pad on right if needed                        */
    /* CR 439175 - elided next stmt. Seemed bogus.   */
    /* par->len = strlen( lp);                       */
    padding( par->left_flag, par);
}

/*---------------------------------------------------*/
/*                                                   */
/* This routine moves a number to the output buffer  */
/* as directed by the padding and positioning flags. */
/*                                                   */

void outnum( const long n, const long base, params_t *par)
{
	caddr_t cp;
    int negative;
    char outbuf[32];
    const char digits[] = "0123456789ABCDEF";
    unsigned long num;

    /* Check if number is negative                   */
    if (base == 10 && n < 0L) {
        negative = 1;
        num = -(n);
    }
    else{
        num = (n);
        negative = 0;
    }

    /* Build number (backwards) in outbuf            */
    cp = outbuf;
    par->len=0;
    do {
        *cp++ = digits[(int)(num % base)];
        ++par->len;
    } while ((num /= base) > 0);
    if (negative){
    	++par->len;
    	 *cp++ = '-';
    }
    *cp-- = 0;

    /* Move the converted number to the buffer and   */
    /* add in the padding where needed.              */
    padding( !(par->left_flag), par);
    while (cp >= outbuf)
    	printk_putchar( *cp--);
    padding( par->left_flag, par);
}
/*---------------------------------------------------*/
/*                                                   */
/* This routine gets a number from the format        */
/* string.                                           */


#define ISDIGIT(X)  ((X) >= '0' && (X) <= '9' )
static int getnum( const char** linep)
{
    int n;
    const char*  cp;

    n = 0;
    cp = *linep;
    while (ISDIGIT(*cp))
        n = n*10 + ((*cp++) - '0');
    *linep = cp;
    return(n);
}

/*
 * Print a character on console or users terminal.  If destination is
 * the console then the last MSGBUFS characters are saved in msgbuf for
 * inspection later.
 */
static void kputchar(int c, int flags, struct tty *tp)
{
	printk_putchar(c);

    extern int msgbufmapped;
    register struct msgbuf *mbp;

    if (panicstr)
        constty = NULL;
    if ((flags & TOCONS) && tp == NULL && constty) {
        tp = constty;
        flags |= TOTTY;
    }
    if ((flags & TOTTY) && tp && tputchar(c, tp) < 0 &&
        (flags & TOCONS) && tp == constty)
        constty = NULL;
    if ((flags & TOLOG) &&
        c != '\0' && c != '\r' && c != 0177 && msgbufmapped) {
        mbp = msgbufp;
        if (mbp->msg_magic != MSG_MAGIC) {
            bzero((caddr_t)mbp, sizeof(*mbp));
            mbp->msg_magic = MSG_MAGIC;
        }
        mbp->msg_bufc[mbp->msg_bufx++] = c;
        if (mbp->msg_bufx < 0 || mbp->msg_bufx >= MSG_BSIZE)
            mbp->msg_bufx = 0;
        /* If the buffer is full, keep the most recent data. */
        if (mbp->msg_bufr == mbp->msg_bufx) {
            if (++mbp->msg_bufr >= MSG_BSIZE)
                mbp->msg_bufr = 0;
        }
    }
    if ((flags & TOCONS) && constty == NULL && c != '\0')
        (*v_putc)(c);

}
#endif
/*
 * Put a number (base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated (preceded?)
 * buffer.
 */
struct number_settings {
	enum FLAGS {
		GROUP = (1<<0),
		LEFT = (1<<1),
		SHOWSIGN = (1<<2),
		SPACE = (1<<3),
		ALT = (1<<4),
		ZERO = (1<<4),
	};
	size_t base;
	size_t width;
	size_t max;
	char pad_begin;
	char pad_end;
	int flags;
	constexpr number_settings(size_t base, size_t width, size_t max, char pad_begin,char pad_end) :
			base(base),
			width(width),
			max(max),
			pad_begin(pad_begin),
			pad_end(pad_end) ,flags(0){}
	constexpr number_settings():
			base(10),
			width(0),
			max(0),
			pad_begin(' '),
			pad_end(' '),flags(0) {}
	void clear() { *this = number_settings(); }


	template<uint32_t A,uint32_t LENGTH>
	constexpr static inline bool is_between(uint32_t c) { return c >= A && c <= (A + (LENGTH-1)); }
	template<uint32_t BASE>
	constexpr static inline bool is_digit(uint32_t c) {
		return BASE < 10 ?
				is_between<'0',BASE>(c) :
				is_between<'0',10>(c) || is_between<'a',(BASE-10)>(c) || is_between<'a',(BASE-10)>(c);
	}
	const char* parse_number_arg(const char* cnum, size_t& value) {
		while(is_digit<10>(*cnum))
			value = (value * 10) + (*cnum++ - '0');
		return cnum;
	}
	const char* parse_flags(const char* fmt) {
		int f = 0;
		while(1) {
			switch(*fmt){
			case '\'': f |= FLAGS::GROUP; break;
			case '-': f |= FLAGS::LEFT; break;
			case '+': f |= FLAGS::SHOWSIGN; break;
			case ' ': f |= FLAGS::SPACE; break;
			case '#': f |= FLAGS::ALT; break;
			case '0': f |= FLAGS::ZERO; break;
			default:
				flags = f;
				return fmt;
			}
			fmt++;
		}
	}
	// very simple parser, ends at the start of the type
	const char* parse(const char* fmt) {
		// order is important
		if(fmt[0] == '%'){
		clear();
	      fmt=parse_flags(fmt);
	      fmt=parse_number_arg(fmt,width);
	      if(*fmt == '.') fmt=parse_number_arg(fmt+1,max);
	      if(flags & FLAGS::ZERO) pad_begin = '0';
	      else if(flags & FLAGS::SPACE) pad_begin = ' ';
		}
	    return fmt;
	}
};


// carful, this also counts the sign
template<typename T,  typename E> // E is for enable!
struct digit_info;

template<typename T, typename = typename std::enable_if<(std::is_integral<T>::value && sizeof(T) == 4)>::type>
struct digit_info {
	static constexpr bool is_signed = std::is_signed<T>::value;
	using type = T;
	static constexpr size_t type_size = sizeof(T);
	template<size_t BASE>
	typename std::enable_if<(BASE == 10 && is_signed),size_t>::type
	static inline digit_count(T x){
		  return
		    (x < 0 ? digit_count<BASE>(-x) + 1:
		    (x < 10 ? 1 :
		    (x < 100 ? 2 :
		    (x < 1000 ? 3 :
		    (x < 10000 ? 4 :
		    (x < 100000 ? 5 :
		    (x < 1000000 ? 6 :
		    (x < 10000000 ? 7 :
		    (x < 100000000 ? 8 :
		    (x < 1000000000 ? 9 :
		    10))))))))));
	}
	template<size_t BASE>
	typename std::enable_if<(BASE == 10 && !is_signed),size_t>::type
	static inline digit_count(T x){
		  return
		    (x < 10 ? 1 :
		    (x < 100 ? 2 :
		    (x < 1000 ? 3 :
		    (x < 10000 ? 4 :
		    (x < 100000 ? 5 :
		    (x < 1000000 ? 6 :
		    (x < 10000000 ? 7 :
		    (x < 100000000 ? 8 :
		    (x < 1000000000 ? 9 :
		    10)))))))));
	}
	template<size_t BASE>
	typename std::enable_if<BASE == 16,size_t>::type
	static inline digit_count(T x){
	  return
	    (x < 0x10 ? 1 :
	    (x < 0x100 ? 2 :
	    (x < 0x1000 ? 3 :
	    (x < 0x10000 ? 4 :
	    (x < 0x100000 ? 5 :
	    (x < 0x1000000 ? 6 :
	    (x < 0x10000000 ? 7 : 8)))))));
	}
	template<size_t BASE>
	typename std::enable_if<BASE == 8,size_t>::type
	static inline digit_count(T x){
	  return
		(x < 010 ? 1 :
		(x < 0100 ? 2 :
		(x < 01000 ? 3 :
		(x < 010000 ? 4 :
		(x < 0100000 ? 5 :
		(x < 01000000 ? 6 :
		(x < 010000000 ? 7 :
		(x < 0100000000 ? 8 :
		(x < 01000000000 ? 9 :
		(x < 010000000000 ? 10 :
		(x < 0100000000000 ? 11 :
		(x < 01000000000000 ? 12 :
		(x < 010000000000000 ? 13 :
		(x < 0100000000000000 ? 14 :
		(x < 01000000000000000 ? 15 :	16)))))))))))))));
	}
	template<size_t BASE>
	typename std::enable_if<BASE == 2,size_t>::type
	static inline digit_count(T x){ return 32; }
	static inline  size_t digit_count(T x, size_t base) {
		return base == 10 ? digit_count<10>(x) : base == 8 ? digit_count<8>(x) : digit_count<16>(x);
	}
	template<typename F>
	static void sprintn(F&& put_func, type ul, size_t base)
	{                   /* A long in base 8, plus NULL. */
	    do {
	    	put_func("0123456789abcdef"[ul % base]);
			ul /= base;
	    } while (ul);
	}
	template<typename F>
	static void printnumber(F&& put_func, type ul, const number_settings& settings) {
		number_settings s = settings;
		const size_t ndigits = digit_info<T>::digit_count(ul,s.base);
	    if (s.width && (s.width > ndigits) > 0){
	    	s.width -= ndigits;
	    	while (s.width-- && s.max--)
	    		put_func(s.pad_begin); // print the pad
	    }
	    sprintn(put_func, ul, s.base);
	    if(s.max >= ndigits) s.max -=ndigits;
	    while (s.max--) put_func(s.pad_end);
	}
};




#ifdef LOGTIME
template<typename F>
void print_timestamp(F&& put_func,struct timeval * tv) {
	put_func('[');
	digit_info<uint32_t>::printnumber(put_func,tv->tv_sec,number_settings(10, 4,4,' ',' '));
	put_func('.');
	digit_info<uint32_t>::printnumber(put_func,tv->tv_usec,number_settings(10,0,4,' ','0'));
	put_func(']');
	put_func(' ');
}
#else
#define print_timestamp(TV)
#endif
/*
 * Scaled down version of printf(3).
 *
 * Two additional formats:
 *
 * The format %b is supported to decode error registers.
 * Its usage is:
 *
 *  printf("reg=%b\n", regval, "<base><arg>*");
 *
 * where <base> is the output base expressed as a control character, e.g.
 * \10 gives octal; \20 gives hex.  Each arg is a sequence of characters,
 * the first of which gives the bit number to be inspected (origin 1), and
 * the next characters (up to a control character, i.e. a character <= 32),
 * give the name of the register.  Thus:
 *
 *  kprintf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 *
 * would produce output:
 *
 *  reg=3<BITTWO,BITONE>
 *
 * The format %r passes an additional format string and argument list
 * recursively.  Its usage is:
 *
 * fn(char *fmt, ...)
 * {
 *  va_list ap;
 *  va_start(ap, fmt);
 *  printf("prefix: %r: suffix\n", fmt, ap);
 *  va_end(ap);
 * }
 *
 * Space or zero padding and a field width are supported for the numeric
 * formats only.
 */


#define ISEOL(c) ((c) == '\n' || (c) == '\r')
template<typename F>
void kprintf(F&& put_func, const char *fmt, int flags,  va_list ap)
{
    char *p;
    int ch;
    number_settings ns;
#ifdef LOGTIME
    struct timeval time_stamp;
    gettimeofday(&time_stamp, NULL);
#endif
    for (;;) {
    	ch = *(uint8_t *)fmt++;
        if(ch == '\0') return;
		if(ch != '%') {
			put_func(ch);
			if(ch == '\n') print_timestamp(put_func,&time_stamp);
			continue;
		}
        // we have a %
        fmt = ns.parse(fmt);

    	switch(ch) { // only handle single ch's for right now
    	case '%':
    	case 'c':	// chars and %
        	put_func(ch);
        	break;
    	case 's': // string
    	{ // no formating support yet
            const char* p = va_arg(ap, char *);
            while ((ch = *p++)) put_func(ch);
    	}
        break;
    	case 'd': // no float uspport
    	case 'i':
    	{ // no formating support yet
            int32_t v = va_arg(ap, int32_t);
        	digit_info<int32_t>::printnumber(put_func,v,ns);
    	}
        break;
    	case 'u':
    	{
            uint32_t v = va_arg(ap, uint32_t);
        	digit_info<uint32_t>::printnumber(put_func,v,ns);
    	}
        break;
        case 'o':
    	{
            uint32_t v = va_arg(ap, uint32_t);
            ns.base = 8;
        	digit_info<uint32_t>::printnumber(put_func,v,ns);
    	}
    	break;
        case 'x':
    	{
            uint32_t v = va_arg(ap, uint32_t);
            ns.base = 16;
        	digit_info<uint32_t>::printnumber(put_func,v,ns);
    	}
    	break;
        case 'p': // for pointers
    	{
            uint32_t v = va_arg(ap, uint32_t);
            ns.width = 8;
            ns.base = 16;
            put_func('0');
            put_func('x');
        	digit_info<uint32_t>::printnumber(put_func,v,ns);
    	}
    	break;
    	// not standard
    	case 'b':
    	{ // no formating support yet
    		uint32_t v = va_arg(ap, uint32_t);
            put_func('0');
            put_func('b');
            for(int tmp=31;tmp>=0; tmp--) {
            	put_func((v & (1<<tmp)) !=0? '1': '0');
            }
    	}
        case 'r': // recursive
            p = va_arg(ap, char *);
            kprintf(put_func, p, flags, va_arg(ap, va_list));
            break;
        case 't': // time stamp format, its two digts, first and second
			{
				struct timeval* t = va_arg(ap, struct timeval*);
				print_timestamp(put_func,t);
			}
        	break;
#if 0
            p = va_arg(ap, char *);
            q = ksprintn(ul, *p++, NULL);
            while ((ch = *q--))
            	printk_putchar(ch);

            if (!ul)
                break;

            for (tmp = 0; (n = *p++); ) {
                if (ul & (1 << (n - 1))) {
                	printk_putchar(tmp ? ',' : '<');
                    for (; (n = *p) > ' '; ++p)
                    	printk_putchar(n);
                    tmp = 1;
                } else
                    for (; *p > ' '; ++p)
                        continue;
            }
            if (tmp)
            	printk_putchar('>');
#endif
        default:
        	assert(0);
        	break; // unsupported
    	}
    }
}



extern "C" void panic(const char*fmt,...){
	assert(0);
	panic_mode();
	putsk("\r\n\r\n");
	va_list ap;
	va_start(ap,fmt);
	default_queue_t buffer;
	auto put_func = [&buffer](char c) { push_buffer(buffer,c); };
	kprintf(put_func, fmt,0,ap);
	//vprintk(fmt,ap);
	va_end(ap);
	//_printk_putchar = panic_usart;
	while(1);
}

extern "C" void printk(const char* fmt, ...){
	va_list ap;
	va_start(ap,fmt);
	default_queue_t buffer;
	auto put_func = [&buffer](char c) { push_buffer(buffer,c); };
	kprintf(put_func, fmt,0,ap);
	//vprintk(fmt,ap);
	va_end(ap);
}

/*
 * Note that stdarg.h and the ANSI style va_start macro is used for both
 * ANSI and traditional C compilers.
 */






