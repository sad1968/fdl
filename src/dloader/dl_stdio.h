/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FDL_STDIO_H_
#define _FDL_STDIO_H_

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void*)0)
#endif /* __cplusplus */
#endif /* NULL */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define _LITTLE_ENDIAN 1

static inline unsigned short EndianConv_16 (unsigned short value)
{
#ifdef _LITTLE_ENDIAN
    return (value >> 8 | value << 8);
#else
    return value;
#endif
}
static inline unsigned int EndianConv_32 (unsigned int value)
{
#ifdef _LITTLE_ENDIAN
    unsigned int nTmp = 0;
	nTmp = (value >> 24 | value << 24);

    nTmp |= ( (value >> 8) & 0x0000FF00);
    nTmp |= ( (value << 8) & 0x00FF0000);
    return nTmp;
#else
    return value;
#endif
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FDL_STDIO_H */

