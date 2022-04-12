
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "ClientSocket.h"
#include "WatchDialog.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET,&CRemoteClientDlg::OnSendPacket)         //添加到消息映射表里，消息函数  ③注册消息
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();       //默认为ture，把控件的值赋给成员变量
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));
	if (!ret)
	{
		AfxMessageBox("网络初始化失败！");
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("Send ret %d\n", ret);
	int cmd = pClient->DealCommand();
	TRACE("ack:%d\n", cmd);
	if (bAutoClose)
	{
		pClient->CloseSocket();
	}
	return cmd;
}

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();     //默认为ture，把控件的值赋给成员变量
	m_server_address = 0x7F000001;
	m_nPort = _T("9527");
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = false;

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteClientDlg::OnBnClickedBtnTest()
{
	// TODO: 在此添加控件通知处理程序代码
	SendCommandPacket(1981);

}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	//std::list<CPacket> lstPackets;
	//int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0);
	int ret = SendCommandPacket(1);
	if (ret == -1)
	{
		AfxMessageBox(_T("命令处理失败!!!"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();    //清空树，防止越来越高
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',')
		{
			dr += ":";
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}

}


void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	Sleep(50);      //现在在对话框之后启动
	CClientSocket* pClient = NULL;
	do
	{
		pClient = CClientSocket::getInstance();
	} while (pClient == NULL);
	for (;;)
	{
		if (m_isFull == false)//更新数据到缓存
		{
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);
			if (ret == 6)
			{
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0); //申请内存
				if (hMem == NULL)
				{
					TRACE("内存不足！");
					Sleep(1);     //防止死循环
					continue;
				}
				IStream* pStream = NULL;                      //创建一个流
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
				if (hRet == S_OK) //判断内存不足
				{
					ULONG length = 0;
					pStream->Write(pData, pClient->GetPacket().strData.size(), &length);
					LARGE_INTEGER bg = { 0 };
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);  //跳转到流开头
					m_image.Load(pStream);                     //load从指定文件加载图像
					m_isFull = true;
				}
			}
			else
			{
				Sleep(1);   //当网络拿到了建立连接，网络断掉，send会瞬间返回-1，把cpu资源耗尽卡死
			}
		}
		else
		{
			Sleep(1);;
		}
		
	}
}

void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}

void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = m_List.GetSelectionMark();               //获取选择的标记
	CString strFile = m_List.GetItemText(nListSelected, 0);      //第零个就是第一个数据就是文件名字
	CFileDialog dlg(FALSE, NULL,               //保存文件，扩展名
		strFile,                              //文件名
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, //隐藏只读，防止重名
		NULL, this);                           //过滤器
	if (dlg.DoModal() == IDOK)                //模态对话框
	{
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");      //本地二进制创建的方式
		if (pFile == NULL)
		{
			AfxMessageBox(_T("没有权限保存该文件，或者文件无法创建！！！"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		HTREEITEM hSelected = m_Tree.GetSelectedItem();           //获取选中的
		strFile = GetPath(hSelected) + strFile;
		TRACE("%s\n", LPCSTR(strFile));
		CClientSocket* pClient = CClientSocket::getInstance();
		do {
			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCTSTR)strFile);
			if (ret < 0)
			{
				AfxMessageBox(_T("执行下载命令失败！"));
				TRACE("下载失败 ret = %d\r\n", ret);
				break;;
			}
			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			if (nLength == 0)
			{
				AfxMessageBox("文件长度为0或无法读取文件！！！");
				break;;
			}
			long long nCount = 0;
			while ((nCount < nLength))
			{
				ret = pClient->DealCommand();
				if (ret < 0)
				{
					AfxMessageBox("传输失败！");
					TRACE("传输失败 ret = %d\r\n", ret);
					break;
				}
				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
				nCount += pClient->GetPacket().strData.size();
			}
		} while (false);
		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成！！"), _T("完成"));
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();

	while (pInfo->HasNext)       //只要名字为空，hasnext为false
	{
		TRACE(" [%s] isdirectory %d\r\n", pInfo->szFileName, pInfo->IsDirectory);

		if (!pInfo->IsDirectory)      //只更新文件夹
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		//TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)
		{
			break;
		}
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();

}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);         //拿到鼠标位置 全局坐标，相对屏幕左上角（0，0）
	m_Tree.ScreenToClient(&ptMouse);//可以检测树控件的点击
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
	{
		return;
	}
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)        //文件不获取子目录
	{
		return;
	}
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	int Count = 0;
	while (pInfo->HasNext)       //只要名字为空，hasnext为false
	{
		TRACE(" [%s] isdirectory %d\r\n", pInfo->szFileName, pInfo->IsDirectory);

		if (pInfo->IsDirectory)
		{
			if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == ".."))  //这两个特殊目录排除防止死循环
			{
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0)
				{
					break;
				}
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();

				continue;
			}
			//TRACE("hselected %08X %08X\r\n", hParent, m_Tree.GetSelectedItem());
			//m_Tree.Expand(hParent, TVE_EXPAND);
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		Count++;
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)
		{
			break;
		}
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
	TRACE("Count = %d\n", Count);

}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);       //获取子节点
		if (hSub != NULL)
		{
			m_Tree.DeleteItem(hSub);
		}
	} while (hSub != NULL);

}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	//右键单击
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList); //屏幕坐标转换客户端坐标
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0)     //没点中
	{
		return;
	}
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);//左对齐，跟踪右键
	}
}


void CRemoteClientDlg::OnDownloadFile()  // 下载文件
{
	// TODO: 在此添加命令处理程序代码
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	BeginWaitCursor();         //设置光标等待状态
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);//对话框居中
	m_dlgStatus.SetActiveWindow(); //激活，掷到前台
}


void CRemoteClientDlg::OnDeleteFile()  // 删除文件
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);              //拿到路径
	int nSelected = m_List.GetSelectionMark();         //拿到列表选中的
	CString strFile = m_List.GetItemText(nSelected, 0);//拿到文件名
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0)
	{
		AfxMessageBox(_T("删除文件命令执行失败！"));
	}
	LoadFileCurrent();
}


void CRemoteClientDlg::OnRunFile()  // 打开文件
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);              //拿到路径
	int nSelected = m_List.GetSelectionMark();         //拿到列表选中的
	CString strFile = m_List.GetItemText(nSelected, 0);//拿到文件名
	strFile = strPath + strFile;
	int ret = SendCommandPacket(3, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0)
	{
		AfxMessageBox(_T("打开文件命令执行失败！"));
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)       //④实现消息相应函数
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd)
	{
	case 4:
	{
		CString strFile = (LPCSTR)lParam;
		ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	}
	break;
	case 6:
	{
		ret = SendCommandPacket(cmd, wParam & 1);
	}
	break;
	default:
		ret = -1;
		break;
	}
	return ret;
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	// TODO: 在此添加控件通知处理程序代码
	CWatchDialog dlg(this);
	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	//GetDlgItem(IDC_BTN_START_WATCH)->EnableWindow(FALSE); //防止启用多个线程，点击一次后禁用按钮
	dlg.DoModal();     //模态就防止了多次点击按钮
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}
