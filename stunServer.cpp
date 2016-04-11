////////////////////////////////////////////////////////////
//公司名称：西安影子科技有限公司
//产品名称：思扣快信
//版 本 号：1.0.0.0
//文 件 名：stunServer.cpp
//开发人员：赵娟
//日    期：2015-11-16
//更新人员：
//更新日期：
//文件说明：stun服务器端
////////////////////////////////////////////////////////////

#include "typedef.h"
#include "logfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <fstream>

using namespace std;

typedef struct
{
	u_8	flag; //
	RequestMessagePackage msg; //
}MessageBuffer;

pthread_t threadHandle[SOCKET_NUMBER]; //
RECVPARAM pRecvParam[SOCKET_NUMBER];//
MessageBuffer buffer[SOCKET_NUMBER][MAX_MESSAGE_NUMBER];//
Logger Log[SOCKET_NUMBER];
char writeChar[MSG_TOTAL_LENGTH];

//===========================================
//FindBuffer函数说明
//函数功能：查找MessageBuffer中状态为findflag的下标
//参数：    buffer：MessageBuffer数组
//			bufferNumber：MessageBuffer长度
//			findNumber：找到并返回的下标
//			findflag：查找条件
//函数返回：true： 函数执行成功
//			false：函数执行失败
//===========================================
bool 
FindBuffer(MessageBuffer* buffer, u_32 bufferNumber, int* findNumber,u_8 findflag)
{
	*findNumber = -1;
	for (u_32 i=0;i<bufferNumber;i++)
	{
		if (buffer[i].flag == findflag)
		{
			*findNumber = i;
			break;
		}
	}
	if (*findNumber == -1)
	{
		return false;
	}
	return true;
}

//===========================================
//FindAndSend函数说明
//函数功能： 查找状态为2的buffer，如果找到就发送数据
//参数：    buffer: buffer 数组
//		   bufferSize: buffer大小
//		   portSocket: socket连接
//		   fileName: 文件名
//函数返回：    无
//===========================================
void 
FindAndSend(MessageBuffer* buffer, u_16 bufferSize, Socket portSocket,u_16 number)
{
	int findN = -1;
	bool ret = FindBuffer(buffer,bufferSize,&findN,SEND_BUFFER_FLAG);
	if ( ret) // 找到了
	{
		int msgSize = sizeof( RequestMessagePackage);
		memset(writeChar,0x00,MSG_TOTAL_LENGTH);
		sprintf(writeChar,"Send To IP: [ %d  ], PORT: [ %d ]\n",
				buffer[findN].msg.messageContent.localNatAddr.ip,
				buffer[findN].msg.messageContent.localNatAddr.port);
		Log[number].Log(writeChar);
		//发送
		ret = sendMessage(portSocket,(char*)(&(buffer[findN].msg)),msgSize,
			buffer[findN].msg.messageContent.localNatAddr.ip,
			buffer[findN].msg.messageContent.localNatAddr.port);
		//发送完后，该buffer的状态置为0
		buffer[findN].flag = AVAILABLE_BUFFER_FLAG;
		usleep(1);
	}
}

//===========================================
//MainProc函数说明
//函数功能： 线程调用函数
//参数：    lpParameter：传入参数，包括scoket连接、buffer数组下标、文件名等信息
//函数返回： 无
//===========================================
void*
MainProc(void* lpParameter)
{
	Socket portSocket = ((RECVPARAM*)lpParameter)->sock;
	u_16 number = ((RECVPARAM*)lpParameter)->number;
	bool isok;
	while(1)
	{
		//第一步：查找状态为0的buffer
		int findNumber = -1;
		isok = FindBuffer(&(buffer[number][0]),MAX_MESSAGE_NUMBER,&findNumber,AVAILABLE_BUFFER_FLAG);
		if( isok) //找到了
		{
			NetAddr from;
			int msgSize = sizeof( RequestMessagePackage);
			bool ret = getMessage(portSocket,(char*)(&(buffer[number][findNumber].msg)),&msgSize,&from.ip,&from.port);
			if ( ret) //接收到数据
			{
				//判断消息格式是否正确
				if( CheckRequestMessage(&(buffer[number][findNumber].msg)) &&
					buffer[number][findNumber].msg.dataType == REQUEST_TYPE)
				{
					memset(writeChar,0x00,MSG_TOTAL_LENGTH);
					sprintf(writeChar,"Received Buffer From IP:[ %d ], PORT: [ %d ]\n",from.ip,from.port);
					Log[number].Log(writeChar);
					//该buffer的状态置为2
					buffer[number][findNumber].flag = SEND_BUFFER_FLAG;
					//填写该buffer的信息
					buffer[number][findNumber].msg.messageContent.localNatAddr.ip = from.ip;
					buffer[number][findNumber].msg.messageContent.localNatAddr.port = from.port;
				}
			}
		}
		//查找状态为2的buffer,找到后发送
		FindAndSend(&(buffer[number][0]),MAX_MESSAGE_NUMBER,portSocket,number);
		usleep(1);
	}
	return NULL;
}

//===========================================
//ThreadProc函数说明
//函数功能：打开端口，并创建线程
//参数：    startPort：起始端口号
//		   portNumber：打开的端口个数
//函数返回： 成功或者失败
//===========================================
bool
ThreadProc( u_16 startPort, u_16 portNumber)
{
	if (portNumber == 0)
	{
		return false;
	}
	char time[256];
	for ( int i=0;i<portNumber;i++)
	{
		//打开端口
		Socket localSocket = openPort(startPort+i,0);
		if (localSocket == INVALID_SOCKET)
		{
			continue;
		}
		pRecvParam[i].sock = localSocket;
		pRecvParam[i].number = i;
		pRecvParam[i].startPort = startPort+i;
		GetDate(&time[0]);
		char filename[512];
		sprintf(filename,"%d %s.log",startPort+i,time);
		Log[i].CreateFile(filename);
		//接收发送线程
		int ret = pthread_create(&threadHandle[i],NULL,MainProc,&pRecvParam[i]);
		if( ret != 0)
		{
			cout<<"false"<<endl;
			continue;
		}
		usleep(1);
	}
	for(int i=0;i<portNumber;i++)
	{
		pthread_join(threadHandle[i],NULL);
	}
	for ( int i=0;i<SOCKET_NUMBER;i++)
	{
		if ( pRecvParam[i].sock != INVALID_SOCKET)
		{
			CloseSocket( pRecvParam[i].sock);
		}
	}
	return true;
}

static char Usage[] = 
	"Usage: stunServer [options]\n"
	"-p  server port start number(like 10020)\n";

//===========================================
//main函数说明
//函数功能：入口函数
//===========================================
int main(int argc, char** argv)
{
	//解析参数
	if (argc == 1)
	{
		printf("%s",Usage);
		return -1;
	}
	u_16 localPort = 0;
	for (int arg = 1; arg < argc; arg++)
	{
		if ( !strcmp(argv[arg],"-p"))
		{
			arg++;
			if ( argc <= arg ) 
			{
				printf("%s",Usage);
				return -1;
			}
			localPort = strtol(argv[arg],NULL,10);
		}
	}
	ThreadProc( localPort, SOCKET_NUMBER);
	return 0;
}

