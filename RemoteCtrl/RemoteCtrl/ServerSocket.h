#pragma once

#include "pch.h"
#include "framework.h"
void Dump(BYTE* pData, size_t nSize);
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
		for (i = 0; i < nSize; i++)
		{
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
		if (sum == sSum)
		{
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
	const char* Data()               //包数据的值
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

class CServerSocket
{
public:
	static CServerSocket* getInstance()  //静态函数全局可以访问，没有this指针，无法直接访问成员变量
	{
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitSocket()
	{
		//SOCKET m_sock = socket(PF_INET, SOCK_STREAM, 0);         // 协议族，type流tcp，放到构造函数
		if (m_sock == -1)
		{
			return false;
		}
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr)); //初始化，清零
		serv_adr.sin_family = AF_INET;          //地址族
		serv_adr.sin_addr.s_addr = INADDR_ANY;  //监听所有ip
		serv_adr.sin_port = htons(9527);        //端口
		//绑定
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		{
			return false;
		}
		if (listen(m_sock, 1) == -1)                    //因为控制设计1v1
		{
			return false;
		}
		return true;
	}

	bool AcceptClient()
	{
		TRACE("enter AcceptClient\n");
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client = %d\n", m_client);
		if (m_client == -1)
		{
			return false;
		}
		return true;
	}

#define BUFFER_SIZE 4096
	int DealCommand()
	{
		if (m_client == -1)
		{
			return -1;
		}
		char* buffer = new char[BUFFER_SIZE];      //收拾单个信息，发可能是多个信息
		if (buffer == NULL)
		{
			TRACE("内存不足！\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len < 0)
			{
				delete[]buffer;
				return -1;
			}
			TRACE("recv %d\n", len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1;
	}
	bool Send(const char* pData, int nSize)
	{
		if (m_client == -1)
		{
			return false;
		}
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack)
	{
		if (m_client == -1)
		{
			return false;
		}
		//Dump((BYTE*)pack.Data(), pack.Size());//调试用，要注释掉
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}
	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4) || (m_packet.sCmd == 9))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse)        //拿到鼠标事件
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}
	CPacket& GetPacket()
	{
		return m_packet;
	}
	void CloseClient()
	{
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}
private:
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;

	CServerSocket& operator=(const CServerSocket& serversocket)  //赋值构造函数
	{}
	CServerSocket(const CServerSocket& serversocket)             //复制构造函数
	{
		m_sock = serversocket.m_sock;
		m_client = serversocket.m_client;
	}
	CServerSocket()
	{
		m_client = INVALID_SOCKET; // = -1
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket()
	{
		closesocket(m_sock);
		WSACleanup();          // 清理网络
	}
	BOOL InitSockEnv()         //初始化socket环境
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)  // 版本低，兼容性好
		{
			return FALSE;
		}
		return TRUE;
	}

	static void releasrInstance()
	{
		if (m_instance != NULL)
		{
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	static CServerSocket* m_instance;

	class  CHelper {
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releasrInstance();
		}
	};
	static CHelper m_helper;
};


