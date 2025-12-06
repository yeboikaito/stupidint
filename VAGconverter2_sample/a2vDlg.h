/* SIE CONFIDENTIAL
 * VAG Converter 2 VAG Converter 2 version 2.5.0.3
 * Copyright(C) 2016 Sony Interactive Entertainment Inc.
 */

#pragma once

struct ChunkHeader;
struct ChunkAIFFHeader;

class Ca2vDlg : public CDialog
{
// Construction
public:
	Ca2vDlg(CWnd* pParent = NULL);
	
// Dialog Data
	enum { IDD = IDD_A2V_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	CString m_editSrc;
	CString m_editDst;
	CButton m_button_cnv;
	afx_msg void OnBnClickedButtonSrc();
	afx_msg void OnBnClickedButtonDst();
	afx_msg void OnBnClickedButtonCnv();

	CFile vagFile;
	CFile arFp;
	CFileException vagFileError;
	CString errStr;
	CString srcPath;
	CString dstPath;
	CString noExName;

	float sane2ld(unsigned char *sane);
	void Convert(short loopon);
	void readChunkHead(CFile& arFp, ChunkHeader *aChunk);
	void readChunkAIFFHead(CFile& arFp, ChunkAIFFHeader *aChunk);
	void Encode(CFile& arFp, unsigned long frame, short loopflg, unsigned long loopstart, unsigned long loopend);
};
