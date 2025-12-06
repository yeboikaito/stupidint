/********************************************
	(C) Copyright 2009, 2010, 2011, 2012, 2013, 2014 Sony Corporation
	All Rights Reserved.
*********************************************/
/* SIE CONFIDENTIAL
 ATRAC9(TM) DLL version 4.0.1.1
 *
 *      Copyright (C) 2014 Sony Interactive Entertainment Inc.
 *                        All Rights Reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "at9wave.h"
#include "at9sample.h"

/* FORMAT_PCM */
#ifndef WAVE_FORMAT_PCM
# define WAVE_FORMAT_PCM 	(1)
# define WAVE_FORMAT_IEEEFLOAT (3)
#endif	/* WAVE_FORMAT_PCM */

#ifndef WAVE_FORMAT_EXTENSIBLE
# define WAVE_FORMAT_EXTENSIBLE (0xfffe)
#endif	/* WAVE_FORMAT_EXTENSIBLE */


/* size of "fmt" chunk without "chunk type" and "chunk size" fields */
/* (may be sizeof(_pcmextHeader)) */
#define PCM_EXT_FMT_CHUNK_DATA_SIZE	(40)


#define AT9_HEADER_CB_SIZE	     (34)	/* be set to "cbSize" field */
#define SONY_ATRAC9_WAVEFORMAT_VERSION	(1)
#define SONY_ATRAC9_WAVEFORMAT_VERSION_BEX	(2)

/******************************************************************************
	for FORMAT_AT9 new RIFF-wave header(uses WAVE_FORMAT_EXTENSIBLE)
******************************************************************************/

#ifndef KSDATAFORMAT_SUBTYPE_PCM
static const GUID KSDATAFORMAT_SUBTYPE_PCM
= {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
/* 00000001-0000-0010-8000-00aa00389b71 */
#endif	/* KSDATAFORMAT_SUBTYPE_PCM */

#ifndef KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
static const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
= {0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
/* 00000003-0000-0010-8000-00aa00389b71 */
#endif	/* KSDATAFORMAT_SUBTYPE_IEEE_FLOAT */

#ifndef KSDATAFORMAT_SUBTYPE_ATRAC9
static const GUID KSDATAFORMAT_SUBTYPE_ATRAC9 =
{ 0x47e142d2, 0x36ba, 0x4d8d, { 0x88, 0xfc, 0x61, 0x65, 0x4f, 0x8c, 0x83, 0x6c } };
/* {47E142D2-36BA-4d8d-88FC-61654F8C836C} */
#endif	/* KSDATAFORMAT_SUBTYPE_ATRAC9 */

static unsigned long atrac9dwChannelMask[9] = {
	0,
	AT9_CHMAP_MONO,
	AT9_CHMAP_STEREO,
	AT9_CHMAP_3_0CH,
	AT9_CHMAP_4_0CH,
	AT9_CHMAP_5_0CH,
	AT9_CHMAP_5_1CH,
	AT9_CHMAP_7_0CH,
	AT9_CHMAP_7_1CH
};		/* n = nChannels */



/******************************************************************************
	common functinos
******************************************************************************/
static int _cmpGUID(const GUID *s1, const GUID *s2);
static int _writeLong(char *p, unsigned long d);
static int _writeShort(char *p, unsigned short d);
static unsigned long _fgetLong(FILE *fp);
static unsigned short _fgetShort(FILE *fp);


/* the function "guidcpy()" copies GUID s2 to s1 */
/* and returns s1 */
GUID
*GUIDCpy(GUID *s1, const GUID *s2)
{
    s1->Data1 = s2->Data1;
    s1->Data2 = s2->Data2;
    s1->Data3 = s2->Data3;
    MEMCPY(s1->Data4, s2->Data4, sizeof(s2->Data4));

    return s1;
}

/******************************************************************************
	parse wave file
******************************************************************************/
int
parseWaveHeader(FILE    *fp, 
				FmtChunk *fmt, 
				long    *total, 
				int    *encdelay)
{
    unsigned long	chunkLength;
    int 	format = SCE_ERROR_MAIN_UNKNOW_FORMAT;
    int 	dataChunkStart = 0;
    unsigned short Samples;
    PcmHeader 	*pcmHeader;
    PcmExtHeader	*pcmextHeader;
	At9Header    *pat9header;
	int nSmplchunk = 0, loopcnt = 0;
	size_t readSize;

    do {
		if (_fgetLong(fp) != 0x46464952) { /* 'RIFF' */
			return SCE_ERROR_HEADER_FATAL_ERROR;
		}
		chunkLength = _fgetLong(fp) - 4; /* rLen, RIFF chunk length */
		if (chunkLength % 2 == 1) {
			chunkLength += 1; /* if chunkLength is odd, add padding data length */
		}
		if (_fgetLong(fp) == 0x45564157) { /* 'WAVE' */
			break;
		}
    } while (!fseek(fp, chunkLength, SEEK_CUR));

    while (!feof(fp)) {
		unsigned long chunkType = _fgetLong(fp);
		if (feof(fp))
			break;
		chunkLength = _fgetLong(fp);
	
		if (chunkLength % 2 == 1)
			chunkLength += 1; /* if chunkLength is odd, add padding data length */
	
		switch (chunkType) {
		case 0x20746d66: /* 'fmt ' */
			format = _fgetShort(fp);	/* wFormatTag */
			switch (format) {
			case WAVE_FORMAT_PCM:	/* WAVE_FORMAT_PCM */
			case WAVE_FORMAT_IEEEFLOAT:	/* WAVE_FORMAT_IEEEFLOAT */
				pcmHeader = &fmt->pcmHeader;
				pcmHeader->wFormatTag 	   = format;
				pcmHeader->nChannels 	   = _fgetShort(fp);
				pcmHeader->nSamplesPerSec  = _fgetLong(fp);
				pcmHeader->nAvgBytesPerSec = _fgetLong(fp);
				pcmHeader->nBlockAlign	   = _fgetShort(fp);
				pcmHeader->wBitsPerSample  = _fgetShort(fp);

				/* rest is unknown data. just skip them */
				chunkLength -= PCM_FMT_CHUNK_DATA_SIZE;
				if (format == WAVE_FORMAT_IEEEFLOAT) {
					format = FORMAT_IEEEFLOAT;
				} else {
					format = FORMAT_PCM;
				}
				break;

			case WAVE_FORMAT_EXTENSIBLE: /* FORMAT_AT9 | EXTENSIBLE_PCM */
				pcmextHeader = &fmt->pcmextHeader;
				pcmextHeader->wFormatTag 	= format;
				pcmextHeader->nChannels 	= _fgetShort(fp);
				pcmextHeader->nSamplesPerSec 	= _fgetLong(fp);
				pcmextHeader->nAvgBytesPerSec 	= _fgetLong(fp);
				pcmextHeader->nBlockAlign 	= _fgetShort(fp);
				pcmextHeader->wBitsPerSample 	= _fgetShort(fp);
				pcmextHeader->cbSize 		= _fgetShort(fp);
				Samples 			= _fgetShort(fp);	
				/* one of wValidBitsPerSample,wSamplesPerBlock or wReserved, */
				/* which will be found after "SubFormat" parsed */
				pcmextHeader->dwChannelMask 	= _fgetLong(fp);
				pcmextHeader->SubFormat.Data1 	= _fgetLong(fp);
				pcmextHeader->SubFormat.Data2 	= _fgetShort(fp);
				pcmextHeader->SubFormat.Data3 	= _fgetShort(fp);
				readSize = fread(pcmextHeader->SubFormat.Data4, 1, 8, fp); 

				if (readSize != 8) {
					return SCE_ERROR_COMMON_FREAD_ERROR;
				}

				if (_cmpGUID(&pcmextHeader->SubFormat, &KSDATAFORMAT_SUBTYPE_ATRAC9)) {
					pat9header = &fmt->at9Header;
					pat9header->Samples.wSamplesPerBlock 	= Samples;
					pat9header->dwVersionInfo 			= _fgetLong(fp);
					if ((pat9header->dwVersionInfo != SONY_ATRAC9_WAVEFORMAT_VERSION)
						&& (pat9header->dwVersionInfo != SONY_ATRAC9_WAVEFORMAT_VERSION_BEX)) {
						return SCE_ERROR_HEADER_VERSION;
					}
					readSize = fread(pat9header->configData, 1, 4, fp);
					if (readSize != 4) {
						return SCE_ERROR_COMMON_FREAD_ERROR;
					}
					readSize = fread(pat9header->Reserved, 1, 4, fp);

					if (readSize != 4) {
						return SCE_ERROR_COMMON_FREAD_ERROR;
					}
					/* rest is unknown data. just skip them */
					chunkLength -= AT9_FMT_CHUNK_DATA_SIZE;
					format = FORMAT_AT9;
				}
				else if (_cmpGUID(&pcmextHeader->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) {
					pcmextHeader->Samples.wValidBitsPerSample = Samples;
					chunkLength -= PCM_EXT_FMT_CHUNK_DATA_SIZE;
					format = FORMAT_PCM;
					/* rest is unknown data. just skip them */
				}
				else if (_cmpGUID(&pcmextHeader->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
					pcmextHeader->Samples.wValidBitsPerSample = Samples;
					chunkLength -= PCM_EXT_FMT_CHUNK_DATA_SIZE;
					format = FORMAT_IEEEFLOAT;
					/* rest is unknown data. just skip them */
				}
				else {	/* unsupported SubFormat */
					return SCE_ERROR_HEADER_FATAL_ERROR;
				}
				break;
			default:	/* unsupported wFormatTag */
				return SCE_ERROR_HEADER_FATAL_ERROR;
			}
			break;
		case 0x74636166: /* 'fact' */
			*total = _fgetLong(fp);
			chunkLength -= 4;
			/* Extended fact Chunk*/
			if (chunkLength >= 8) {
				_fgetLong(fp);
				*encdelay = _fgetLong(fp);
				chunkLength -= 8;
			}
			break;

		case 0x61746164: /* 'data' */
			if ((format == FORMAT_PCM)
				|| format == FORMAT_IEEEFLOAT) {
				if (fmt->pcmHeader.wBitsPerSample != 0 && fmt->pcmHeader.nChannels != 0) {
					*total = chunkLength
						/ (fmt->pcmHeader.wBitsPerSample/8*fmt->pcmHeader.nChannels);
				} else {
					return SCE_ERROR_HEADER_FATAL_ERROR;
				}
			}
			if (dataChunkStart != 0)
				return SCE_ERROR_HEADER_FATAL_ERROR;
			dataChunkStart = ftell(fp);
			if (dataChunkStart == -1){
				return SCE_ERROR_HEADER_FATAL_ERROR;
			}
			break;

		case 0x6c706d73: /* 'smpl' */
			/* command line option is prior to the 'smpl' chunk */
			if (fseek(fp, 28, SEEK_CUR)){ /* skip all the unused data */
				return SCE_ERROR_HEADER_FATAL_ERROR;
			}
			loopcnt = _fgetLong(fp);
			chunkLength -= 32;
			if (loopcnt != 0) { /* check Num Sample Loops */
				int loopstart, loopend, i;

				DPRINTF("This tool doesn't support loop\n");
				DPRINTF("The following loop is disregarded.\n");
				if (fseek(fp, 4, SEEK_CUR)){/* skip all the unused data */
					return SCE_ERROR_HEADER_FATAL_ERROR; 
				}
				chunkLength -= 4;
				for (i = 0; i < loopcnt; i++) {
					if (fseek(fp, 8, SEEK_CUR)){/* skip all the unused data */
						return SCE_ERROR_HEADER_FATAL_ERROR; 
					}
					loopstart = _fgetLong(fp);
					loopstart -= *encdelay;
					loopend = _fgetLong(fp);
					loopend -= *encdelay;
					if (fseek(fp, 8, SEEK_CUR)){/* skip all the unused data */
						return SCE_ERROR_HEADER_FATAL_ERROR; 
					}
					chunkLength -= 24;
					DPRINTF("[loopstart = %8d, loopend = %8d]\n", loopstart, loopend);
				}
			}
			nSmplchunk++;
			break;

		default:	/* unknown chunk */
			break;
		}
	
		/* skip remain of chunk */
		if (fseek(fp, chunkLength, SEEK_CUR)){
			DPRINTF("input file is illegal file or over 2G Byte\n");
			return SCE_ERROR_HEADER_FATAL_ERROR;
		}
    }
    
    /* rewind to data chunk start point */
    if (fseek(fp, dataChunkStart, SEEK_SET)){
		return SCE_ERROR_HEADER_FATAL_ERROR;
    }
	
    return format;
}

int createAt9Header(FILE            *outfile,
					SceAt9CodecInfo *pCodecInfo,
					int             bex,
					int             totalSample)
{
	int status = SCE_SUCCESS;
	int encdelay = pCodecInfo->frameSamples + SCE_AT9_ENCODER_DELAY_SAMPLES;
	int channels = pCodecInfo->channels;
	int nBlockAlign = pCodecInfo->superframeSize;
	int nSamplesPerSec = pCodecInfo->samplingRate;
	int blockSamples = pCodecInfo->frameSamples * pCodecInfo->framesInSuperframe;
	int nAvgBytesPerSec = (int)(0.5 + (nBlockAlign * nSamplesPerSec) / (double)(blockSamples));
	char tmpBuf[AT9_HEADER_SIZE] = {0};
    char *ptr = tmpBuf;
	int dataSize = (totalSample + pCodecInfo->frameSamples + blockSamples - 1) / blockSamples * nBlockAlign;
	int fileSize = AT9_HEADER_SIZE + dataSize;

	/* make main chunk header */
    *ptr++ = 'R'; *ptr++ = 'I'; *ptr++ = 'F'; *ptr++ = 'F';
	ptr += _writeLong(ptr, fileSize - 8);

	/* make wave chunk */
    *ptr++ = 'W'; *ptr++ = 'A'; *ptr++ = 'V'; *ptr++ = 'E';
    *ptr++ = 'f'; *ptr++ = 'm'; *ptr++ = 't'; *ptr++ = ' ';

	ptr += _writeLong (ptr, AT9_FMT_CHUNK_DATA_SIZE);
	ptr += _writeShort(ptr, WAVE_FORMAT_EXTENSIBLE);
	ptr += _writeShort(ptr, channels);
	ptr += _writeLong (ptr, pCodecInfo->samplingRate);
	ptr += _writeLong (ptr, nAvgBytesPerSec);
	ptr += _writeShort(ptr, nBlockAlign);
	ptr += _writeShort(ptr, 0);
	ptr += _writeShort(ptr, AT9_HEADER_CB_SIZE);
	ptr += _writeShort(ptr, blockSamples);
	ptr += _writeLong (ptr, atrac9dwChannelMask[channels]);
	ptr += _writeLong (ptr, KSDATAFORMAT_SUBTYPE_ATRAC9.Data1);
	ptr += _writeShort(ptr, KSDATAFORMAT_SUBTYPE_ATRAC9.Data2);
	ptr += _writeShort(ptr, KSDATAFORMAT_SUBTYPE_ATRAC9.Data3);
	MEMCPY(ptr, KSDATAFORMAT_SUBTYPE_ATRAC9.Data4, 8);	ptr += 8;

	/* FORMAT_AT9 codec information appened to the end of WAVEFORMATEXENSIBLE structure */
	if (bex != FALSE) {
		ptr += _writeLong(ptr, SONY_ATRAC9_WAVEFORMAT_VERSION_BEX);
	} else {
		ptr += _writeLong(ptr, SONY_ATRAC9_WAVEFORMAT_VERSION);
	}
	MEMCPY(ptr, pCodecInfo->configData, 4);   ptr+= 4;
	memset(ptr, 0, 4);		ptr += 4;

	/* fact chunk */
	*ptr++ = 'f'; *ptr++ = 'a'; *ptr++ = 'c'; *ptr++ = 't';

	/* Write Extended fact chunk */
	ptr += _writeLong(ptr, AT9_FACT_CHUNK_DATA_SIZE);
	ptr += _writeLong(ptr, totalSample);
	/* write encdelay (the part of extended)*/
	ptr += _writeLong(ptr, encdelay);
	ptr += _writeLong(ptr, encdelay);

    *ptr++ = 'd'; *ptr++ = 'a'; *ptr++ = 't'; *ptr++ = 'a';
    ptr += _writeLong(ptr, dataSize);

	if (fwrite(tmpBuf, AT9_HEADER_SIZE, 1, outfile) != 1){
		status = SCE_ERROR_COMMON_FWRITE_ERROR;
		DPRINTF("File output error in atracEncCreateHeader\n");
		goto fail;
    }
 fail:
	return status;
}

int
createPcmHeader(FILE *outfile,
				FmtChunk *pInWaveHdr,
				int totalSamples,
				int nBytePerSample)
{
	int status = SCE_SUCCESS;
	char tmpBuf[PCM_HEADER_SIZE] = {0};
    char *ptr = tmpBuf;
	PcmHeader *pcmHdr = &pInWaveHdr->pcmHeader;
	int nAvgBytesPerSec = pcmHdr->nSamplesPerSec * pcmHdr->nChannels * nBytePerSample;
	int nBlockAlign = pcmHdr->nChannels * nBytePerSample;
	int dataSize = pcmHdr->nChannels * nBytePerSample * totalSamples;
	int fileSize = PCM_HEADER_SIZE + dataSize;

/* make main chunk header */
    *ptr++ = 'R'; *ptr++ = 'I'; *ptr++ = 'F'; *ptr++ = 'F';
    ptr += _writeLong(ptr, fileSize - 8);

/* make wave chunk */
    *ptr++ = 'W'; *ptr++ = 'A'; *ptr++ = 'V'; *ptr++ = 'E';
    *ptr++ = 'f'; *ptr++ = 'm'; *ptr++ = 't'; *ptr++ = ' ';

	ptr += _writeLong (ptr, PCM_FMT_CHUNK_DATA_SIZE);
	if ((nBytePerSample * CHAR_BIT) != SCE_AT9_WORD_LENGTH_FLOAT) {
		ptr += _writeShort(ptr, WAVE_FORMAT_PCM);
	} else {
		ptr += _writeShort(ptr, WAVE_FORMAT_IEEEFLOAT);
	}
	ptr += _writeShort(ptr, pcmHdr->nChannels);
	ptr += _writeLong (ptr, pcmHdr->nSamplesPerSec);
	ptr += _writeLong (ptr, nAvgBytesPerSec);
	ptr += _writeShort(ptr, nBlockAlign);
	ptr += _writeShort(ptr, nBytePerSample * CHAR_BIT);
    *ptr++ = 'd'; *ptr++ = 'a'; *ptr++ = 't'; *ptr++ = 'a';
    ptr += _writeLong(ptr, dataSize);

	if (fwrite(tmpBuf, PCM_HEADER_SIZE, 1, outfile) != 1) {
		DPRINTF("output error in atracDecCreateHeader\n");
		status = SCE_ERROR_COMMON_FWRITE_ERROR;
	}
	return status;
}

/* the function "_cmpGUID()" compares two GUIDs and */
/* returns TRUE if they are identical, otherwise returns FALSE */
static int
_cmpGUID(const GUID *s1, const GUID *s2)
{
    return (
		(s1->Data1 == s2->Data1) &&
		(s1->Data2 == s2->Data2) &&
		(s1->Data3 == s2->Data3) &&
		!memcmp(s1->Data4, s2->Data4, 8)
		);
}

static int _writeLong(char *p, unsigned long d)
{
    p[0] = (char)(d & 0xff);
	p[1] = (char)((d >> CHAR_BIT) & 0xff);
    p[2] = (char)((d >> CHAR_BIT*2) & 0xff);
    p[3] = (char)((d >> CHAR_BIT*3) & 0xff);
    return 4;
}

static int _writeShort(char *p, unsigned short d)
{
    p[0] =  d & 0xff;
    p[1] = (d >> CHAR_BIT) & 0xff;
    return 2;
}

/* get 32-bit data from the FILE stream */
static unsigned long _fgetLong(FILE *fp)
{
    unsigned long ret;
    ret  =  fgetc(fp) & 0xff;
    ret |= (fgetc(fp) & 0xff) << (CHAR_BIT);
    ret |= (fgetc(fp) & 0xff) << (CHAR_BIT*2);
    ret |= (fgetc(fp) & 0xff) << (CHAR_BIT*3);
    return ret;
}

/* get 16-bit data from the FILE stream */
static unsigned short _fgetShort(FILE *fp)
{
    unsigned short ret;
    ret  =  fgetc(fp) & 0xff;
    ret |= (fgetc(fp) & 0xff) << CHAR_BIT;
    return ret;
}
