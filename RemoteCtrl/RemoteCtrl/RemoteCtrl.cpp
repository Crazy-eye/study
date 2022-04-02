// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/*
#pragma comment(linker,"/subsytem:windows /entry:WinMainCRTStartuo")
#pragma comment(linker,"/subsytem:windows /entry:mainCRTStartuo")
#pragma comment(linker,"/subsytem:console /entry:mainCRTStartuo")
#pragma comment(linker,"/subsytem:console /entry:WinMainCRTStartuo")
*/



// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
void Dump(BYTE* pData, size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0))
        {
            strOut += "\n";
        }
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF); //避免负数影响
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo()  //检查磁盘分区1==A;2==B;3==C...26==Z
{
    std::string result;
    for (int i = 1; i <= 26; i++)  //改变当前驱动，能成功就存在
    {
        if (_chdrive(i) == 0)
        {
            if (result.size() > 0)
            {
                result += ',';
            }
            result += 'A' + i - 1;
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size()); //打包
    Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <io.h>            //文件查找
#include <list>


int MakeDirectoryInfo()     //查看指定目录下的文件
{
    std::string strPath;
    //std::list<FILEINFO> lstFileInfos;  //放到一个list里
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false)
    {
        OutputDebugString(_T("当前命令，不是获取文件列表，命令解析错误！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0)     //切换磁盘目录
    {
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        //lstFileInfos.push_back(finfo);     //list用
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限访问目录！"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1)
    {

        OutputDebugString(_T("没有找到任何文件！"));
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        return -3;
    }
    int count = 0;
    do
    {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;      //判断是否为目录
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        TRACE(" %s\n", finfo.szFileName);
        //lstFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        count++;
    } while (!_findnext(hfind, &fdata));
    TRACE("server:count=%d\n", count);

    //发送结束信息到控制端
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstance()->Send(pack);

    return 0;
}

int RunFile()    //打开文件
{
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);     //多字节字符集用A

    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);

    return 0;

}

//#pragma warning(disable:4996)    //可以使用fopen sprintf strcpy strstr
int DownloadFile()         //下载文件
{
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");            //文本 二进制都可以用二进制读
    if (err != 0)
    {
        CPacket pack(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != NULL)
    {
        fseek(pFile, 0, SEEK_END);        //读写位置移动到文件尾
        data = _ftelli64(pFile);          //距文件首偏移
        CPacket head(4, (BYTE*)&data, 8); //发送文件大小的包
        CServerSocket::getInstance()->Send(head);
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = "";
        size_t rlen = 0;
        do                                //发送文件
        {
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024);
        fclose(pFile);
    }
    //防止意外终止
    CPacket pack(4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int MouseEvent()      //鼠标操作
{
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse))
    {
        DWORD nFlage = 0;//标志 低四位1 2 4 8 用于button 16 32 64 128用于action

        switch (mouse.nButton)
        {
        case 0://左键
            nFlage = 1;
            break;
        case 1://右键
            nFlage = 2;
            break;
        case 2://中键
            nFlage = 4;
            break;
        case 4://没有按键
            nFlage = 8;
            break;
        default:
            break;
        }
        if (nFlage != 8)
        {
            SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);   //把光标移到屏幕的指定位置
        }
        switch (mouse.nAction)
        {
        case 0://单击
            nFlage |= 0x10;
            break;
        case 1://双击
            nFlage |= 0x20;
            break;
        case 2://按下
            nFlage |= 0x40;
            break;
        case 3://放开
            nFlage |= 0x80;
            break;
        default:
            break;
        }
        switch (nFlage)
        {
        case 0x21://左键双击 
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo()); //GetMessageExtraInfo  系统API获取当前线程中额外的信息包括键盘鼠标的信息
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12://右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x24://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14://中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x08://鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        default:
            break;
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else
    {
        OutputDebugString(_T("获取鼠标操作参数失败！！"));
        return -1;
    }
    return 0;
}

int SendScreen()
{
    CImage screen; //封装了图形的操作，方便GDI（全局设备接口：显示器）编程
    HDC hScreen = ::GetDC(NULL); //DC： 设备上下文-》显示器和所有的设置
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);  //位宽（一个点有多少比特）  GetDeviceCaps获取设备多个属性  RGB:8B8B8B ARGB:8B8B8B8B 等 
    int nWidth = GetDeviceCaps(hScreen, HORZRES);
    int nHeight = GetDeviceCaps(hScreen, VERTRES);
    screen.Create(nWidth, nHeight, nBitPerPixel);
    BitBlt(screen.GetDC(), 0, 0, 1920, 1080, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);   //分配可移动的堆内存
    if (hMem == NULL)
    {
        return -1;
    }
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);    //在全局对象上创建一个 stream object。 This object is the OLE-provided implementation of the IStream interface
    if (ret == S_OK)
    {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);
        PBYTE pData = (PBYTE)GlobalLock(hMem);
        SIZE_T nSize = GlobalSize(hMem);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);
    }

    //DWORD tick = GetTickCount64();      //测试png，jpg延迟
    //screen.Save(_T("test2022.png"), Gdiplus::ImageFormatPNG);
    //TRACE("png:%d\n", GetTickCount64() - tick);
    //tick = GetTickCount64();
    //screen.Save(_T("test2022.jpg"), Gdiplus::ImageFormatJPEG);
    //TRACE("jpg:%d\n", GetTickCount64() - tick);
    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}


#include "LockInfoDialog.h"
CLockInfoDialog dlg;
unsigned threadid = 0;

unsigned __stdcall threadLockDlg(void* arg)    //子线程  防止在消息循环卡死
{
    TRACE("%s(%d):%d\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
    dlg.Create(IDD_DIALOG_INFO, NULL);//没有父窗口，非模态
    dlg.ShowWindow(SW_SHOW);
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN); //获取系统参数(x坐标) 1920
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);//（本PC测试）1057
    rect.bottom = LONG(rect.bottom * 1.03);                            //覆盖全屏
    TRACE("right=%d bottom=%d \n", rect.right, rect.bottom);
    dlg.MoveWindow(rect);
    //置（z轴）顶窗口
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//不改变大小，不移动

    ShowCursor(false);                //限制鼠标功能，鼠标隐藏
    ::ShowWindow(::FindWindow(_T("Shell-TrayWnd"), NULL), SW_HIDE);    //windows的任务栏隐藏
    dlg.GetWindowRect(rect);          //拿到窗口范围
    rect.left = 0;
    rect.right = 0;
    rect.right = 1;
    rect.bottom = 1;       //限制在一个像素点（左上角）
    ClipCursor(rect);                 //鼠标限制活动范围
    //MFC,windows编程都基于消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))   //没有窗口
    {

        TranslateMessage(&msg);            //传递消息
        DispatchMessage(&msg);             //分发
        if (msg.message == WM_KEYDOWN)
        {
            TRACE("msg:%08X wparam:%08X lparam:%08X\n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == 0x1B)     //按下esc退出
            {
                break;
            }
        }
    }
    ShowCursor(true);
    ::ShowWindow(::FindWindow(_T("Shell-TrayWnd"), NULL), SW_SHOW);
    dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}
int LockMachine()   //锁定
{
    //不是空也不无效时防止多次创建子进程
    if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE)
    {
        //_beginthread(threadLockDlg, 0, NULL);
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid=%d\n", threadid);
    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}
int   UnlockMachine()  //解锁
{
    //dlg.SendMessage(WM_KEYDOWN, 0x1b, 0x00010001);                //失败
    //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x1b, 0x00010001);      //全局    失败 线程只接受在线程的信息（不根据对话框和窗口句柄，根据线程）
    PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);               //向指定线程发信息
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int TestConnect()     //测试包
{
    CPacket pack(1981, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret = %d\n", ret);
    return 0;

}

int DeleteLocalFile()     // 删除文件
{
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    TCHAR sPath[MAX_PATH] = _T("");
    //mbstowcs(sPath, strPath.c_str(), strPath.size());   //多字节字符集转宽字节        中文乱码了
    MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
    DeleteFileA(strPath.c_str());
    CPacket pack(9, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret = %d\n", ret);
    return 0;
}

int ExcuteCommand(int nCmd)
{
    int ret = 0;
    switch (nCmd)
    {
    case 1: //查看磁盘分区
        ret = MakeDriverInfo();
        break;
    case 2://查看指定目录下的文件
        ret = MakeDirectoryInfo();
        break;
    case 3://打开文件
        ret = RunFile();
        break;
    case 4://下载文件
        ret = DownloadFile();
        break;
    case 5://鼠标操作
        ret = MouseEvent();
        break;
    case 6://发送屏幕内容==发送屏幕的截图
        ret = SendScreen();
        break;
    case 7://锁机  开线程
        ret = LockMachine();
        //Sleep(80);
        //LockMachine();
        break;
    case 8://解锁
        ret = UnlockMachine();
        break;
    case 9://删除文件
        ret = DeleteLocalFile();
        break;
    case 1981:
        ret = TestConnect();
        break;
    default:
        break;
    }
    //Sleep(5000);
    //UnlockMachine();
    //while (dlg.m_hWnd != NULL)
    //{
    //    Sleep(10);
    //}
    //Sleep(5000);
    return ret;
}


int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
             // socket bind listen accept read(recv) write(send) close
            // 套接字初始化
            CServerSocket* pserver = CServerSocket::getInstance();       //单例 从语法先知而不是规范  拿到这个实例
            int count = 0;
            if (pserver->InitSocket() == false)
            {
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while (CServerSocket::getInstance() != NULL)      //这个实例存在就一直循环
            {
                if (pserver->AcceptClient() == false)
                {
                    if (count >= 3)
                    {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("用户接入失败"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("用户接入失败"), MB_OK | MB_ICONERROR);
                    count++;
                }
                TRACE("AcceptClient return true\n");
                int ret = pserver->DealCommand();
                TRACE("DealCommand ret %d\n", ret);
                if (ret > 0)     //成功就处理
                {
                    ret = ExcuteCommand(ret);
                    if (ret != 0)
                    {
                        TRACE("执行命令失败：%d ret=%d\n", pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();          //短连接方式
                    TRACE("Command has done!\n");
                }
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}

