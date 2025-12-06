/********************************************
	(C) Copyright 2009, 2010, 2011, 2012, 2013, 2014, 2015 Sony Corporation
	All Rights Reserved.
*********************************************/
/* SIE CONFIDENTIAL
 ATRAC9(TM) DLL version 4.0.1.1
 *
 *      Copyright (C) 2018 Sony Interactive Entertainment Inc.
 *                        All Rights Reserved.
 *
 */



#include <libatrac9.h>


#ifndef _AT9SAMPLE_H
#define _AT9SAMPLE_H

#ifndef _WIN32
#include <errno.h>

#define SCE_AT9SAMPLE_VERSION "2.6.5.0"
#else /* _WIN32 */
#define SCE_AT9SAMPLE_VERSION "4.0.1.1"
#endif /* _WIN32 */

#define SCE_COPYRIGHT  "SIE CONFIDENTIAL \nCopyright(C) 2020 Sony Interactive Entertainment Inc.\nCopyright 2009, 2010, 2011, 2012, 2013, 2014, 2015 Sony Corporation\n"

#define SCE_MAX_FILES  (2)

#define SCE_SUCCESS                              (0x00000000)

#define SCE_ERROR_MAIN_CANNOT_OPEN_INFILE        (0x80000000)
#define SCE_ERROR_MAIN_CANNOT_OPEN_OUTFILE       (0x80000001)
#define SCE_ERROR_MAIN_OPTION_ILLEGAL_BITRATE    (0x80000002)
#define SCE_ERROR_MAIN_OPTION_NUMBER_OF_FILES    (0x80000003)
#define SCE_ERROR_MAIN_OPTION_ILLEGAL_MODE       (0x80000004)
#define SCE_ERROR_MAIN_OPTION_ILLEGAL_NBANDS     (0x80000005)
#define SCE_ERROR_MAIN_OPTION_ILLEGAL_ISBAND     (0x80000006)
#define SCE_ERROR_MAIN_OPTION_ILLEGAL_GRADMODE   (0x80000007)
#define SCE_ERROR_MAIN_UNKNOW_FORMAT             (0x80000008)
#define SCE_ERROR_MAIN_OPTION_ILLEGAL_BEX        (0x80000009)

/* Encode Error Code */
#define SCE_ERROR_ENCODE_ILLEGAL_INPUT_FORMAT    (0x81000000)
#define SCE_ERROR_ENCODE_ILLEGAL_BITS_PER_SAMPLE (0x81000001)
#define SCE_ERROR_ENCODE_TOO_SHORT_FILE_SAMPLES  (0x81000002)
#define SCE_ERROR_ENCODE_NOT_SUPPORT_PARAMETER   (0x81000003)
#define SCE_ERROR_ENCODE_INTERNAL_ERROR          (0x81000100)
#define SCE_ERROR_ENCODE_CANNOT_GET_HANDLE       (0x81000101)

/* Decode Error Code */
#define SCE_ERROR_DECODE_ILLEGAL_INPUT_FORMAT    (0x82000000)
#define SCE_ERROR_DECODE_ILLEGAL_PARAM           (0x82000001)
#define SCE_ERROR_DECODE_NOT_SUPPORT_PARAMETER   (0x82000002)
#define SCE_ERROR_DECODE_INTERNAL_ERROR          (0x82000101)
#define SCE_ERROR_DECODE_CANNOT_GET_HANDLE       (0x81000102)

/* Common Error Code */
#define SCE_ERROR_COMMON_FWRITE_ERROR            (0x83000000)
#define SCE_ERROR_COMMON_FSEEK_ERROR             (0x83000001)
#define SCE_ERROR_COMMON_FTELL_ERROR             (0x83000002)
#define SCE_ERROR_COMMON_FREAD_ERROR			 (0x83000003)

/*J Header Error Code */
#define SCE_ERROR_HEADER_VERSION                 (0x84000000)
#define SCE_ERROR_HEADER_FATAL_ERROR             (0x84000FFF)

#define AT9_MIN_SAMPLES                          (1)

#define TRUE  (1)
#define FALSE (0)

#ifdef _WIN32
#define DPRINTF 					printf_s
#define	MEMCPY(dst, src, cnt)		memcpy_s(dst, cnt, src, cnt)
#else /* _WIN32 */
#define DPRINTF 					printf
#define	MEMCPY(dst, src, cnt)		memcpy(dst, src, cnt)
#endif /* _WIN32 */

#endif /* _AT9SAMPLE_H */
