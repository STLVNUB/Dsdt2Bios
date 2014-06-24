//
//  Dsdt2Bios.h
//  Dsdt2Bios
//
//  Created by tuxuser on 24.06.14.
//  Copyright (c) 2014 FredWst. All rights reserved.
//

#ifndef Dsdt2Bios_Dsdt2Bios_h
#define Dsdt2Bios_Dsdt2Bios_h

#include "capstone.h"

extern char cr[0x10000];

#define DEBUG 1

#ifdef DEBUG
    #define dprintf(...) \
            do { sprintf(cr, __VA_ARGS__); } while (0)
#else
    #define dprintf(...)
#endif

#define printf(...) \
        do { sprintf(cr, __VA_ARGS__); } while (0)

#define HEADER_SIZE             3

#define HEADER_DSDT             "DSDT"
#define HEADER_UNPATCHABLE_SECTION  ".ROM"

struct platform
{
	cs_arch arch;
	cs_mode mode;
	unsigned char *code;
	size_t size;
	char *comment;
	cs_opt_type opt_type;
	cs_opt_value opt_value;
};

#endif
