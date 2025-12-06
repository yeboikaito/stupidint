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



#ifndef _AT9ENC_H
#define _AT9ENC_H

#include <libatrac9.h>
#include <stdio.h>



int at9Enc(FILE *infile,
		   FILE *outfile,
		   int  bitrate,
		   int  supframe,
		   int  dualMode,
		   int  nbands,
		   int  isband,
		   int  gradMode,
		   int  wband,
		   int  bex,
		   int  slc);


#endif /* _AT9ENC_H */
