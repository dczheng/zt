#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <sys/select.h>
#include <errno.h>

#include "ctrl.h"

char *
to_bytes(uint64_t b) {
	static char buf[64];
	char *p = "KMGT";
	int i;

	for (i=0; i<4; i++)
		if (b<(1ULL<<(10*(i+1))))
			break;
	if (i==0)
		sprintf(buf, "%lu B", b);
	else
		sprintf(buf, "%.2f %cB", (double)b/(1ULL<<(10*i)), p[i-1]);
	return buf;
}

void
dump_hex(unsigned char *buf, int n) {
    for (int i = 0; i < n; i++)
        printf("0x%X ", buf[i]);
    printf("\n");
    fflush(stdout);
}

void
dump(unsigned char *buf, int n) {
    struct CtrlInfo *ctrl_info = NULL;
    int i;

    for (i = 0; i < n; i++) {
        if (buf[i] >= 0x20 && buf[i] <= 0x7E) {
            printf("%c", buf[i]);
            continue;
        }
        if (ISCTRL(buf[i])) {
            get_ctrl_info(buf[i], &ctrl_info);
            printf(" %s ", ctrl_info->name);
            continue;
        }
        printf(" 0x%X ", buf[i]);
    }
    printf("\n");
    fflush(stdout);
}

long
get_time(void) {
    struct timespec tv;
    
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return tv.tv_sec * NANOSEC + tv.tv_nsec;
}
