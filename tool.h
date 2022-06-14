#ifndef __TOOL_H__
#define __TOOL_H__

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

long get_time(void);
void dump_hex(unsigned char*, int);
void dump(unsigned char*, int);
char *to_bytes(uint32_t);

#define NANOSEC 1000000000
#define MICROSEC 1000000
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LEN(x) ((int)(sizeof(x) / sizeof(x[0])))
#define SWAP(a,b) do { \
    typeof(a) _t;\
    _t = a;\
    a = b;\
    b = _t;\
} while(0);

#define CASE(value, doing) \
    case value: \
    doing;\
    break;

#define LIMIT(x, a, b) \
    x = (x) < (a) ? (a) : ((x) > (b) ? (b) : (x))

#define BIT_SET(a, t)   (a |= (t))
#define BIT_UNSET(a, t) (a &= ~(t))
#define BIT_GET(a, t)   ((a) & (t))

#define ASSERT(exp, fmt, ...) do { \
    if (!(exp)) { \
        fprintf(stderr, "Assert failed: %s in %s %d, "fmt"\n", \
            #exp, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        _exit(1);\
    }\
} while(0)

#endif
