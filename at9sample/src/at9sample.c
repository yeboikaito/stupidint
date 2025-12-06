/********************************************
	(C) Copyright 2009, 2010, 2011, 2012, 2013, 2014, 2015 Sony Corporation
	All Rights Reserved.
*********************************************/
/* SIE CONFIDENTIAL
 ATRAC9(TM) DLL version 4.0.1.1
 *
 *      Copyright (C) 2016 Sony Interactive Entertainment Inc.
 *                        All Rights Reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <libatrac9.h>
#include "at9wave.h"
#include "at9sample.h"
#include "at9enc.h"
#include "at9dec.h"

static int _checkOptions(int argc,
						 char* argv[],
						 char* pMode,
						 int *pBitrate,
						 int *pSupFrame,
						 int *pDual,
						 int *pNbands,
						 int *pIsband,
						 int *pGradMode,
						 int *pWband,
						 int *pBex,
						 int *pSlc,
						 int *pBitPerSample,
						 char **ppInFileName,
						 char **ppOutFileName);

static void _finish(void);

static void _showLibraryVersion(void);

int 
main (int argc, char* argv[])
{
	int status;
	char mode = 0;	
    int bitrate  = 128;	/* default bitrate 128kbps */
	int superframe = TRUE, dualMode = FALSE;
	int nbands   = SCE_AT9_PARAM_UNSET;
	int isband   = SCE_AT9_PARAM_UNSET;
	int gradMode = SCE_AT9_PARAM_UNSET;
	int wband = FALSE;
	int bex = FALSE;
	int slc = FALSE;
	int bitPerSample = SCE_AT9_WORD_LENGTH_16BIT; /* default:16bit PCM */
	char *infilename = NULL;
	char *outfilename = NULL;
    FILE *infile = NULL;
	FILE *outfile = NULL;
#ifdef _WIN32
	errno_t err;
#else
	char tmpInfilename[256];
	char tmpOutfilename[256];
#endif /* _WIN32 */

	status = _checkOptions(argc, 
						   argv,
						   &mode,
						   &bitrate,
						   &superframe,
						   &dualMode,
						   &nbands,
						   &isband,
						   &gradMode,
						   &wband,
						   &bex,
						   &slc,
						   &bitPerSample,
						   &infilename,
						   &outfilename);

	if (status < 0) {
		DPRINTF("_checkOptions() = 0x%x\n", status);
		goto finish;
	}

#ifdef _WIN32
	err = fopen_s(&infile, infilename, "rb");
	if (err != 0) {
#else /* _WIN32 */
	memset(tmpInfilename, 0, 256);
	MEMCPY(tmpInfilename, infilename, 256);
	infile = fopen(tmpInfilename, "rb");
	if (infile == NULL) {
#endif /* _WIN32 */
		DPRINTF("cannot open %s\n", infilename);
		status = SCE_ERROR_MAIN_CANNOT_OPEN_INFILE;
		goto finish;
	}
#ifdef _WIN32
	err = fopen_s(&outfile, outfilename, "wb");
	if (err != 0) {
#else /* _WIN32 */
	memset(tmpOutfilename, 0, 256);
	MEMCPY(tmpOutfilename, outfilename, 256);
	outfile = fopen(tmpOutfilename, "wb");
	if (outfile == NULL) {
#endif /* _WIN32 */
		DPRINTF("cannot open %s\n", outfilename);
		status = SCE_ERROR_MAIN_CANNOT_OPEN_OUTFILE;
		goto finish;
	}


	switch (mode) {
	case 'e':
		status = at9Enc(infile, 
			            outfile,
				        bitrate,
					    superframe,
						dualMode,
						nbands,
						isband,
						gradMode,
						wband,
						bex,
						slc);
		break;
	case 'd':
		status = at9Dec(infile,
						outfile,
						bitPerSample);
		break;
	default:
		status = SCE_ERROR_MAIN_OPTION_ILLEGAL_MODE;
		break;
	}
finish:
	if (infile != NULL) {
		fclose(infile);
	}
	if (outfile != NULL) {
		fclose(outfile);
	}
	if (status < 0) {
		_finish();
	}
	return EXIT_SUCCESS;
}


static int 
_checkOptions(int argc,
			  char* argv[],
			  char* pMode,
			  int *pBitrate,
			  int *pSupFrame,
			  int *pDual,
			  int *pNbands,
			  int *pIsband,
			  int *pGradMode,
			  int *pWband,
			  int *pBex,
			  int *pSlc,
			  int *pBitPerSample,
			  char **ppInFileName,
			  char **ppOutFileName)
{
	int status = SCE_SUCCESS;
	int i;
	int nFiles = 0;

    for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-e") == 0) {
			*pMode = 'e'; /* encode mode */
		} else if (strcmp(argv[i], "-d") == 0) {
			*pMode = 'd'; /* decode mode */
		} else if (strcmp(argv[i], "-br") == 0) {
			if (++i >= argc) {
				status = SCE_ERROR_MAIN_OPTION_ILLEGAL_BITRATE;
				break;
			}
			*pBitrate = atoi(argv[i]);
		} else if (strcmp(argv[i], "-supframeon") == 0) {
			*pSupFrame = TRUE;
		} else if (strcmp(argv[i], "-supframeoff") == 0) {
			*pSupFrame = FALSE;			
		} else if (strcmp(argv[i], "-dual") == 0) {
			*pDual = TRUE;	
		} else if (strcmp(argv[i], "-nbands") == 0) {
			if (++i >= argc) {
				status =  SCE_ERROR_MAIN_OPTION_ILLEGAL_NBANDS;
				break;
			}
			*pNbands = atoi(argv[i]);
		} else if (strcmp(argv[i], "-isband") == 0) {
			if (++i >= argc) {
				status = SCE_ERROR_MAIN_OPTION_ILLEGAL_ISBAND;
				break;
			}
			*pIsband = atoi(argv[i]);
		} else if (strcmp(argv[i], "-gradmode") == 0) {
			if (++i >= argc) {
				status = SCE_ERROR_MAIN_OPTION_ILLEGAL_GRADMODE;
				break;
			}
			*pGradMode = atoi(argv[i]);
		} else if (strcmp(argv[i], "-wband") == 0) {
			*pWband = TRUE;
		} else if (strcmp(argv[i], "-bex") == 0) {
			*pBex = TRUE;
		} else if (strcmp(argv[i], "-slc") == 0) {
			*pSlc = TRUE;
		} else if (strcmp(argv[i], "-int16") == 0) {
			*pBitPerSample = SCE_AT9_WORD_LENGTH_16BIT;
		} else if (strcmp(argv[i], "-int24") == 0) {
			*pBitPerSample = SCE_AT9_WORD_LENGTH_24BIT;
		} else if (strcmp(argv[i], "-float") == 0) {
			*pBitPerSample = SCE_AT9_WORD_LENGTH_FLOAT;
		} else {
			if (nFiles >= SCE_MAX_FILES) {
				status = SCE_ERROR_MAIN_OPTION_NUMBER_OF_FILES;
				break;
			}
			if (nFiles == 0) {
				*ppInFileName  = argv[i];
			} else {
				*ppOutFileName = argv[i];
			}
			nFiles++;
		}
    }
	if (status != SCE_SUCCESS) {
		goto fail;
	}
	if (*pMode != 'd' && *pMode != 'e') {
		status = SCE_ERROR_MAIN_OPTION_ILLEGAL_MODE;
		goto fail;
	}

	if (nFiles != SCE_MAX_FILES) {
		status = SCE_ERROR_MAIN_OPTION_NUMBER_OF_FILES;
		goto fail;
	}
fail:
	return status;
}

static void 
_finish(void)
{
    DPRINTF("SIEI ATRAC9 Sample Version %s\n", SCE_AT9SAMPLE_VERSION);
    DPRINTF(SCE_COPYRIGHT);

	_showLibraryVersion();

    DPRINTF("Usage : at9sample [-<option>] file1 file2\n");
    DPRINTF("  -e : encode file1 to file2 \n");
    DPRINTF("       (file1: 16, 24bit Integer PCM, IEEE float PCM /\n");
	DPRINTF("               48kHz,24kHz,12kHz Linear FORMAT_PCM wav file)\n");

	DPRINTF("below options only affects to encoding mode(when not, they will be ignored)\n");
    DPRINTF("  -br N       : specify the bitrate Nkbps\n");
	DPRINTF("  -supframeon : set superframe encode\n");
	DPRINTF("  -supframeoff: don't set superframe encode\n");
	DPRINTF("  -dual       : use dualMode encode mode\n");
	DPRINTF("  -nbands N   : Number of quantized bands [3,..,18]\n");
	DPRINTF("  -isband N   : Intensity start band [-1, 3,..,nbands]\n");
	DPRINTF("  -gradmode N : Encoding mode  [0,1,2,3, and 4]\n");
	DPRINTF("                0-3: gradient of the quantization noise.\n");
	DPRINTF("                4  : 0-3 is selected automatically.\n");
	DPRINTF("                Specify the small number in case of tone sound source.\n");
	DPRINTF("                Or specify the high number in case of noisy sound source. \n");
	DPRINTF("  -wband      : Wide band encoding mode. default=OFF\n");
	DPRINTF("  -bex        : Band Extension (default=OFF)\n");
	DPRINTF("  -slc        : LFE Super Low Cut ON (default=OFF)\n");
    DPRINTF("\n");
    DPRINTF("  -d : decode to file1 to file2(Linear PCM wav file)\n");
	DPRINTF("  -int16      : 16bit Integer PCM output(default)\n");
	DPRINTF("  -int24      : 24bit Integer PCM output\n");
	DPRINTF("  -float      : IEEE float PCM output\n");
    exit(EXIT_FAILURE);
}

static void 
_showLibraryVersion(void)
{
	int majorVersion, minorVersion, branchVersion, ver;

	ver = sceAt9GetVersion();
	majorVersion = 0xff & (ver >> 16);
	minorVersion = 0xff & (ver >> 8);
	branchVersion = 0xff & ver;
	if (!branchVersion) {
		DPRINTF("\t*** built w/ ATRAC9 library version %d.%02d ***\n",
				majorVersion, minorVersion);
	}
	else {
		DPRINTF("\t*** built w/ ATRAC9 library version %d.%02d.%02d ***\n",
				majorVersion, minorVersion, branchVersion);
	}
	return;
}
