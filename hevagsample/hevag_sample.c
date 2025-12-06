/* SIE CONFIDENTIAL
 * HE-VAG DLL version 4.0.0.1
 * Copyright (C) 2016 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audiodec_cpu.h"
#include "hevag_adapter.h"

typedef struct SceVagSampleOption {
	int32_t bitPcm;					/* PCM data word size */
	uint8_t outFormat;				/* Output interleaved format */
	char *pInputFileName;			/* Input file name */
	char *pOutputFileName;			/* Output file name */
} SceVagSampleOption;

typedef struct SceVagHeader {
	uint8_t id[4];					/* 'VAGp' */
	uint32_t version;				/* Version */
	uint8_t reserved1[4];			/* Always 0 */
	uint32_t dataSize;				/* Data size */
	uint32_t fs;					/* Frequency [Hz] */
	uint8_t reserved2[10];			/* Always 0 */
	uint8_t ch;						/* Number of channels (0 and 1 as 1ch, 2 as 2ch...) */
	uint8_t reserved3;				/* Always 0 */
	uint8_t name[16];				/* Name */
} SceVagHeader;

#define DPRINTF 				printf_s
#define SCE_COPYRIGHT			("SIE CONFIDENTIAL \nCopyright(C) 2016 Sony Interactive Entertainment Inc. All Rights Reserved.\nCopyright 2010 Sony Corporation.\n")

#define INPUTBUFFERSIZE_MAX		(SCE_AUDIODEC_CPU_HEVAG_BLOCK_SIZE * SCE_AUDIODEC_CPU_HEVAG_MAX_CHANNELS + sizeof(SceVagHeader))
#define OUTPUTBUFFERSIZE_MAX	(SCE_AUDIODEC_CPU_HEVAG_BLOCK_SAMPLES * sizeof(float) * SCE_AUDIODEC_CPU_HEVAG_MAX_CHANNELS)
#define WAVE_FORMAT_PCM			(1)
#define WAVE_FORMAT_IEEEFLOAT	(3)
/* RIFF chunk : 12 bytes */
/* fmt chunk : 24bytes */
/* data chunk(only ID and chunk size) : 8bytes */
#define FMTCHUNK_DATASIZE		(16)
#define PCM_HEADER_SIZE			(44)

static void setDefaultOption(SceVagSampleOption *pSampleOption)
{
	pSampleOption->bitPcm = SCE_AUDIODEC_CPU_WORD_SZ_16BIT;
	pSampleOption->outFormat = SCE_AUDIODEC_CPU_HEVAG_INTERLEAVED_OUTPUT;
	pSampleOption->pInputFileName = NULL;
	pSampleOption->pOutputFileName = NULL;
}

static int32_t checkOption(int argc,
							char* argv[],
							SceVagSampleOption *pSampleOption)
{
	int32_t i = 0, fileCnt = 0;
	
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-bitPCM") == 0) {
			if (++i >= argc) {
				DPRINTF("Error : value of -bitPCM N failure\n");
				return -1;
			}
			pSampleOption->bitPcm = atoi(argv[i]);
			switch (pSampleOption->bitPcm) {
			case 0:
				pSampleOption->bitPcm = SCE_AUDIODEC_CPU_WORD_SZ_16BIT;
				break;
			case 1:
				pSampleOption->bitPcm = SCE_AUDIODEC_CPU_WORD_SZ_FLOAT;
				break;
			default:
				DPRINTF("Error : value of -bitPCM (%d) failure\n", pSampleOption->bitPcm);
				return -1;
				break;
			}
		} else if (strcmp(argv[i], "-outformat") == 0) {
			if (++i >= argc) {
				DPRINTF("Error : value of -outformat N failure\n");
				return -1;
			}
			pSampleOption->outFormat = atoi(argv[i]);
			switch (pSampleOption->outFormat) {
			case 0:
				pSampleOption->outFormat = SCE_AUDIODEC_CPU_HEVAG_INTERLEAVED_OUTPUT;
				break;
			case 1:
				pSampleOption->outFormat = SCE_AUDIODEC_CPU_HEVAG_NON_INTERLEAVED_OUTPUT;
				break;
			default:
				DPRINTF("Error : value of -outformat (%d) failure\n", pSampleOption->outFormat);
				return -1;
				break;
			}
		} else {
			if (fileCnt == 0) {
				pSampleOption->pInputFileName = argv[i];
			} else if (fileCnt == 1) {
				pSampleOption->pOutputFileName = argv[i];
			} else {
				DPRINTF("Error : Number of file or Unsupported option failure\n");
				DPRINTF("        Input file (%s)\n", pSampleOption->pInputFileName);
				DPRINTF("        Output file (%s)\n", pSampleOption->pOutputFileName);
				return -1;
			}
			fileCnt++;
		}
	}
	if ((pSampleOption->pInputFileName == NULL) || (pSampleOption->pOutputFileName == NULL)) {
		DPRINTF("Error : Number of file or Unsupported option failure\n");
		DPRINTF("        Input file (%s)\n", pSampleOption->pInputFileName);
		DPRINTF("        Output file (%s)\n", pSampleOption->pOutputFileName);
		return -1;
	}
	return SCE_OK;
}

static void setDecodeParam(SceVagHeader *pVagHeader,
							SceVagSampleOption sampleOption,
							 SceAudiodecCpuParamHevag *pHevagDecodeParam)
{
	int32_t i;
	
	pHevagDecodeParam->uiSize = sizeof(SceAudiodecCpuParamHevag);
	for (i = 0; i < 4; i++) {
		pHevagDecodeParam->ucId[i] = pVagHeader->id[i];
	}
	pHevagDecodeParam->uiVersion = pVagHeader->version;
	pHevagDecodeParam->uiDataSize = pVagHeader->dataSize;
	pHevagDecodeParam->uiFs = pVagHeader->fs;
	pHevagDecodeParam->ucNumChannels = pVagHeader->ch;
	/* ucNumChannels == 0 : 1ch */
	if (pHevagDecodeParam->ucNumChannels == 0) {
		pHevagDecodeParam->ucNumChannels = 1;
	}
	for (i = 0; i < 16; i++) {
		pHevagDecodeParam->ucName[i] = pVagHeader->name[i];
	}
	pHevagDecodeParam->iBwPcm = sampleOption.bitPcm;
	pHevagDecodeParam->ucOutFormat = sampleOption.outFormat;
}

static int setPointaLong(uint8_t *p,
							uint32_t d)
{
	p[0] = (uint8_t)(d & 0xff);
	p[1] = (uint8_t)((d >> CHAR_BIT) & 0xff);
	p[2] = (uint8_t)((d >> CHAR_BIT*2) & 0xff);
	p[3] = (uint8_t)((d >> CHAR_BIT*3) & 0xff);
	return 4;
}

static int setPointaShort(uint8_t *p,
							uint16_t d)
{
	p[0] =  d & 0xff;
	p[1] = (d >> CHAR_BIT) & 0xff;
	return 2;
}

static void setPcmHeader(SceAudiodecCpuParamHevag hevagDecodeParam,
							SceAudiodecCpuHevagInfo heavgDecodeInfo,
							uint32_t totalOutputSize,
							uint8_t *pPcmHeader)
{
	uint32_t bytePerSample = 0;
	
	/* create RIFF chunk */
	*pPcmHeader++ = 'R';
	*pPcmHeader++ = 'I';
	*pPcmHeader++ = 'F';
	*pPcmHeader++ = 'F';
	pPcmHeader += setPointaLong(pPcmHeader, (totalOutputSize + PCM_HEADER_SIZE - 8));
	*pPcmHeader++ = 'W';
	*pPcmHeader++ = 'A';
	*pPcmHeader++ = 'V';
	*pPcmHeader++ = 'E';
	/* create fmt chunk */
	*pPcmHeader++ = 'f';
	*pPcmHeader++ = 'm';
	*pPcmHeader++ = 't';
	*pPcmHeader++ = ' ';
	pPcmHeader += setPointaLong(pPcmHeader, FMTCHUNK_DATASIZE);
	switch (hevagDecodeParam.iBwPcm) {
	case SCE_AUDIODEC_CPU_WORD_SZ_FLOAT:
		pPcmHeader += setPointaShort(pPcmHeader, WAVE_FORMAT_IEEEFLOAT);
		bytePerSample = sizeof(float);
		break;
	case SCE_AUDIODEC_CPU_WORD_SZ_16BIT:
	default:
		pPcmHeader += setPointaShort(pPcmHeader, WAVE_FORMAT_PCM);
		bytePerSample = sizeof(short);
		break;
	}
	if (heavgDecodeInfo.ucNumChannels == 0) {
		heavgDecodeInfo.ucNumChannels = 1;
	}
	/* channel */
	pPcmHeader += setPointaShort(pPcmHeader, heavgDecodeInfo.ucNumChannels);
	/* sampling rate */
	pPcmHeader += setPointaLong(pPcmHeader, heavgDecodeInfo.uiFs);
	/* average bytes / second */
	pPcmHeader += setPointaLong(pPcmHeader, (heavgDecodeInfo.uiFs * heavgDecodeInfo.ucNumChannels * bytePerSample));
	/* block align */
	pPcmHeader += setPointaShort(pPcmHeader, (heavgDecodeInfo.ucNumChannels * bytePerSample));
	/* bit / sample */
	pPcmHeader += setPointaShort(pPcmHeader, (bytePerSample * CHAR_BIT));
	/* create data chunk */
	*pPcmHeader++ = 'd';
	*pPcmHeader++ = 'a';
	*pPcmHeader++ = 't';
	*pPcmHeader++ = 'a';
	pPcmHeader += setPointaLong(pPcmHeader, totalOutputSize);
}

static void setDisplayUsage(void)
{
	DPRINTF("*----------------------------------------------------------------------*\n");
	DPRINTF("SIEI HE-VAG Decoder Sample\n");
	DPRINTF(SCE_COPYRIGHT);
	DPRINTF("Usage : hevagdec_sample [-<option>] file1 file2\n");
	DPRINTF("      : decode file1 to file2 \n");
	DPRINTF("*--- option list            -------------------------------------------*\n");
	DPRINTF("  -bitPCM N    : PCM data word size\n");
	DPRINTF("                 0 : 16bit PCM output(default)\n");
	DPRINTF("                 1 : float PCM output\n");
	DPRINTF("  -outformat N : Output interleaved format\n");
	DPRINTF("                 0 : Interleaved output(default)\n");
	DPRINTF("                 1 : Non Interleaved output\n");
	DPRINTF("*----------------------------------------------------------------------*\n");
}

int main(int argc, char *argv[])
{
	int32_t ret = SCE_OK;
	uint32_t frameCnt = 0, readSize = 0, totalOutputSize = 0;
	FILE *pInputFile = NULL;
	FILE *pOutputFIle = NULL;
	errno_t err;
	uint8_t inputBuf[INPUTBUFFERSIZE_MAX], outputBuf[OUTPUTBUFFERSIZE_MAX], pcmHeaderBuf[PCM_HEADER_SIZE];
	SceAudiodecCpuParamHevag hevagDecodeParam;
	SceAudiodecCpuCtrl audiodecCpuCtrl;
	SceAudiodecCpuHevagInfo heavgDecodeInfo;
	SceAudiodecCpuAuInfo auInfo;
	SceAudiodecCpuPcmItem pcmItem;
	SceAudiodecCpuResource resource;
	SceVagSampleOption sampleOption;
	
	memset(inputBuf, 0, INPUTBUFFERSIZE_MAX);
	memset(outputBuf, 0, OUTPUTBUFFERSIZE_MAX);
	memset(pcmHeaderBuf, 0, PCM_HEADER_SIZE);
	memset(&hevagDecodeParam, 0, (sizeof(SceAudiodecCpuParamHevag)));
	memset(&audiodecCpuCtrl, 0, (sizeof(SceAudiodecCpuCtrl)));
	memset(&heavgDecodeInfo, 0, (sizeof(SceAudiodecCpuHevagInfo)));
	memset(&auInfo, 0, (sizeof(SceAudiodecCpuAuInfo)));
	memset(&pcmItem, 0, (sizeof(SceAudiodecCpuPcmItem)));
	memset(&resource, 0, (sizeof(SceAudiodecCpuResource)));
	
	setDefaultOption(&sampleOption);
	ret = checkOption(argc,
						argv,
						&sampleOption);
	if (ret != SCE_OK) {
		DPRINTF("Error : Input Option failure\n");
		goto error;
	}
	
	/* Input file(.vag) */
	err = fopen_s(&pInputFile, sampleOption.pInputFileName, "rb");
	if (err != 0) {
		DPRINTF("Error : Input file open failure (%s)\n", sampleOption.pInputFileName);
		ret = -1;
		goto error;
	}
	/* Output file(.pcm) */
	err = fopen_s(&pOutputFIle, sampleOption.pOutputFileName, "wb");
	if (err != 0) {
		DPRINTF("Error : Output file open failure (%s)\n", sampleOption.pOutputFileName);
		ret = -1;
		goto error;
	}
	
	/* Get HE-VAG header */
	if (fread_s(inputBuf, (sizeof(SceVagHeader)), (sizeof(SceVagHeader)), 1, pInputFile) != 1) {
		DPRINTF("Error : Reading failure of HE-VAG header\n");
		ret = -1;
		goto error;
	}
	
	/* Set Decode Parameter */
	setDecodeParam((SceVagHeader *)inputBuf,
					sampleOption,
					&hevagDecodeParam);
	
	/* Set audio decoder control structure */
	audiodecCpuCtrl.pParam = &hevagDecodeParam;
	audiodecCpuCtrl.pBsiInfo = &heavgDecodeInfo;
	audiodecCpuCtrl.pAuInfo = &auInfo;
	audiodecCpuCtrl.pPcmItem = &pcmItem;
	
	/* Set audio decoder resources management structure(only uiSize) */
	resource.uiSize = sizeof(SceAudiodecCpuResource);
	
	/* execute QueryMemSize() */
	ret = sceAudiodecCpuQueryMemSize(&audiodecCpuCtrl,
										&resource,
										SCE_AUDIODEC_CPU_TYPE_HEVAG);
	if (ret != SCE_OK) {
		DPRINTF("Error : sceAudiodecCpuQueryMemSize() = 0x%x\n", ret);
		goto error;
	}
	
	/* Get work memory area */
	resource.pWorkMemAddr = malloc(resource.uiWorkMemSize);
	if (resource.pWorkMemAddr == NULL) {
		DPRINTF("Error : Memory allocation failure\n");
		ret = -1;
		goto error;
	}

	/* execute InitDecoder() */
	ret = sceAudiodecCpuInitDecoder(&audiodecCpuCtrl,
									&resource,
									SCE_AUDIODEC_CPU_TYPE_HEVAG);
	if(ret != SCE_OK){
		DPRINTF("Error : sceAudiodecCpuInitDecoder() = 0x%x\n", ret);
		goto error;
	}
	
	/* Set pcm information structure */
	pcmItem.uiSize = sizeof(SceAudiodecCpuPcmItem);
	pcmItem.pPcmAddr = (uint8_t *)outputBuf;
	switch (hevagDecodeParam.iBwPcm) {
	case SCE_AUDIODEC_CPU_WORD_SZ_FLOAT:
		pcmItem.uiPcmSize = SCE_AUDIODEC_CPU_HEVAG_BLOCK_SAMPLES * sizeof(float) * hevagDecodeParam.ucNumChannels;
		break;
	case SCE_AUDIODEC_CPU_WORD_SZ_16BIT:
	default:
		pcmItem.uiPcmSize = SCE_AUDIODEC_CPU_HEVAG_BLOCK_SAMPLES * sizeof(short) * hevagDecodeParam.ucNumChannels;
		break;
	}
	
	/* Header is stocked in the 1st decode */
	/* Set Access unit information structure */
	auInfo.uiSize = sizeof(SceAudiodecCpuAuInfo);
	auInfo.pAuAddr = (uint8_t *)(inputBuf);
	readSize = SCE_AUDIODEC_CPU_HEVAG_BLOCK_SIZE * hevagDecodeParam.ucNumChannels;
	auInfo.uiAuSize = readSize + sizeof(SceVagHeader);
	
	/* Set BSI information structure(only uiSize) */
	heavgDecodeInfo.uiSize = sizeof(SceAudiodecCpuHevagInfo);
	
	/* create blank space for PCM header */
	if (fwrite(pcmHeaderBuf, PCM_HEADER_SIZE, 1, pOutputFIle) < 1) {
		DPRINTF("Error : Writing failure of PCM Header(blank)\n");
		ret = -1;
		goto error;
	}
	while (fread_s((inputBuf + sizeof(SceVagHeader)), INPUTBUFFERSIZE_MAX, readSize, 1, pInputFile) == 1) {
		frameCnt++;
		/* header is unnecessary from the 2nd decode */
		if (frameCnt != 1) {
			auInfo.uiAuSize = readSize;
			auInfo.pAuAddr = (uint8_t *)(inputBuf + sizeof(SceVagHeader));
		}
		/* execute Decode() */
		ret = sceAudiodecCpuDecode(&audiodecCpuCtrl,
									&resource,
									SCE_AUDIODEC_CPU_TYPE_HEVAG);
		if(ret != SCE_OK){
			DPRINTF("Error : sceAudiodecCpuDecode() = 0x%x\n", ret);
			DPRINTF("        Frame count       : %d count\n", frameCnt);
			goto error;
		}
		/* through the 1st decode */
		if (frameCnt != 1) {
			/* Finish decode process with end point of loop information(0x07) */
			if (heavgDecodeInfo.ucVagLoopFlag[0] == 0x07) {
				break;
			}
			if (pcmItem.uiPcmSize != 0) {
				if (fwrite(outputBuf, pcmItem.uiPcmSize, 1, pOutputFIle) < 1) {
					DPRINTF("Error : Writing failure of Decode Data\n");
					ret = -1;
					goto error;
				}
				totalOutputSize += pcmItem.uiPcmSize;
			}
		}
	}
	/* update PCM header */
	setPcmHeader(hevagDecodeParam,
					heavgDecodeInfo,
					totalOutputSize,
					pcmHeaderBuf);
	rewind(pOutputFIle);
	if (fwrite(pcmHeaderBuf, PCM_HEADER_SIZE, 1, pOutputFIle) < 1) {
		DPRINTF("Error : Writing failure of PCM Header\n");
		ret = -1;
		goto error;
	}
	DPRINTF("*--- HE-VAG Decode Finish -------------------------------------------*\n");
	DPRINTF("        Frame count       : %d count\n", frameCnt);
	DPRINTF("        Total Output Size : %d bytes\n", totalOutputSize);
	DPRINTF("*--------------------------------------------------------------------*\n");
	
error:
	if(resource.pWorkMemAddr != NULL){
		free(resource.pWorkMemAddr);
	}
	if(pInputFile != NULL){
		fclose(pInputFile);
	}
	if(pOutputFIle != NULL){
		fclose(pOutputFIle);
	}
	if (ret != SCE_OK) {
		setDisplayUsage();
	}
	return ret;
}
