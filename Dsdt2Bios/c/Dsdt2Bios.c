//
//  DragDropView.m
//  AmiInfoBoard
//
//  Created by Frédéric Geoffroy on 14/04/2014.
//  Copyright (c) 2014 FredWst. All rights reserved.
//

//***************************************************
//  main.c
//  Dsdt2Bios
//
//  Created by FredWst on 09/04/2014.
//
// **************************************************
//
// Dsdt2Bios using capstone lib engine to patch code.
//
// http://www.capstone-engine.org
//
//***************************************************

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "PeImage.h"
#include "Dsdt2Bios.h"

char cr[0x10000];
static csh handle;

static UINT64 insn_detail(csh ud, cs_mode mode, cs_insn *ins)
{
	int i;
    UINT64 r = 0;
	cs_x86 *x86 = &(ins->detail->x86);
    
	for (i = 0; i < x86->op_count; i++)
    {
		cs_x86_op *op = &(x86->operands[i]);
		switch((int)op->type)
        {
			case X86_OP_MEM:
                if ((op->mem.disp > 0x900) && (op->mem.disp < 0xf000) ) r = op->mem.disp;
                break;
			default:
                break;
		}
	}
    return r;
}

static int Disass(unsigned char *X86_CODE64, int CodeSize, int size)
{
    uint64_t address = 0;
    int ret = 0;
	struct platform x64_platform =
    {
			.arch = CS_ARCH_X86,
			.mode = CS_MODE_64,
			.code = (unsigned char *)X86_CODE64,
			.size = CodeSize - 1,
			.comment = "X86 64 (Intel syntax)"
	};
    
	cs_insn *insn;
    
    cs_err err = cs_open(x64_platform.arch, x64_platform.mode, &handle);
    if (err) {
        printf("\n\n\n\n\n\n\n\n%sFailed on cs_open() with error returned: %u\n",cr, err);
        return 0;
    }
        
	if (x64_platform.opt_type)
		cs_option(handle, x64_platform.opt_type, x64_platform.opt_value);
        
	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
        
	size_t count = cs_disasm_ex(handle, x64_platform.code, x64_platform.size, address, 0, &insn);
	if (!count) {
        printf("\n\n\n\n\n\n\n\n%sERROR: Failed to disasm given code!\n",cr);
        cs_close(&handle);
        return 0;
    }
    
    size_t j;
    for (j = 0; j < count; j++)
    {
        if (insn_detail(handle, x64_platform.mode, &insn[j]) == 0)
            continue;
        
        unsigned short *adr = (unsigned short *)&X86_CODE64[insn[j].address+3];
        *adr += size;
        dprintf("%s%s\t%s \t-> \t[0x%x]\n",cr, insn[j].mnemonic, insn[j].op_str,*adr);
        ret = 1;
    }
    // free memory allocated by cs_disasm_ex()
    cs_free(insn, count);
    
    cs_close(&handle);
        
    return ret;
}

int isDSDT(const char *FileName)
{
    int fd;
    unsigned char *data = malloc(HEADER_SIZE+1);
    
    fd = open(FileName, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        free(data);
        return -1; // File not existant
    }
    
    read(fd, data, HEADER_SIZE);
    close(fd);
    
    if(memcmp((const void *)data, HEADER_DSDT, HEADER_SIZE)) {
        free(data);
        return 0;
    }
    
    free(data);
    return 1;
}

int isAmiBoardInfo(const char *FileName)
{
    int fd;
    unsigned char *data = malloc(sizeof(EFI_IMAGE_DOS_HEADER)+1);
    
    fd = open(FileName, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        free(data);
        return -1; // File not existant
    }
    
    read(fd, data, sizeof(EFI_IMAGE_DOS_HEADER));
    close(fd);
    
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)data;
    
    if (HeaderDOS->e_magic != 0x5A4D ) {
        free(data);
        return 0;
    }
    
    free(data);
    return 1;
}

unsigned int Read_AmiBoardInfo(const char *FileName, unsigned char *d,unsigned long *len, unsigned int *Old_Dsdt_Size, unsigned int *Old_Dsdt_Ofs, int Extract)
{
    int fd_amiboard, fd_out;
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    
    printf("");
    
    fd_amiboard = open(FileName, O_RDWR | O_NONBLOCK);
    if (fd_amiboard < 0)
    {
        printf("\n\n\n\n\n\n\n\nFile %s does not exist\n",FileName);
        return 0;
    }
    //Get size of AmiBoardInfo in Header
    *len = read(fd_amiboard, d, 0x3FFFF); // max 256kb
    close(fd_amiboard);
    
    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)d;
    
    if (HeaderDOS->e_magic != 0x5A4D )
    {
        printf("\n\n\n\n\n\n\n\nFile %s has bad header\n",FileName);
        return 0;
    }
    
    for ( *Old_Dsdt_Ofs = 0; *Old_Dsdt_Ofs < *len ; *Old_Dsdt_Ofs+=1)
    {
        if(!memcmp((const void *)&d[*Old_Dsdt_Ofs], HEADER_DSDT, HEADER_SIZE))
        {
            *Old_Dsdt_Size = (*Old_Dsdt_Size << 8) + d[*Old_Dsdt_Ofs+7];
            *Old_Dsdt_Size = (*Old_Dsdt_Size << 8) + d[*Old_Dsdt_Ofs+6];
            *Old_Dsdt_Size = (*Old_Dsdt_Size << 8) + d[*Old_Dsdt_Ofs+5];
            *Old_Dsdt_Size = (*Old_Dsdt_Size << 8) + d[*Old_Dsdt_Ofs+4];
            break;
        }
    }
    
    if ( Extract )
    {
        char *homeDir = getenv("HOME");
        char FileOutput[256];
        strcpy(FileOutput,homeDir);
        strcat(FileOutput,"/Desktop/DSDT-Original.aml");
        remove(FileOutput);
        fd_out = open(FileOutput, O_CREAT | O_RDWR | O_NONBLOCK, 0666);
        write(fd_out,&d[*Old_Dsdt_Ofs],*Old_Dsdt_Size);
        close(fd_out);
        //dprintf("DSDT-Original.aml has been successfully created\n\n");
        if ( Extract == 1 )
        {
            printf("\n\nDSDT-Original.aml and AmiBoardInfo.bin\n");
            printf("%sHAVE BEEN SUCCESSFULLY CREATED ON YOUR DESKTOP\n\n",cr);
        }
        else
        {
            printf("\n\n\n\n\n\n\nDSDT-Original.aml\n\n");
            printf("%sHAS BEEN SUCCESSFULLY CREATED ON YOUR DESKTOP\n\n",cr);
        }
    }
    
    return 1;
}

unsigned int Read_Dsdt(const char *FileName, unsigned char *d, unsigned long len, unsigned int Old_Dsdt_Size, unsigned int Old_Dsdt_Ofs,unsigned short *reloc_padding)
{
    int fd_dsdt, fd_out, i, j;
    unsigned long dsdt_len;
    short size, padding;
    unsigned char *dsdt;
    unsigned short New_Dsdt_Size;
    bool foundDataSection = false;
    
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    EFI_IMAGE_NT_HEADERS64 *HeaderNT;
    EFI_IMAGE_SECTION_HEADER *Section;
    
    dsdt = malloc(0x40000); // max 256 kb
    
    fd_dsdt = open(FileName, O_RDWR | O_NONBLOCK);
    
    if (fd_dsdt < 0)
    {
        printf("\n\n\n\n\n\n\n\nFile %s does not exist\n",FileName);
        free(dsdt);
        return 0;
    }
    //Read DSDT into buffer
    dsdt_len = read(fd_dsdt, dsdt, 0x3FFFF); // max 256 kb
    close(fd_dsdt);
    
    if(memcmp((const void *)dsdt, HEADER_DSDT, HEADER_SIZE))
    {
        printf("\n\n\n\n\n\n\n\nFile %s has bad header\n",FileName);
        free(dsdt);
        return 0;
    }
    
    //Check is not in good place but it's faster to me to put here ;)
    for (i = 0 ; i < len ; i++)
    {
        if(!memcmp((const void *)&d[i], HEADER_UNPATCHABLE_SECTION, HEADER_SIZE))
        {
            printf("\n\n\n\n\n\n\n\nFile has .ROM section, it can't be patched\n");
            free(dsdt);
            return 0;
        }
    }
    
    New_Dsdt_Size = (New_Dsdt_Size << 8) + dsdt[7];
    New_Dsdt_Size = (New_Dsdt_Size << 8) + dsdt[6];
    New_Dsdt_Size = (New_Dsdt_Size << 8) + dsdt[5];
    New_Dsdt_Size = (New_Dsdt_Size << 8) + dsdt[4];
    
    size = New_Dsdt_Size - Old_Dsdt_Size;
    padding = 0x10-(len+size)&0x0f;
    size += padding + *reloc_padding;
    
    if ((len+size) > 0x3FFFF)
    {
        printf("\n\n\n\n\n\n\n\nFinal size > 0x3FFFF not tested aborting\n");
        free(dsdt);
        return 0;
    }
    
    memcpy(&d[Old_Dsdt_Ofs+Old_Dsdt_Size+size],&d[Old_Dsdt_Ofs+Old_Dsdt_Size],len - Old_Dsdt_Ofs - Old_Dsdt_Size);
    memset(&d[Old_Dsdt_Ofs],0x00,New_Dsdt_Size + padding + *reloc_padding);
    memcpy(&d[Old_Dsdt_Ofs],dsdt,New_Dsdt_Size);
    
    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)&d[0];
    HeaderNT = (EFI_IMAGE_NT_HEADERS64 *)&d[HeaderDOS->e_lfanew];
    
    dprintf("%sPatching header\n",cr);
    dprintf("%s---------------\n\n",cr);
    dprintf("%sSizeOfInitializedData       \t0x%x",cr,HeaderNT->OptionalHeader.SizeOfInitializedData);
    HeaderNT->OptionalHeader.SizeOfInitializedData += size;
    dprintf("%s\t -> \t0x%x\n",cr,HeaderNT->OptionalHeader.SizeOfInitializedData);
    dprintf("%sSizeOfImage                 \t0x%x",cr,HeaderNT->OptionalHeader.SizeOfImage);
    HeaderNT->OptionalHeader.SizeOfImage += size;
    dprintf("%s\t -> \t0x%x\n",cr,HeaderNT->OptionalHeader.SizeOfImage);
    
    for ( i = 0; i < EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES ;i++)
    {
        if ( HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress != 0 )
        {
            dprintf("%sDataDirectory               \t0x%x",cr,HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress);
            HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress += size;
            dprintf("%s\t -> \t0x%x\n\n",cr,HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress);
        }
    }
    
    Section = (EFI_IMAGE_SECTION_HEADER *)&d[HeaderDOS->e_lfanew+sizeof(EFI_IMAGE_NT_HEADERS64)];
    dprintf("%sPatching sections\n",cr);
    dprintf("%s-----------------\n\n",cr);
    
    for ( i = 0 ; i < HeaderNT->FileHeader.NumberOfSections; i++)
    {
        if (!strcmp((char *)&Section[i].Name, ".data"))
        {
            foundDataSection = true;
            dprintf("%sName                         \t%s\t -> \t %s\n",cr,Section[i].Name,Section[i].Name);
            dprintf("%sPhysicalAddress             \t0x%x",cr,Section[i].Misc.PhysicalAddress);
            Section[i].Misc.PhysicalAddress += size;
            dprintf("%s\t -> \t0x%x\n",cr,Section[i].Misc.PhysicalAddress);
            dprintf("%sSizeOfRawData               \t0x%x",cr,Section[i].SizeOfRawData);
            Section[i].SizeOfRawData += size;
            dprintf("%s\t -> \t0x%x\n\n",cr,Section[i].SizeOfRawData);
        }
        else if(foundDataSection)
        {
            if (!strcmp((char *)&Section[i].Name,""))
                strcpy((char *)&Section[i].Name,".empty");
            
            dprintf("%sName                        \t%s\t -> \t%s\n",cr,Section[i].Name,Section[i].Name);
            dprintf("%sVirtualAddress              \t0x%x",cr,Section[i].VirtualAddress);
            Section[i].VirtualAddress += size;
            dprintf("%s\t -> \t0x%x\n",cr,Section[i].VirtualAddress);
            dprintf("%sPointerToRawData            \t0x%x",cr,Section[i].PointerToRawData);
            Section[i].PointerToRawData += size;
            dprintf("%s\t -> \t0x%x\n\n",cr,Section[i].PointerToRawData);
            
            if (!strcmp((char *)&Section[i].Name, ".reloc"))
            {
                
                dprintf("%sPatching relocations\n",cr);
                dprintf("%s--------------------\n\n",cr);
                
                
                EFI_IMAGE_BASE_RELOCATION *p;
                UINT16 *s;
                UINT32 Offset = 0;
                UINT32 sizeSection = 0;
                UINT32 index = 0;
                UINT32 OldAdr = 0;
                UINT32 OldOfs = 0;
                do {
                    p = (EFI_IMAGE_BASE_RELOCATION *)((UINT32 *)(&d[Section[i].PointerToRawData]) + Offset);
                    Offset = p->SizeOfBlock / sizeof(UINT32);
                    sizeSection += p->SizeOfBlock;
                    s = (UINT16 *)p + 4;
                    
                    index = 0;
                    dprintf("%sVirtual base address           \t0x%04x",cr,p->VirtualAddress);
                    OldAdr = p->VirtualAddress;
                    if (p->VirtualAddress != 0 )
                        p->VirtualAddress =(len + size) & 0xf000;
                    
                    dprintf("%s\t -> \t0x%04x\n",cr,p->VirtualAddress);
                    
                    for ( j = 0; j < (p->SizeOfBlock - 8); j+=2) {
                        if (*s != 0) {
                            dprintf("%sTable index %i                \t0x%04x",cr,index++, OldAdr + (*s & 0xfff));
                            if (p->VirtualAddress != 0 )
                                *s = 0xa000 + ((*s + size ) & 0xfff);
                            
                            dprintf("%s\t -> \t0x%04x\n",cr,p->VirtualAddress + (*s & 0xfff));
                        }
                        if (p->VirtualAddress != 0 )
                            OldOfs = *s & 0xfff;
                        
                        s++;
                        
                        if ((p->VirtualAddress != 0) && (j < (p->SizeOfBlock - 8 - 4)) && (OldOfs > ((*s +size) & 0xfff))) {
                            *reloc_padding = 0x10 + (0x1000 - OldOfs) & 0xff0;
                            dprintf("%s error %04X \n",cr,*reloc_padding);
                            goto error; //sorry it's not à beautifull end of prog ;)
                        }
                    }
                }while (sizeSection < Section[i].Misc.VirtualSize);
            }
        }
    }
   
    char *homeDir = getenv("HOME");
    char FileOutput[256];
    strcpy(FileOutput,homeDir);
    strcat(FileOutput,"/Desktop/AmiBoardInfo.bin");
    remove(FileOutput);
    dprintf("%sPatching adr in code\n",cr);
    dprintf("%s--------------------\n\n",cr);
    if ( Disass(&d[HeaderNT->OptionalHeader.BaseOfCode],HeaderNT->OptionalHeader.SizeOfCode, size) )
    {
        fd_out = open(FileOutput, O_CREAT | O_RDWR | O_NONBLOCK, 0666 );
        write(fd_out,d,len+size);
        close(fd_out);
        //dprintf("%s\nDSDT-Original.aml has been successfully created\n\n",cr);
        //dprintf("%sAmiBoardInfo.bin has been successfully created\n\n",cr);
    }
    else
        printf("%s\n\n\n\n\n\n\n\nCode not patched, AmiBoardInfo.bin has not been created\n\n", cr);
  
error:
    free(dsdt);
    
    return 1;
}
