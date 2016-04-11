////////////////////////////////////////////////////////
//
//文件说明：udp源文件
//
////////////////////////////////////////////////////////
#include <cassert>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <cstdlib>
#include <time.h>

#ifdef WIN32

#include <winsock2.h>
#include <stdlib.h>
#include <io.h>

#else

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>

#endif

#include <string.h>

#include "udp.h"

using namespace std;
//===========================================
//openPort函数说明
//函数功能：打开指定的ip和端口准备接收消息
//参数说明：port：指定的端口
//          interfaceIp：指定的ip
//函数返回：Socket：如果返回值为INVALID_SOCKET说明连接失败，反之则连接成功
//===========================================
Socket openPort( unsigned short port, unsigned int interfaceIp)
{
   Socket fd;
   //创建一个套接口
   fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if ( fd == INVALID_SOCKET )
   {
      int err = getErrno();
      cerr << "Could not create a UDP socket:" << err << endl;
      return INVALID_SOCKET;
   }
    
   struct sockaddr_in addr;//IPv4 Socket 地址
   memset((char*) &(addr),0, sizeof((addr)));
   addr.sin_family = AF_INET;// 主要是用于互联网地址
   //htonl将一个32位数由主机字节顺序转换为网络字节顺序
   addr.sin_addr.s_addr = htonl(INADDR_ANY);//INADDR_ANY 0
   addr.sin_port = htons(port);
    
   if ( (interfaceIp != 0) && ( interfaceIp != 0x100007f ) )//interfaceIp不为0也不为127.0.0.1
   {
      addr.sin_addr.s_addr = htonl(interfaceIp);//

#ifdef WIN32 //
         clog << "Binding to interface " << hex << "0x" << htonl(interfaceIp) << dec << endl;
#endif

   }
	//将一个地址与一套接口捆绑
   if ( bind( fd,(struct sockaddr*)&addr, sizeof(addr)) != 0 ) //bind函数成功返回0，失败返回-1
   {
      int e = getErrno();//获得错误类型
        
      switch (e)
      {
         case 0:
         {
            cerr << "Could not bind socket" << endl;
            return INVALID_SOCKET;
         }
         case EADDRINUSE:
         {
            cerr << "Port " << port << " for receiving UDP is in use" << endl;
            return INVALID_SOCKET;
         }
         break;
         case EADDRNOTAVAIL:
         {

#ifdef WIN32 //             
               cerr << "Cannot assign requested address" << endl;          
#endif
            return INVALID_SOCKET;
         }
         break;
         default:
         {
            cerr << "Could not bind UDP receive port"
                 << "Error=" << e << " " << strerror(e) << endl;
            return INVALID_SOCKET;
         }
         break;
      }//end switch
   }//end if

#ifdef WIN32 // 
//      clog << "Opened port " << port << " with fd " << fd << endl;
#endif
   assert( fd != INVALID_SOCKET  );
   return fd;
}

//===========================================
//getMessage函数说明
//函数功能：从源主机和端口接收消息
//参数说明：fd：连接成功的Socket
//          buf：接收到的消息
//          len：接收到的消息的长度
//          srcIp：源主机ip
//          srcPort：源主机端口
//函数返回：true：消息接收成功
//          false：消息接收失败
//===========================================
bool getMessage( Socket fd, char* buf, int* len,unsigned int* srcIp, unsigned short* srcPort)
{
   assert( fd != INVALID_SOCKET );//fd不能为空
	
   int originalSize = *len;
   assert( originalSize > 0 );//消息长度大于0
   
   struct sockaddr_in from;  //发送消息的IPv4 Socket地址
   int fromLen = sizeof(from);
   *len = recvfrom(fd,
                   buf,
                   originalSize,
                   0,
                   (struct sockaddr *)&from,
                   (socklen_t*)&fromLen);//经socket接收数据,成功则返回接收到的字符数，失败则返回-1
	
   if ( *len == SOCKET_ERROR )//
   {
      int err = getErrno();
		
      switch (err)
      {
         case ENOTSOCK:
            //cerr << "Error fd not a socket" <<   endl;
            break;
         case ECONNRESET:
            //cerr << "Error connection reset - host not reachable" <<   endl;
            break;
		 case ENETDOWN:
			 //cerr << "Error " <<endl;
			 break;
         default:
            //cerr << "Socket Error=" << err << endl;
			 break;
      }//end switch
		
      return false;
  
   }// end if
	
   if ( *len < 0 )
   {
      //clog << "socket closed? negative len" << endl;
      return false;
   }
    
   if ( *len == 0 )
   {
      //clog << "socket closed? zero len" << endl;
      return false;
   }
    //ntohs将一个16位数由网络字节顺序转换为主机字节顺序
   *srcPort = ntohs(from.sin_port);
   //ntohl将一个32位数由网络字节顺序转换为主机字节顺序
   *srcIp = ntohl(from.sin_addr.s_addr);
	
   if ( *len > originalSize )//接收的字符串超过指定长度
   {

#ifdef WIN32 //
         //clog << "Received a message that was too large" << endl;
#endif

      return false;
   }
   //buf[*len - 1]=0;
    
   return true;
}

//===========================================
//sendMessage函数说明
//函数功能：给目标主机发送消息
//参数说明：fd：连接成功的Socket
//          buf：发送的字符串
//          l：发送的字符串的长度
//          dstIp：目标主机ip
//          dstPort：目标主机端口号
//函数返回：true：消息接收成功
//          false：消息接收失败
//===========================================
bool sendMessage( Socket fd, char* buf, int l, unsigned int dstIp, unsigned short dstPort)
{
   assert( fd != INVALID_SOCKET );//fd不能为空
    
   int s;
   if ( dstPort == 0 ) //端口为0
   {
      // sending on a connected port 
      assert( dstIp == 0 );
		
      s = send(fd,buf,l,0);
   }
   else //端口不为0
   {
      assert( dstIp != 0 );
      assert( dstPort != 0 );
        
      struct sockaddr_in to;
      int toLen = sizeof(to);
      memset(&to,0,toLen);
        
      to.sin_family = AF_INET;
      to.sin_port = htons(dstPort);
      to.sin_addr.s_addr = htonl(dstIp);
        
      s = sendto(fd, buf, l, 0,(sockaddr*)&to, toLen);//数，成功则返回实际传送出去的字符失败返回－1
   }// end
    
   if ( s == SOCKET_ERROR ) //发送函数返回值判断
   {
      int e = getErrno();//获得错误类型
      switch (e)
      {
         case ECONNREFUSED:
         case EHOSTDOWN:
         case EHOSTUNREACH:
         {
            // quietly ignore this 
         }
         break;
         case EAFNOSUPPORT:
         {
            cerr << "err EAFNOSUPPORT in send" << endl;
         }
         break;
         default:
         {
            cerr << "err " << e << " "  << strerror(e) << " in send" << endl;
         }
      }
      return false;
   }// end if
    
   if ( s == 0 ) //
   {
      cerr << "no data sent in send" << endl;
      return false;
   }
    
   if ( s != l )
   {

#ifdef WIN32 //
         cerr << "only " << s << " out of " << l << " bytes sent" << endl;
#endif

      return false;
   }
    
   return true;
}

//===========================================
//initNetwork()函数说明
//函数功能：初始化Socket
//参数：    无
//函数返回：  true：函数执行成功
//            false：函数执行失败
//===========================================
bool initNetwork()
{
#ifdef WIN32 
   WORD wVersionRequested = MAKEWORD( 2, 2 ); //可以调用的Windows Sockets的最高版本
   WSADATA wsaData;//接收Windows Sockets的具体信息
   int err;
	
   err = WSAStartup( wVersionRequested, &wsaData ); //初始化Winsock
   if ( err != 0 ) 
   {
      //找不到可用的WinSock DLL
      cerr << "Could not load winsock" << endl;
      assert(0); // is this is failing, try a different version that 2.2, 1.0 or later will likely work 
      return false;
   }
    
   /* Confirm that the WinSock DLL supports 2.2.*/
   /* Note that if the DLL supports versions greater    */
   /* than 2.2 in addition to 2.2, it will still return */
   /* 2.2 in wVersion since that is the version we      */
   /* requested.                                        */
    
   if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) 
   {
      /* Tell the user that we could not find a usable */
      /* WinSock DLL.                                  */
      WSACleanup( );
      cerr << "Bad winsock verion" << endl;
      assert(0); // is this is failing, try a different version that 2.2, 1.0 or later will likely work 
	  return false;
   }    
   return true;

#endif
}

//===========================================
//ParseHostName函数说明
//函数功能：字符串转换成ip和port分离的形式
//参数说明：peerName：终端主机IP和端口，例如10.21.5.106:32156
//          ip：从peerName字符串分割出的ip，返回参数
//          portVal：从peerName字符串分割出的port，返回参数
//          defaultPort：默认端口，默认参数值3478
//函数返回：true：函数执行成功
//          false：函数执行失败
//===========================================

bool ParseHostName(const char* peerName,unsigned int* ip,unsigned short* portVal,unsigned short defaultPort)
{
	//目标主机IP
	in_addr sin_addr;
	char host[512];
	//peerName拷贝到host字符数组中
	strncpy(host,peerName,512);
	host[512-1]='\0';
	char* port = NULL;
	unsigned short portNum = defaultPort;
	//host字符数组中查找':'字符
	char* sep = strchr(host,':');
	if ( sep == NULL )//没有找到':'字符
	{
		portNum = defaultPort;
	}
	else//找到':'字符
	{
		*sep = '\0';
		port = sep + 1;
		//若参数endptr不为NULL，strtol函数会将遇到不合条件而终止的nptr中的字符指针由endptr返回；若参数endptr为NULL，则会不返回非法字符串
		char* endPtr=NULL;
		//参数nptr字符串根据参数base（参数base代表采用的进制方式）来转换成长整型数
		portNum = strtol(port,&endPtr,10);
		if ( endPtr != NULL )
		{
			if ( *endPtr != '\0' )
			{
				portNum = defaultPort;
			}
		}
	}

	if ( portNum < 1024 ) return false;
	if ( portNum >= 0xFFFF ) return false;

	//解析ip
	struct hostent* h;

#ifdef WIN32 //WIN32环境下执行下面的代码
	assert( strlen(host) >= 1 );
	if ( isdigit( host[0] ) ) //host字符串以数字开头
	{
		//将一个点分十进制的IP转换成一个长整数型数
		unsigned long a = inet_addr(host);
		//将一个无符号长整形数从网络字节顺序转换为主机字节顺序
		*ip = ntohl( a );
	}
	else
	{
		//用域名或主机名获取IP地址
		h = gethostbyname( host );
		if ( h == NULL )
		{
			int err = getErrno();
			cerr << "error was " << err << std::endl;
			assert( err != WSANOTINITIALISED );
			//127.0.0.1
			*ip = ntohl( 0x7F000001L );
			return false;
		}
		else
		{
			sin_addr = *(struct in_addr*)h->h_addr;
			*ip = ntohl( sin_addr.s_addr );
		}
	}

#else //Linux环境下执行下面的代码
	if ( isdigit( host[0] ) ) //host字符串以数字开头
	{
		//将一个点分十进制的IP转换成一个长整数型数
		unsigned long a = inet_addr(host);
		//将一个无符号长整形数从网络字节顺序转换为主机字节顺序
		*ip = ntohl( a );
	}
	else
	{
		h = gethostbyname( host );
		if ( h == NULL )
		{
			int err = getErrno();
			std::cerr << "error was " << err << std::endl;
			*ip = ntohl( 0x7F000001L );
			return false;
		}
		else
		{
			sin_addr = *(struct in_addr*)h->h_addr;
			*ip = ntohl( sin_addr.s_addr );
		}
	}

#endif

	*portVal = portNum;

	return true;
}

//===========================================
//GetLocalIp()函数说明
//函数功能：通过主机名获得本机的ip
//参数：     ip：整形的ip地址
//			ipString：ip地址字符串
//函数返回：  true：函数执行成功
//            false：函数执行失败
//===========================================
bool GetLocalIp(unsigned int* ip,char* ipString)
{
	char peerName[256];  
	int nStatus = gethostname(peerName, sizeof(peerName)); 
	printf("peerName is %s\n",peerName);
	if (nStatus == SOCKET_ERROR )  
	{  
		return false;  
	}  
	in_addr sin_addr;
	struct hostent* host;
	//用域名或主机名获取IP地址
	host = gethostbyname( peerName);
	if ( host == NULL )
	{
		int err = getErrno();
		cerr << "error was " << err << std::endl;
		assert( err != WSANOTINITIALISED );
		//127.0.0.1
		*ip = ntohl( 0x7F000001L );
		ipString = "127.0.0.1";
		return false;
	}
	else
	{
		ipString = inet_ntoa(*(struct in_addr *)host->h_addr);
		sin_addr = *(struct in_addr*)host->h_addr;
		*ip = ntohl( sin_addr.s_addr );
		printf("sin_addr.s_addr is %ld,*ip is %d\n",sin_addr.s_addr,*ip);

	} 
	return true;
}

//===========================================
//GetDate函数说明
//函数功能：获得系统当前日期时间
//参数：    无
//函数返回：  系统当前日期时间字符串
//===========================================
void
GetDate(char* t)//
{
 #ifdef WIN32
 	SYSTEMTIME currentTime;
 	GetLocalTime(&currentTime);
 	sprintf(t,"%d-%d-%d %d:%d:%d.%d",currentTime.wYear,currentTime.wMonth,currentTime.wDay,
 		currentTime.wHour,currentTime.wMinute,currentTime.wSecond,currentTime.wMilliseconds);
 #else

  	time_t timep;
  	struct tm *p;
  	time(&timep);
  	p = localtime(&timep);
  	struct timeval tv;
  	gettimeofday(&tv,NULL);
  	sprintf(t,"%d-%d-%d %d:%d:%d.%d", 1900 + p->tm_year, 1 + p->tm_mon,
  			p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec,tv.tv_usec/1000);

 #endif
}

//===========================================
//GetTime函数说明
//函数功能：获得系统当前时间
//参数：    无
//函数返回：  系统当前时间的time_t格式
//===========================================
time_t
GetTime()
{
	time_t timep;
	time( &timep);
	return timep;
}

//===========================================
//CloseSocket函数说明
//函数功能：关闭Socket
//参数：    无
//函数返回：  成功或者失败
//===========================================
int CloseSocket( Socket fd )
{
#ifdef WIN32
//	WSACleanup( );
	return closesocket(fd);
#else
	return close(fd);
#endif
};



/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End:
