/*
 *
 * Copyright (C) 2014 Embest Technology Co., Ltd. <http://www.embest-tech.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  - Neither the name of Embest Technology Co., Ltd. nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS", 
 * it is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <types.h>
#include <mmap.h>
#include <amp_config.h>


u32  PageTable[4096] __attribute__ ((aligned (16*1024))) __attribute__ ((section ("PGTL")));

/* build page table */
u32  build_PGTL (void)
{
    u32  vaddr;
    u32  paddr;
    u32  i;
    u32  *pt_phys = (u32*)VIRT_TO_PHYS(PageTable);

        
    for(i = (sizeof(PageTable) / sizeof(u32)); i; i -= sizeof(u32))
	{
        pt_phys[i]=0;
    }
	
	/*(paddr & (~(1<<20)-1)) | 0x11c0e;*/
	/* 将0x1E000000到0x1FFFFFFF的32m虚拟地址映射到0x1E000000到0x1FFFFFFF的32m物理地址,写回写分配，所有权级可执行和读写，共享，全局所有进程可用 */
    for (paddr = SDRAM_PHYS_BASE, vaddr = SDRAM_VIRT_BASE; vaddr <= (SDRAM_VIRT_BASE + SDRAM_SIZE); vaddr += PGTL_L1_SECT_SIZE, paddr += PGTL_L1_SECT_SIZE)
	{
        pt_phys[VADDR_TO_L1_INDEX(vaddr)] = PGTL_L1_SECT_WBWA_ENTRY(paddr , 0, 0, 0, AP_PRW_URW, 1, 0);
    } //wbwa

	/* 将将0x1FE00000到0x1FFFFFFF的2m虚拟地址映射到0x1FE00000到0x1FFFFFFF的2m物理地址,不缓存,所有权级可执行和读写，共享，全局所有进程可用 */
	for (paddr = SDRAM_PHYS_BASE + DMABUFPHY_OFFSET, vaddr = SDRAM_VIRT_BASE + DMABUFPHY_OFFSET; vaddr <= (SDRAM_VIRT_BASE + SDRAM_SIZE); vaddr += PGTL_L1_SECT_SIZE, paddr += PGTL_L1_SECT_SIZE)
	{
        pt_phys[VADDR_TO_L1_INDEX(vaddr)] = PGTL_L1_SECT_ENTRY(paddr, MEM_CACHE_TYPE(MEM_NC, MEM_NC) , 0, 0, 0, AP_PRW_URW, 1, 0);
    } //nc(no cache)

	/* 将0x20000000到0x27FFFFFF的128m的虚拟地址映射到0xE0000000到0xE7FFFFFF的128m的物理地址，写回写分配，所有权级可执行，共享，全局所有进程可用 */
	for (paddr = FPGA_SDRAM_PHYS_BASE, vaddr = FPGA_SDRAM_VIRT_BASE; vaddr <= (FPGA_SDRAM_VIRT_BASE + (FPGA_SDRAM_SIZE/2)); vaddr += PGTL_L1_SECT_SIZE, paddr += PGTL_L1_SECT_SIZE)
	{
        pt_phys[VADDR_TO_L1_INDEX(vaddr)] = PGTL_L1_SECT_WBWA_ENTRY(paddr , 0, 0, 0, AP_PRW_URW, 1, 0);
    } //wbwa

	/* 将0x28000000到0x2FFFFFFF的128m的虚拟地址映射到0xE8000000到0xEFFFFFFF128m的物理地址，不缓存,所有权级可执行和读写，共享，全局所有进程可用 */
	for (paddr = FPGA_SDRAM_PHYS_BASE + (FPGA_SDRAM_SIZE/2), vaddr = FPGA_SDRAM_VIRT_BASE + (FPGA_SDRAM_SIZE/2); vaddr <= (FPGA_SDRAM_VIRT_BASE + FPGA_SDRAM_SIZE); vaddr += PGTL_L1_SECT_SIZE, paddr += PGTL_L1_SECT_SIZE)
	{
        pt_phys[VADDR_TO_L1_INDEX(vaddr)] = PGTL_L1_SECT_ENTRY(paddr, MEM_CACHE_TYPE(MEM_NC, MEM_NC) , 0, 0, 0, AP_PRW_URW, 1, 0);
    } //nc


	//for linux L1 page table
	/* 将0x00000000到0x000FFFFF的1m虚拟地址映射到0x00000000到0x000FFFFF的1m物理地址。写回写分配，所有权级可执行，共享，全局所有进程可用 */
	pt_phys[VADDR_TO_L1_INDEX(0x0)] = PGTL_L1_SECT_WBWA_ENTRY(0x0 , 0, 0, 0, AP_PRW_URW, 1, 0);


	//io device mapping
	/* 将0xff000000到0xfff00000的15m虚拟地址映射到0xff000000到0xfff00000的15m物理地址，共享的设备区域 */
    for (paddr = 0xfff00000, vaddr = 0xfff00000; vaddr >= 0xff000000; vaddr -= PGTL_L1_SECT_SIZE, paddr -= PGTL_L1_SECT_SIZE)
	{	
        pt_phys[VADDR_TO_L1_INDEX(vaddr)] = PGTL_L1_SECT_ENTRY(paddr, MEM_SHARE_DEVICE , 0, 0, 0, AP_PRW_URW, 1, 0);
    }

    return (u32)pt_phys;
}
