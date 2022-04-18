﻿// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "WatchDialog.h"
#include "afxdialogex.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_nObjWidth = -1;
	m_nObjHeight = -1;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	//客户端800/450
	CRect clientRect;
	if (isScreen)
	{
		ScreenToClient(&point);     //全局坐标到客户区域坐标
	}
	TRACE("x=%d y=%d\r\n", point.x, point.y);
	m_picture.GetWindowRect(clientRect); 
	TRACE("Width=%d Height=%d\r\n", clientRect.Width(), clientRect.Height());

	//本地坐标到远程坐标
	int width0 = clientRect.Width();
	int height0 = clientRect.Height();
	int width = m_nObjWidth, height = m_nObjHeight;//被控端屏幕
	//int width = 1920, height = 1080;    //被控端屏幕
	int x = point.x * width / width0;
	int y = point.y * height / height0;
	return CPoint(x, y);
	/* 精简前6行
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
    */
	}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 50, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0)
	{
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull()) //有数据才显示
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);    //画面不全
			if (m_nObjWidth == -1)
			{
				m_nObjWidth = pParent->GetImage().GetWidth();
			}
			if (m_nObjHeight == -1)
			{
				m_nObjHeight = pParent->GetImage().GetHeight();
			}
			pParent->GetImage().StretchBlt(        //缩放的方式显示全部画面
				m_picture.GetDC()->GetSafeHdc(), 0, 0,
				rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL); //重绘，显示出来
			pParent->GetImage().Destroy();
			pParent->SetImageStatus();
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)//左键双击  拿到的就是客户端坐标
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight) != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);//坐标转换
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 1;//双击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}

	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)//左键按下  拿到的就是客户端坐标
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight) != -1)
	{
		TRACE("原始x=%d y=%d\r\n", point.x, point.y);
		CPoint remote = UserPoint2RemoteScreenPoint(point);//坐标转换
		TRACE("校对x=%d y=%d\r\n", point.x, point.y);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//按下
		/*
		CClientSocket* pClient = CClientSocket::getInstance();
		CPacket pack(5, (BYTE*)&event, sizeof(event));
		pClient->Send(pack);
		有bug换用下两行代码
		*/
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}

	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)//左键弹起  拿到的就是客户端坐标
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight) != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);//坐标转换
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 3;//弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)//右键双击  拿到的就是客户端坐标
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight) != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);//坐标转换
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 1;//双击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}

	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)//右键按下  拿到的就是客户端坐标
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight) != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);//坐标转换
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 2;//按下
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}

	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)//右键弹起  拿到的就是客户端坐标
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight) != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);//坐标转换
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 3;//弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}

	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)//鼠标移动  拿到的就是客户端坐标
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight) != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);//坐标转换
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8;//没有按键
		event.nAction = 0;//移动
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();   //todo：设计缺陷、网络通信和对话框有耦合
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}

	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch() //左键单击
{
	// TODO: 在此添加控件通知处理程序代码
	if ((m_nObjWidth != -1) && (m_nObjHeight) != -1)
	{
		CPoint point;
		GetCursorPos(&point);        //拿到的scree屏幕坐标
		CPoint remote = UserPoint2RemoteScreenPoint(point, true);//坐标转换
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 0;//单击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}

}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类
	//屏蔽OnOK
	//CDialog::OnOK();
}
