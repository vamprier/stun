#include "typedef.h"


//===========================================
//CreateCRC函数说明
//函数功能：生成CRC校验码
//参数：    msg：字符串
//          msgSize：字符串长度
//          start：开始位置
//			end：结束位置
//函数返回：  CRC校验码
//===========================================
u_8 
CreateCRC( u_8* msg)
{
	size_t msgSize = DATA_TOTAL_LENGTH;
	size_t start = 0;
	size_t end = DATA_TOTAL_LENGTH;
	if (end > msgSize-1 || start < 4)
	{
		return -1;
	}
	u_8 ch = msg[start];
	for (size_t i=start+1;i<end;i++)
	{		
		ch = ch ^ msg[i];
	}
	return ch;
}


//===========================================
//***函数说明
//函数功能：***
//参数：    ***
//			***
//			***
//函数返回：***
//===========================================
bool CheckCRC(u_8 crc1,u_8 crc2)
{	
	if (crc1 == crc2)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//===========================================
//***函数说明
//函数功能：***
//参数：    ***
//			***
//			***
//函数返回：***
//===========================================
bool CheckRequestMessage(RequestMessagePackage* msp)
{
	//判断消息格式是否正确
	if ( msp->headFlag != PACKAGE_HEADER_FLAG || msp->tailFlag != PACKAGE_TAIL_FLAG)
	{
		return false;
	}
	return true;
}

//===========================================
//***函数说明
//函数功能：***
//参数：    ***
//			***
//			***
//函数返回：***
//===========================================
bool CheckMessage(MessagePackage* msp)
{
	//判断消息格式是否正确
	if ( msp->headFlag != PACKAGE_HEADER_FLAG || msp->tailFlag != PACKAGE_TAIL_FLAG )//|| !CheckCRC( CreateCRC(&msp->messageContent.dataContent[0]),msp->CRC)
	{
		return false;
	}
	return true;
}
