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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <amp_config.h>
#include <bmlog.h>
#include <regs.h>

extern void* usr_stack_end;

//debug only!
/* 串口缓冲水位 */
#define UART_LSR_THRE   0x20            /* Xmit holding register empty */

/* 将一个字符ch输出到串口,可能会忙等待 */
void serial0_putc(char ch)
{
        u32 uart0_va_base;
        uart0_va_base = SOCFPGA_UART0_ADDRESS;
		/* 如果串口缓冲区中数据没达到水位，就将ch放进缓冲区 */
        while ((*(volatile u32 *)(((u32)uart0_va_base)+(5<<2)) & UART_LSR_THRE) == 0);
                *(volatile u32 *)uart0_va_base = ch;
}

/* 将s指向的'\0'结尾的字符串输出到串口 */
void uartputs(char *s)
{
	while(*s != '\0')
		serial0_putc(*s++);		//

	serial0_putc('$');
}



/* 该函数好像未被使用 */
caddr_t _sbrk(int incr){
	
  static char *heap_end = NULL;
  char *prev_heap_end;

  if (heap_end == 0) {
    heap_end = usr_stack_end;
	/* give 2KB area for stacks and use the rest of memory for heap*/
	heap_end += 2048;
  }
  prev_heap_end = heap_end;
  heap_end += incr;
  return (caddr_t) prev_heap_end;
}


/* 将字符串格式化到buf
 * 返回去除'\0'后的字符串长度  
 * buf -- 格式化后的字符串输出的地方
 * size -- 格式化的最大长度
 */
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	int i;

	/* 将格式化后的字符串写入buf中,
	 * 返回格式化后的字符串的长度(如果字符串长度(不包括'\0'的长度)不超过size-1则返回字符串长度，
	 * 如果字符串长度大于或等于size则返回size)
	 */
	i = vsnprintf(buf, size, fmt, args);

	/* 如果格式化后的字符串小于size则返回该字符串的长度 */
	if (i < size)
		return i;

	/* else i>=size */
	/* 如果格式化后的字符串大于或等于size，则返回size-1(即去掉'\0') */
	if (size != 0)
		return size - 1;
	
	/* 如果size为0则返回0 */
	return 0;
}


//将内容写入asp->logbuf但是不输出到uart
void bmlog(const char *fmt, ...)
{
	int len;
	
	va_list args;	//声明args(等价于char *args)
	
	va_start(args, fmt); //args指向第一个变参，即紧接fmt的一个

	/* 将fmt和args格式化的字符串拷贝到asp->logbuf中偏移asp->logindex处，并返回字符串大小 */
	len = vscnprintf(asp->logbuf + asp->logindex, LOG_LEN, fmt, args);
	asp->logindex += len;	//更新logindex

	/* 如果logindex超出正常的logbuf大小(4kb)
     * 则将logbuf后半段的内容拷贝到前面，会将前面的覆盖
	*/
	if(asp->logindex > (LOG_LEN-1) ) {	
		memcpy(asp->logbuf, asp->logbuf + LOG_LEN, asp->logindex - LOG_LEN + 1);
		asp->logindex &= (LOG_LEN - 1);	/* 更新logindex减去LOG_LEN */
		asp->logbuf_overlap = 1;		/* 标记4kb溢出 */
	}
	va_end(args);	//args指向null
}

static char debugbuf[128];

void uartprintf(const char *fmt, ...)
{
	int len;
	int index = 0;
	
	va_list args;
	
	va_start(args, fmt);
	/* 将格式化的字符串写到debugbuf，并返回字符串长度 */
	len = vscnprintf(debugbuf + index, 128 - index, fmt, args);
	index += len;	
	va_end(args);

	/* 如果不是空字符串则通过uartputs将字符串输出到串口 */
	if(index > 0)	
	{
		debugbuf[index] = '\0';
		uartputs(debugbuf);
	}
}


