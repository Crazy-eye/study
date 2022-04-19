#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
////#include <list>
////#include <map>
////#include <mutex>
////#define WM_SEND_PACK (WM_USER+1) //发送包数据
////#define WM_SEND_PACK_ACK (WM_USER+2) //发送包数据应答
#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)    //打包
	{         
		sHead = 0xFEFF;
		nLength = nSize + 2 + 2;
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else
		{
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& pack)                //复制构造函数
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize)     //解包
	{
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;                    //防止只有包头的包被通过
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize)     //包数据可能不全，或包头未能全部接收到
		{
			nSize = 0;                 //用掉0个字节
			return;
		}
		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize)      //包没完全接收到，不需要解析
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			TRACE("%s\r\n", strData.c_str() + 12);     //有12字节无效file_Info
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i);
		i += 2;

		//检验和
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;//head2 length4 data...
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack)    //运算符重载
	{
		if (this != &pack)
		{
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;                            //可以连等a=b=c=d=e=f
	}
	int Size()                       //包数据大小
	{
		return nLength + 2 + 4;      //包头2长度4  head+nlength
	}
	////const char* Data(std::string& strOut) const               //包数据的值
	const char* Data()              //包数据的值
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)(pData) = nLength;
		pData += 4;
		*(WORD*)pData = sCmd;
		pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	WORD sHead;    //包头 固定位：FE FF             2字节
	DWORD nLength; //包长度(从命令开始到校验结束)   4字节
	WORD sCmd;     //包命令                         2字节
	std::string strData;  //包数据                  n字节
	WORD sSum;     //和校验  只算strData            2字节
	std::string strOut;//整个包的数据
};
#pragma pack(pop)

typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;      //动作：点击、移动、双击
	WORD nButton;      //左键0、右键1、中键2
	POINT ptXY;        //坐标
}MOUSEEV, * PMOUSEEV;

typedef struct file_info
{
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;        //是否无效
	BOOL IsDirectory;      //是否为目录 1是 0否
	BOOL HasNext;          //是否有后续 1有 0无
	char szFileName[256];  //文件名 目录名
}FILEINFO, * PFILEINFO;


//enum
//{
//	CSM_AUTOCLOSE = 1,//CSM = Client Socket Mode 自动关闭模式
//};

//typedef struct PacketData {
//	std::string strData;
//	UINT nMode;
//	WPARAM wParam;
//	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) {
//		strData.resize(nLen);
//		memcpy((char*)strData.c_str(), pData, nLen);
//		nMode = mode;
//		wParam = nParam;
//	}
//	PacketData(const PacketData& data) {
//		strData = data.strData;
//		nMode = data.nMode;
//		wParam = data.wParam;
//	}
//	PacketData& operator=(const PacketData& data) {
//		if (this != &data) {
//			strData = data.strData;
//			nMode = data.nMode;
//			wParam = data.wParam;
//		}
//		return *this;
//	}
//}PACKET_DATA;

std::string GetErrInfo(int wsaErrCode);
void Dump(BYTE* pData, size_t nSize);
class CClientSocket
{
public:
	static CClientSocket* getInstance()  //静态函数全局可以访问，没有this指针，无法直接访问成员变量
	{
		if (m_instance == NULL)
		{
			m_instance = new CClientSocket();
			TRACE("CClientSocket size is %d\r\n", sizeof(*m_instance));  	//代码校验
		}
		return m_instance;
	}
	bool InitSocket(int nIP,int nPort)          //客户端要填服务端ip
	{
		//SOCKET m_sock = socket(PF_INET, SOCK_STREAM, 0);         // 协议族，type流tcp，放到构造函数
		if (m_sock != INVALID_SOCKET)
		{
			CloseSocket();
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1)
		{
			return false;
		}
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr)); //初始化，清零
		serv_adr.sin_family = AF_INET;          //地址族
		serv_adr.sin_addr.s_addr = htonl(nIP);  //服务器ip     把host主机字节序转成网络字节序
		serv_adr.sin_port = htons(nPort);       //端口
		if (serv_adr.sin_addr.s_addr == INADDR_NONE) //保证connect不出问题
		{
			AfxMessageBox("指定IP地址不存在！！！");
			return false;
		}
		int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1)
		{
			AfxMessageBox("链接失败");           //MFC可用  ，用多字节字符集
			//TRACE("连接失败：%d %s\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;
	}


#define BUFFER_SIZE 2048000
	int DealCommand()
	{
		if (m_sock == -1)
		{
			return -1;
		}
		char* buffer = m_buffer.data();    //可能会收很多数据
		static size_t index = 0;
		while (true)
		{
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if ((len <= 0) && (index <= 0)) //缓存没数据，读到的也没数据
			{
				return -1;
			}
			//Dump((BYTE*)buffer, index);
			//TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			index += len;
			len = index;
			//TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			m_packet = CPacket((BYTE*)buffer, len);
			//TRACE("command %d\r\n", m_packet.sCmd);
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true);
	////bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam = 0);
	
	bool Send(const char* pData, int nSize)
	{
		if (m_sock == -1)
		{
			return false;
		}
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack)
	{
		if (m_sock == -1)
		{
			return false;
		}
		////Dump((BYTE*)pack.Data(), pack.Size());//调试用，要注释掉
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse)        //拿到鼠标事件
	{
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}
	CPacket& GetPacket()
	{
		return m_packet;
	}
	void CloseSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;  //无效的套接字 -1
	}
	////void UpdateAddress(int nIP, int nPort) {
	////	if ((m_nIP != nIP) || (m_nPort != nPort)) {
	////		m_nIP = nIP;
	////		m_nPort = nPort;
	////	}
	////}
private:
	////HANDLE m_eventInvoke;//启动事件
	////UINT m_nThreadID;
	////typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	////std::map<UINT, MSGFUNC> m_mapFunc;
	////HANDLE m_hThread;
	////bool m_bAutoClose;
	////std::mutex m_lock;
	////std::list<CPacket> m_lstSend;
	////std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	////std::map<HANDLE, bool> m_mapAutoClosed;
	//int m_nIP; //地址
	//int m_nPort;//端口
	std::vector<char> m_buffer;         //内存不需要管理
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& serversocket)  //赋值构造函数
	{}
	CClientSocket(const CClientSocket& serversocket)             //复制构造函数
	{
		m_sock = serversocket.m_sock;
	}
	CClientSocket()
	{
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	~CClientSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();          // 清理网络
	}
	////static unsigned __stdcall threadEntry(void* arg);
	//void threadFunc();
	////void threadFunc2();
	BOOL InitSockEnv()         //初始化socket环境
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)  // 版本低，兼容性好
		{
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance()
	{
		TRACE("CClientSocket has been called!\r\n");
		if (m_instance != NULL)
		{
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
			TRACE("CClientSocket has released!\r\n");
		}
	}
	////bool Send(const char* pData, int nSize) {
	////	if (m_sock == -1)return false;
	////	return send(m_sock, pData, nSize, 0) > 0;
	////}
	////bool Send(const CPacket& pack);
	////void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);
	////
	static CClientSocket* m_instance;
	
	class CHelper {
	public:
		CHelper()
		{
			CClientSocket::getInstance();
		}
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

