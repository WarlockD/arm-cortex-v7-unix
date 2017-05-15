#ifndef _HELPER_TYPES_H_
#define _HELPER_TYPES_H_


#include_next <sys\time.h>

#ifdef __cplusplus

typedef struct ::timeval timeval_t;

static  inline timeval_t operator+(const timeval_t& l, const timeval_t& r){
	timeval_t tv = { l.tv_sec + r.tv_sec, l.tv_usec + r.tv_usec };
	if (tv.tv_usec >= 1000000) { tv.tv_sec++; tv.tv_usec -= 1000000; }
	return tv;
}
static inline timeval_t operator-(const timeval_t& l, const timeval_t& r){
	timeval_t tv = { l.tv_sec - r.tv_sec, l.tv_usec - r.tv_usec };
	if (tv.tv_usec < 0) { tv.tv_sec--; tv.tv_usec += 1000000; }
	return tv;
}
static inline timeval_t& operator+=(timeval_t& l, const timeval_t& r){
	l.tv_sec += r.tv_sec;
	l.tv_usec += r.tv_usec;
	if (l.tv_usec >= 1000000) { l.tv_sec++; l.tv_usec -= 1000000; }
	return l;
}
static inline timeval_t& operator-=(timeval_t& l, const timeval_t& r){
	l.tv_sec -= r.tv_sec;
	l.tv_usec -= r.tv_usec;
	if (l.tv_usec < 0) { l.tv_sec--; l.tv_usec += 1000000; }
	return l;
}
static constexpr inline bool operator<(const timeval_t& l, const timeval_t& r){
	return l.tv_sec == r.tv_sec ? l.tv_usec < r.tv_usec : l.tv_sec < r.tv_sec;
}
static constexpr inline bool operator>(const timeval_t& l, const timeval_t& r){
	return l.tv_sec == r.tv_sec ? l.tv_usec > r.tv_usec : l.tv_sec > r.tv_sec;
}
static constexpr inline bool operator==(const timeval_t& l, const timeval_t& r){
	return l.tv_usec == r.tv_usec && l.tv_sec == r.tv_sec;
}
static constexpr inline bool operator!=(const timeval_t& l, const timeval_t& r){
	return l.tv_usec != r.tv_usec || l.tv_sec != r.tv_sec;
}
static constexpr inline bool operator>=(const timeval_t& l, const timeval_t& r){
	return l == r || l > r;
}
static constexpr inline bool operator<=(const timeval_t& l, const timeval_t& r){
	return l == r || l < r;
}


#endif



#endif
