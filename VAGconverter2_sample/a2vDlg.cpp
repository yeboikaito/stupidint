/* SIE CONFIDENTIAL
 * VAG Converter 2 version 2.5.0.3
 * Copyright(C) 2016 Sony Interactive Entertainment Inc.
 */

#include <memory.h>
#include <afxdlgs.h>
#include <vagconv2.h>
#include "a2v.h"
#include "a2vDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//	AIFF file definitions
//

#define	sEndian(X)		((0xff & (X >> 8)) | ((0xff & X) << 8))
#define	lEndian(X)		(((X >> 24) & 0xff) | ((X >> 8) & 0xff00) | ((X & 0xff00) << 8) | ((X & 0xff) << 24))

#define	AIFF			(0x41494646)
#define	AIFC			(0x41494643)
#define	FVER			(0x46564552)
#define	COMM			(0x434f4d4d)
#define	FORM			(0x464f524d)
#define	SSND			(0x53534e44)
#define	MARK			(0x4d41524b)
#define	INST			(0x494e5354)
#define	MIDI			(0x4d494449)
#define	AESD			(0x41455344)
#define	APPL			(0x4150504c)
#define	COMT			(0x434f4d54)
#define	NAME			(0x4e414d45)
#define	AUTH			(0x41555448)
#define	COPY			(0x434f5059)
#define	ANNO			(0x414e4e4f)

typedef struct ChunkHeader {
	unsigned long		ckID;
	long				ckSize;
} ChunkHeader;

typedef struct ChunkAIFFHeader {
	unsigned long		ckID;
	long				ckSize;
	unsigned long		ckType;
} ChunkAIFFHeader;

typedef struct CommonChunk {
	short				numChannels;
	unsigned long		numSampleFrames;
	short				sampleSize;
	unsigned char		sane[10];
} CommonChunk;

typedef struct Marker {
	short				id;
	unsigned long		position;
	unsigned char		size;
} Marker;

typedef struct SoundDataChunk {
	unsigned long		offset;
	unsigned long		blockSize;
} SoundDataChunk;

//
//	VAG file definitions
//

typedef struct VagHeader {
	unsigned long id;
	unsigned long version;
	unsigned char reserved0[4];
	unsigned long datasize;
	unsigned long fs;
	unsigned char reserved1[12];
	char name[16];
} VagHeader;

//
//	About dialog box class
//

class CAboutDlg : public CDialog
{
public:
	CAboutDlg()
		: CDialog(CAboutDlg::IDD)
		{ }

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

//
//	AIFF to VAG Dialog box class
//

// Message map

BEGIN_MESSAGE_MAP(Ca2vDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SRC, OnBnClickedButtonSrc)
	ON_BN_CLICKED(IDC_BUTTON_DST, OnBnClickedButtonDst)
	ON_BN_CLICKED(IDC_BUTTON_CNV, OnBnClickedButtonCnv)
END_MESSAGE_MAP()

// Construction

Ca2vDlg::Ca2vDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Ca2vDlg::IDD, pParent)
	, m_editSrc(_T(""))
	, m_editDst(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// DDX

void Ca2vDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SRC, m_editSrc);
	DDV_MaxChars(pDX, m_editSrc, 256);
	DDX_Text(pDX, IDC_EDIT_DST, m_editDst);
	DDV_MaxChars(pDX, m_editDst, 256);
	DDX_Control(pDX, IDC_BUTTON_CNV, m_button_cnv);
}

// Initialize

BOOL Ca2vDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	return TRUE;
}

// System command handler

void Ca2vDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// Paint

void Ca2vDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// Query Icon

HCURSOR Ca2vDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void Ca2vDlg::OnBnClickedButtonSrc()
{
	CString csExt = "aif";
	CString csExp = "AIFF Files (*.aif ; *.aiff)|*.aif; *.aiff|All Files (*.*)|*.*||";
	CFileDialog loadDialog(TRUE, csExt, m_editSrc, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, csExp);

	loadDialog.DoModal();
	srcPath = loadDialog.GetPathName();

	dstPath = srcPath;

	if(dstPath.GetLength() > 4){
		if(dstPath.GetAt(dstPath.GetLength()-4) == '.'){
			dstPath = dstPath.GetBufferSetLength(dstPath.GetLength()-4);
		}
		else if(dstPath.GetAt(dstPath.GetLength()-5) == '.'){
			dstPath = dstPath.GetBufferSetLength(dstPath.GetLength()-5);
		}
		dstPath += ".vag";
	}

	noExName = loadDialog.GetFileTitle();
	m_editSrc = srcPath;  // set input path to edit box
	m_editDst = dstPath;  // set output path to edit box
	UpdateData(FALSE);
}

void Ca2vDlg::OnBnClickedButtonDst()
{
	CString csExt = "vag";
	CString csExp = "VAG Files (*.vag)|*.vag|All Files (*.*)|*.*||";
	CFileDialog saveDialog(FALSE, csExt, m_editDst, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, csExp);

	saveDialog.DoModal();
	dstPath = saveDialog.GetPathName();
	m_editDst = dstPath;
	UpdateData(FALSE);
}

void Ca2vDlg::OnBnClickedButtonCnv()
{
	UpdateData(TRUE);
	dstPath = m_editDst;
	srcPath = m_editSrc;

	if(arFp.Open((LPCTSTR)srcPath,CFile::modeRead) == NULL){
		errStr = "Can't open input file : " + srcPath;
		AfxMessageBox(errStr);
		return;
	}

	if (vagFile.Open((LPCTSTR)dstPath,CFile::modeCreate | CFile::modeWrite, &vagFileError ) == NULL){
		arFp.Close();
		errStr = "Can't open output file : " + dstPath;
		AfxMessageBox(errStr);
		return;
	}

	m_button_cnv.EnableWindow(FALSE); // unabled convert button
	UpdateData(TRUE);

	Convert( 0 ); // main convert routine
	arFp.Close();
	vagFile.Close();

	m_button_cnv.EnableWindow(TRUE); // enabled convert button 
	UpdateData(TRUE);
}
//
//
//
float Ca2vDlg::sane2ld( unsigned char *sane )
{
	float tmp;
	float massive;
	short exponent;
	short i;
	short dat;
	unsigned char c2, c3;

	exponent = ( (0x3f & sane[0]) << 8 ) | sane[1] + 1;

	massive = 0.0f;
	c2 = sane[2];
	c3 = sane[3];
	dat = c2 << 8 | c3;
	for( i = 0; i < 16; i++ ) {
		if( (1 & (dat >> (15-i))) != 0 ) {
			massive += ((float)1.0/(1L<<i));
		}
	}
	tmp = massive * (1L << exponent);
	return( tmp );
}

void Ca2vDlg::Convert( short loopon )
{
	ChunkAIFFHeader aChunkAIFF;
	ChunkHeader aChunk;
	CommonChunk aCommonRv, aCommon;
	Marker aMarkRv, aMark[2];
	SoundDataChunk aSndDataChunk;
	CWaitCursor  waitCursor;

	long total, ssnd_pos;
	short i, skip, marks=0, loopflg;	// 0: no loop, 1: loop
	float sampleRate = (float)44100;
	char name[256], c;
	size_t fcount;
	VagHeader aVh;

	memset(aMark,0,sizeof(aMark));

	errStr = "";
	total = 12;
	try{
		readChunkAIFFHead( arFp, &aChunkAIFF );	// FORM XXXX AIFF
	}
	catch(...){
		errStr = "Error in reading AIFF head chunk";
		AfxMessageBox(errStr);
		return;
	}

	if( aChunkAIFF.ckType != AIFF ) {
		errStr = "Sorry, a2v only supports NOT compressed AIFF";
		AfxMessageBox(errStr);
		return;
	}

	while( (total < aChunkAIFF.ckSize) && (total > 0) ) 
	{
		try{
			readChunkHead( arFp, &aChunk );
		}
		catch(...){
			errStr = "Error in reading a chunk head.";
			AfxMessageBox(errStr);
			return;
		}
		total += aChunk.ckSize + sizeof( ChunkHeader );
		skip=0;

		switch( aChunk.ckID ){
			case COMM :
				// ===================== C O M M =====================
				try{
					arFp.Read( &aCommonRv.numChannels, sizeof( short ) );
					arFp.Read( &aCommonRv.numSampleFrames, sizeof( unsigned long ) );
					arFp.Read( &aCommonRv.sampleSize, sizeof( short ) );
					arFp.Read( &aCommonRv.sane, sizeof( char ) * 10 );
				}
				catch(...){
					errStr = "Error in reading AIFF Common chunk.";
					AfxMessageBox(errStr);
					return;
				}

				aCommon.numChannels = sEndian( aCommonRv.numChannels );
				aCommon.numSampleFrames = lEndian( aCommonRv.numSampleFrames );
				aCommon.sampleSize = sEndian( aCommonRv.sampleSize );
				sampleRate = sane2ld( aCommonRv.sane );
				if( aCommon.numChannels > 1 ) {
					errStr = "Sorry, AIFF must be monaural.";
					AfxMessageBox(errStr);
					return;
				}

				if( aCommon.sampleSize != 16 ) {
					errStr = " Sorry, AIFF must be 16 bit PCM.";
					AfxMessageBox(errStr);
					return;
				}
				skip = 1;
				break;

			case APPL :
				// ===================== A P P L =====================
				break;

			case INST :
				// ===================== I N S T =====================
				break;
			case SSND :
				// ===================== S S N D =====================
				try{
					arFp.Read( &aSndDataChunk, sizeof( SoundDataChunk ) );
				}
				catch(...){
					errStr = "Error in reading AIFF sound data chunk.";
					AfxMessageBox(errStr);
					return;
				}
				ssnd_pos = (long)arFp.GetPosition();
				arFp.Seek( (aCommon.numSampleFrames * 2), CFile::current );
				skip = 1;
				break;

			case MARK :
				// ===================== M A R K =====================
				try{
					arFp.Read( &marks, sizeof( short ) );
					marks = sEndian( marks );

					for( i = 0; i < marks; i++ ) {
						arFp.Read( &aMarkRv.id, sizeof( short ) );
						arFp.Read( &aMarkRv.position, sizeof( long ) );
						arFp.Read( &aMarkRv.size, sizeof( char ) );

						fcount = 0xff & aMarkRv.size;
						arFp.Read( name,(UINT)(sizeof(char) * fcount));
						if( (aMarkRv.size % 2) == 0 ) {
							// for padding
							arFp.Read( &c, sizeof( char ) );
						}
						name[aMarkRv.size] = 0;
						if( i < 2 ) {
							aMark[i].id = sEndian( aMarkRv.id );
							aMark[i].position = lEndian( aMarkRv.position );
							aMark[i].size = aMarkRv.size;
						}
					}

					if( marks > 2 ){
						CString csMsg = "%d marks detected, but first two will be used for the looping.";
						errStr.Format(csMsg, marks);
						AfxMessageBox( errStr );
					}
					skip = 1;
				}
				catch(...){
					errStr = "Error in reading AIFF Common chunk.";
					AfxMessageBox(errStr);
					return;
				}
				break;

			default :
				// ===================== default =====================
				break;
		}

		if( skip == 0 ) {
			try{
				arFp.Seek( total, CFile::begin );
			}
			catch(...){
				errStr = "Error in seeking AIFF file.";
				AfxMessageBox(errStr);
				return;
			}
		}
	}

	loopflg = 0;
	if ( marks > 0 ){
		loopflg = 1;
	}
	else if( loopon == 1 ) {
		loopflg = 1;
		aMark[0].position = 0;
		aMark[1].position = aCommon.numSampleFrames;
	}

	if( loopon == 2 ) {
		loopflg = 0;
	}

	vagFile.Write( &aVh, sizeof( VagHeader ) );
	arFp.Seek( ssnd_pos, CFile::begin );

	try{
		Encode( arFp, aCommon.numSampleFrames, loopflg, aMark[0].position, aMark[1].position );
	}
	catch(CString){
		AfxMessageBox(errStr);
		return;
	}

	memset(&aVh,0,sizeof(aVh));

	// "VAGp"
	aVh.id = lEndian( 0x56414770 );
	aVh.version = lEndian( 0x00020001UL );	// HE-VAG (>0x00020001)
	aVh.datasize = lEndian( ((unsigned long)vagFile.GetPosition() - sizeof(VagHeader)) );
	aVh.fs = lEndian( (long)sampleRate );
	int bufSize = sizeof(aVh.name);
	int nameLen = noExName.GetLength();
	memcpy_s(
		aVh.name,
		bufSize,
		noExName,
		(nameLen > bufSize) ? bufSize : nameLen );

	// write VAG header
	vagFile.Seek( 0, CFile::begin );
	vagFile.Write( &aVh, sizeof( VagHeader ) );
}

void Ca2vDlg::readChunkHead( CFile& arFp, ChunkHeader *aChunk )
{
	ChunkHeader tmp;

	arFp.Read( &tmp, sizeof( ChunkHeader ) );
	aChunk->ckID   = lEndian( tmp.ckID );
	aChunk->ckSize = lEndian( tmp.ckSize );
}

void Ca2vDlg::readChunkAIFFHead( CFile& arFp, ChunkAIFFHeader *aChunk )
{
	ChunkAIFFHeader tmp;

	arFp.Read( &tmp, sizeof( ChunkAIFFHeader ) );
	aChunk->ckID   = lEndian( tmp.ckID );
	aChunk->ckSize = lEndian( tmp.ckSize );
	aChunk->ckType = lEndian( tmp.ckType );
}

void Ca2vDlg::Encode( CFile& arFp, unsigned long frame, short loopflg, unsigned long loopstart, unsigned long loopend )
{
	unsigned short fcount;
	short i;
	long rest = frame;		// remain samples
	unsigned long bsframe = 0;
	short blockRv[SCE_VAG_BLOCKSIZE], block[SCE_VAG_BLOCKSIZE];
	short outbuf[8];
	short flag;
	CString str = "";

	// initialize
	for (i=0 ;i<8 ;i++){
		outbuf[i] = 0;
	}

	try{
		vagFile.Write( outbuf, sizeof( short )* 8 );
	}
	catch(...){
		throw errStr = "Write error";
		return;
	}

	sceVagConvertInit( SCE_VAG_CONVMODE_STD );

	do {
		// VAG convert routine for each 28 samples
		if ( rest <= SCE_VAG_BLOCKSIZE ){
			// for last brock
			for( i = 0; i < SCE_VAG_BLOCKSIZE; i++ ) {
				blockRv[i] = 0;
			}
			fcount = (short)rest;
			try{
				arFp.Read( blockRv, sizeof( short )*fcount );
			}
			catch(...){
				throw errStr = "Error in reading last block of sound data chunk.";
				return;
			}
		}
		else {
			fcount = SCE_VAG_BLOCKSIZE;
			try{
				arFp.Read( blockRv, sizeof( short )*SCE_VAG_BLOCKSIZE );
			}
			catch(...){
				throw errStr = "Error in reading AIFF sound data chunk.";
				return;
			}
		}

		// Invert the endian
		for( i = 0; i < SCE_VAG_BLOCKSIZE; i++ ) {
			block[i] = sEndian( blockRv[i] );
		}

		if (loopflg == 1){
			flag = 0;
			if ((bsframe <= loopstart) && (loopstart < (bsframe + SCE_VAG_BLOCKSIZE))){
				flag = SCE_VAG_BLKATR_LOOPSTART;
			}
			else {
				flag = SCE_VAG_BLKATR_LOOPBODY;
			}
			if ((bsframe <= (loopend - 1)) && ((loopend - 1) < (bsframe + SCE_VAG_BLOCKSIZE))){
				flag = SCE_VAG_BLKATR_LOOPEND;
				rest = 0;
			}
		}
		else {
			if (rest <= SCE_VAG_BLOCKSIZE){
				flag = SCE_VAG_BLKATR_1SHOTEND;
			}
			else {
				flag = SCE_VAG_BLKATR_1SHOT;
			}
		}
		sceVagConvert( block, outbuf, flag );

		try{
			vagFile.Write( outbuf, sizeof( short )* 8 );
		}
		catch(...){
			throw errStr = "Write error";
			return;
		}
		rest -= SCE_VAG_BLOCKSIZE;
		bsframe += SCE_VAG_BLOCKSIZE;
	} while( rest > 0 );

	if( loopflg == 0 ) {
		sceVagConvertFin( outbuf );
		try{
			vagFile.Write( outbuf, sizeof( short )* 8 );
		}
		catch(...){
			throw errStr = "Write error";
			return;
		}
	}
}
