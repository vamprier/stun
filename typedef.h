
#ifndef DATATYPE_
#define DATATYPE_

#include "udp.h"

#define  SOCKET_NUMBER 2    //端口个数
#define  MAX_MESSAGE_NUMBER 10    //存放数据包的数组长度

#define  MSG_TOTAL_LENGTH 512    //数据包的长度
#define  UDP_MESSAGE_LENGTH 20    //udp包头的大小
#define  DATA_TOTAL_LENGTH (MSG_TOTAL_LENGTH-UDP_MESSAGE_LENGTH-36)    //数据的大小

#define  PACKAGE_HEADER_FLAG 0x55    //数据包头标志
#define  PACKAGE_TAIL_FLAG 0xAA    //数据包尾标志

#define  REQUEST_TYPE 0x1    //请求返回Nat信息
#define  EXCHANG_TYPE 0x2    //请求交换信息
#define  TRANSFORM_TYPE 0x3  //请求传送信息

#define  REQUEST_COMMAND 0x1    //握手请求
#define  RESPOND_COMMAND 0x2    //握手应答
#define  SUCCESS_COMMAND 0x3    //握手成功

#define  AVAILABLE_BUFFER_FLAG 0   //可用buffer标志
#define  SEND_BUFFER_FLAG 2   //可发送buffer标志

#define TRY_SEND_NUMBER 3

#define TIME_OUT 40


typedef unsigned char u_8;
typedef unsigned short u_16;
typedef unsigned int u_32;
typedef unsigned long long u_64;

//地址结构体
typedef struct
{
	u_32 ip; //IP地址
	u_16 port;//端口号
}NetAddr;

//请求数据
typedef struct
{
	u_8  command;  //指令 0x1:握手请求 0x2:握手应答 0x3:握手成功
	u_32  pairingFlag;//包配对信息
	NetAddr localAddr;
	NetAddr localNatAddr;
	NetAddr remoteAddr;
	NetAddr remoteNatAddr;
}RequestData;

//传输数据
typedef struct
{ 
	NetAddr localAddr;
	NetAddr localNatAddr;
	NetAddr remoteAddr;
	NetAddr remoteNatAddr;
	u_8	dataContent[DATA_TOTAL_LENGTH]; //包内容
}Data;

//请求消息结构体
typedef struct
{
	u_8	headFlag; //包头，固定为0x55
	u_8  dataType; //数据类型 0x1:请求返回Nat信息 0x2:请求交换信息
	RequestData messageContent; //包数据
	u_8  CRC; //校验位
	u_8  tailFlag; //包尾，固定为0xAA
}RequestMessagePackage;

//数据传输消息结构体
typedef struct
{
	u_8	headFlag; //包头，固定为0x55
	u_8  dataType; //数据类型 0x3:数据传输信息
	u_16    realMessageSize; //包实际长度
	u_16	messageSize; //包长度
	u_16    totalNumber; //总共分几个包
	u_16    messageNumber; //包序号,编号从1开始，如果(包实际长度 = 包长度)的话，该位为0
	Data messageContent; //包数据
	u_8  CRC; //校验位
	u_8  tailFlag; //包尾，固定为0xAA
}MessagePackage;

struct RECVPARAM  
{  
	Socket sock;  //
	u_16    number;
	u_16    startPort;
};

//
u_8 CreateCRC( u_8* msg);
//
bool CheckCRC();
//
bool CheckRequestMessage(RequestMessagePackage* msp);
//
bool CheckMessage(MessagePackage* msp);

#endif
