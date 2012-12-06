/************************************************************************/
/* 
FSKModem.cpp
xiaotanyu13
2012/11/12
xiaot.yu@sunyard.com

��װ��FSK���ƽ���ĺ�����������ķ�����ʵ��*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include "FSKModem.h"

extern "C" {
#include "fftsg_h.c"
}

#define FSK_MODEM_GLOBALS



CFSKModem::CFSKModem()
{
	this->m_bIobitFlag = false;
	this->m_cPackageCount = 0;
	this->m_fPoint = (float*)malloc(2 * MAX_N_POINTS * sizeof(float));//�˲���ʱ�򱣴����� 
	// ���ֵֻ���˲���ʱ���õ��ˣ�����ʱ��û���ô������������Ҫ���������һ��
}

/*


���������
type:	������0 �� 1
0��ʾ 0�Ĳ�
1��ʾ 1�Ĳ�
���������
retData:	����buffer��ָ��
����ֵ��
��retData������ƫ��ֵ
*/
int CFSKModem::ModulateBit(int type,short* retData)
{
	int index = 0;
	switch(type)
	{
	case 1: // �����ǰ������1�������1 1�Ĳ������б仯
		for(int i = 0; i < MODULATE_SAMPLE/2; i++)
		{
			*(retData + index++) = ((m_bIobitFlag == true) ?AMP_U:AMP_D);
		}
		m_bIobitFlag = !m_bIobitFlag;
		for(int i = 0; i < MODULATE_SAMPLE/2; i++)
		{
			*(retData + index++) = ((m_bIobitFlag == true) ?AMP_U:AMP_D);
		}
		m_bIobitFlag = !m_bIobitFlag;
		break;
	case 0: // �����0�������0,0�Ĳ�����û�б仯
		for(int i = 0; i < MODULATE_SAMPLE; i ++)
		{
			*(retData + index++) = ((m_bIobitFlag == true) ?AMP_U:AMP_D);
		}
		m_bIobitFlag = !m_bIobitFlag;
		break;
	}
	return index;
}
/*
����һ���ֽڵ�����,һ���ֽ�֮����Ҫ������0
���������
byte��	��Ҫ���Ƶ��ֽ�
���������
retData��������ƺ����������
����ֵ��
��retData������ƫ��
*/
int CFSKModem::ModulateByte(BYTE byte,short* retData)
{
	int offset = 0;
	for(int i = 0; i < BYTE_LEN; i ++)
	{
		if((byte << i) & 0x80)
		{
			offset += this->ModulateBit(1,retData + offset);
		}
		else
		{
			offset += this->ModulateBit(0,retData + offset);
		}
	}

	return offset;
}
/*
��ͬ���򣬹̶�Ϊ0x01015555
���������

���������
data��������ƺ����������
����ֵ��
��data������ƫ��
*/
int CFSKModem::PackSYNC(BYTE* data)
{
	int offset = 0;
	data[offset++] = 0x55;
	data[offset++] = 0x55;
	data[offset++] = 0x01;
	data[offset++] = 0x01;
	return offset;
}
/*
�������1���ֽ�
���������

���������
data��������ƺ����������
����ֵ��
��data������ƫ��
*/
int CFSKModem::PackCount(BYTE* data)
{
	this->m_cPackageCount ++;
	this->m_cPackageCount = this->m_cPackageCount % 256;
	//BYTE* count = (BYTE*)&this->m_cPackageCount;
	int offset = 0;
	data[offset++] = this->m_cPackageCount;
	return offset;

}
/*
�����ݳ�����2�ֽ�
���������
len��	���ݵĳ���
���������
data��������ƺ����������
����ֵ��
��data������ƫ��
*/
int CFSKModem::PackLength(int len,BYTE* data)
{
	int offset = 0;
	data[offset++] = len % 256;
	data[offset++] = len / 256;
	return offset;
}
/*
��������
���������
dataIn����Ҫ���Ƶ�����
len��	���ݵĳ���
���������
data��������ƺ����������
����ֵ��
��data������ƫ��
*/
int CFSKModem::PackData(char* dataIn,int len,BYTE* data)
{
	memcpy(data,(BYTE*)dataIn,len);
	return len;
}
/*
��У����
���������
crc��	У��ֵ
���������
data��������ƺ����������
����ֵ��
��data������ƫ��
*/
int CFSKModem::PackCRC(short crc,BYTE* data)
{
	int offset = 0;
	data[offset++] = crc % 256;
	data[offset++] = crc / 256;
	return offset;
}
/*
���������ʽ��
ͬ����	��������	���ݳ�����	������	У����
4�ֽ�	1�ֽ�		2�ֽ�		n�ֽ�	2�ֽ�
���������
data:	�ϲ㴫������������
len:	���ݵĳ���
���������
outLen������ֵ�ĳ���
����ֵ��
�������ϸ�ʽ��õİ�
*/
BYTE* CFSKModem::PackField(char* data,int len,int *outLen)
{
	int bufLen = len + 9;
	BYTE *buf = NULL;
	int offset = 0;
	int dataLen = 0;
	//buf = (BYTE*)calloc(bufLen,0);
	buf = new BYTE[bufLen];
	offset += this->PackSYNC(buf + offset);
	offset += this->PackCount(buf + offset);
	offset += this->PackLength(len,buf+offset);
	dataLen = this->PackData(data,len,buf+offset);
	// crcУ��ֻ��ҪУ�����������
	short crc = this->CalculateCRC(buf+offset,dataLen);
	offset += dataLen;
	offset += this->PackCRC(crc,buf+offset);
	*outLen = offset;
	return buf;
}

/*
�����ݽ���crcУ�飬���ҷ���crcУ��ֵ
���������
buf��	��Ҫ����crc������
len��	���ݵĳ���
���������

����ֵ��
crcУ����
*/
short CFSKModem::CalculateCRC(BYTE *buf,int len)
{
	BYTE hi,lo;
	int i;
	BYTE j;
	short crc;
	crc=0xFFFF;
	for (i=0;i<len;i++)
	{
		crc=crc ^ *buf;
		for(j=0;j<8;j++)
		{
			BYTE chk;
			chk=crc&1;
			crc=crc>>1;
			crc=crc&0x7fff;
			if (chk==1)
				crc=crc^0x8408;
			crc=crc&0xffff;
		}
		buf++;
	}
	hi=crc%256;
	lo=crc/256;
	crc=(hi<<8)|lo;
	return crc;
}


/*
��������
���������
data��	��Ӧ�ò㴫����������
len��	data�ĳ���
���������
outFrameLen��	���Ƴ���Ƶ���ݵĳ���
����ֵ��
���Ƴɵ���Ƶ����,�������ֵ��Ҫ�û��Լ�ȥ����
*/
short* CFSKModem::Modulate(char* data,int len,int
						   * outFrameLen)
{
	int packageLen = 0;
	BYTE* packBuf = this->PackField(data,len,&packageLen);
	// ��ʼ����
	short* voiceData = NULL;
	int offset = 0;
	int voiceLen = packageLen * MODULATE_SAMPLE * BYTE_LEN;
	voiceData = new short[voiceLen];

	for(int i = 0; i < packageLen; i ++)
	{
		offset += this->ModulateByte(packBuf[i],voiceData+offset);
	}
	free(packBuf);
	*outFrameLen = offset;
	return voiceData;
}


void CFSKModem::FindFrame(short *pData, unsigned long len, long *start, long *end)
{
	unsigned long i, j;
	unsigned long E;
	BYTE flag = 0;

	*start = *end = -1;
	if(len < 128)
		return;
	for(i = 0; i <= len - E_POINTS; i += E_POINTS) {
		E = 0;
		for(j = 0; j < E_POINTS; j ++) {
			E += abs(pData[i+j]);
		}
		if(!flag) { // find start pos of sound
			if(E > E_SOUND_THRESHOLD) {
				if(i >= E_POINTS)
					*start = i - E_POINTS;
				else
					*start = i;
				flag = 1;
			}
		}
		else {
			if(E < E_SILENCE_THRESHOD) {
				*end = i + E_POINTS;
				//*end = i;
				return;
			}
		}
	}
	// start pos found
	if(flag)
		*end = len -1;
}
unsigned long CFSKModem::GetN(unsigned long len)
{
	unsigned long N;

	if(len <= 64) {
		N = 64;
	}
	else if(len <= 128) {
		N = 128;
	}
	else if(len <= 512) {
		N = 512;
	}
	else if(len <= 1204) {
		N = 1024;
	}
	else if(len <= 2048) {
		N = 2048;
	}
	if(len <= 4096) {
		N = 4096;
	}
	else if(len <= 8192) {
		N = 8192;
	}
	else if(len <= 16384) {
		N = 16384;
	}
	else if(len <= 32768) {
		N = 32768;
	}
	else if(len <= 64*1024) {
		N = 64*1024;
	}
	else if(len <= 128*1024) {
		N = 128*1024;
	}
	else if(len <= 256*1024) {
		N = 256*1024;
	}
	else if(len <= 512*1024) {
		N = 512*1024;
	}
	else {
		return 0;
	}

	return N;
}

int CFSKModem::GetValidData(short *InDataBuf,short *OutDataBuf,unsigned long lenth)
{
	unsigned long i = 0;//ָ���i����
	unsigned long NumberOfLow = 0;//С���Ȳ��ĸ���
	unsigned long k = 0;//ָ���K����Ч����
	int Lenthofdata = 0;
	int isEnd = 0;

	if(InDataBuf[lenth-1]>1500 || InDataBuf[lenth-1]<-1500) 
	{
		isEnd++;
	}
	if(InDataBuf[lenth-2]>1500 || InDataBuf[lenth-2]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-3]>1500 || InDataBuf[lenth-3]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-4]>1500 || InDataBuf[lenth-4]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-5]>1500 || InDataBuf[lenth-5]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-6]>1500 || InDataBuf[lenth-6]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-7]>1500 || InDataBuf[lenth-7]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-8]>1500 || InDataBuf[lenth-8]<-1500){
		isEnd++;
	}
	if(InDataBuf[lenth-9]>1500 || InDataBuf[lenth-9]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-10]>1500 || InDataBuf[lenth-10]<-1500) {
		isEnd++;
	}
	if(isEnd >6)
	{
		return -1;
	}

	for(i=0;i<lenth;i++)
	{   
		NumberOfLow = 0;
		while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //ȥ�� ���� 3��С��500�ĵ�
		{
			i++;
			NumberOfLow++;
			if(i == lenth)
			{
				goto endLow;
			}
		}
		if(NumberOfLow < 3)//�����м���ֵ�С���Ȳ�С��3��Ҫ�˻�ȥ
		{
			i -= NumberOfLow;
		} 
		OutDataBuf[Lenthofdata] = InDataBuf[i];
		Lenthofdata++;

	}
endLow:
	//ȥ�����ţ���������20�����ڻ���С��0 ��, �Ѷ����ȥ��
	short currentData = OutDataBuf[0];
	int tempLenth = Lenthofdata;
	int j = 1;

	for(i=0; i<tempLenth; i++)
	{   
		if(i == 47)
			i = i;
		if((currentData <0 && OutDataBuf[i] <=0) ||(currentData >0 && OutDataBuf[i] >=0) )
		{
			j++;
		}
		else
		{	
			currentData = OutDataBuf[i];
			if(j>40)
			{
				memcpy(&OutDataBuf[i]-(j-20),OutDataBuf+(i-20),(tempLenth-(i-20))*2);
				i = i - (j-20);
				tempLenth = tempLenth - (j-40); 
			}
			j = 1;
		}
	}
	if(j>40)
	{
		memcpy(&OutDataBuf[i]-(j-20),OutDataBuf+(i-20),(tempLenth-(i-20))*2);
		i = i - (j-20);
		tempLenth = tempLenth - (j-40); 
	}
	j = 1;

	return tempLenth;
}

/*****************************************
����:�ҵ�ͬ��ͷ  55550101 �� EFEF0101
���������õĺ����嵥: ��
���ñ������ĺ����嵥: main
�������:  *DataBuf   
�������:  �ڼ����㿪ʼ������
��������ֵ˵��:   
ʹ�õ���Դ 
******************************************/
BYTE CFSKModem::FindHead(short * InDataBuf,unsigned long lenth,unsigned long *endlen,BYTE MobileType)
{
	unsigned long i = 0;//ָ���i����

	float LengthOfZorePassage = 0;//�����֮����
	float LastRatio = 0;///�������һ�εı���
	float	RatioOfZorePassage = 0;//�������һ�εı���

	unsigned long NumberOfLow = 0;//С���Ȳ��ĸ���
	unsigned long DataHead = 0;//ͬ��ͷ
	unsigned long datastart = 0;//��ǰ�ڽ�Ĳ��Ŀ�ʼ��
	BYTE bit0flag = 0;//����ʵ������С����ʾ1

	for(;i<lenth;)
	{   
		NumberOfLow = 0;
		while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //ֱ���д���500�ĵ㣬ȥ"����"
		{
			i++;
			NumberOfLow++;
			if(i == lenth)
			{
				return 0;
			}
		}
		if(NumberOfLow < 5)//�����м���ֵ�С���Ȳ�С��5��Ҫ�˻�ȥ
		{
			i -= NumberOfLow;
		}
		datastart = i;//��ǰ���Ŀ�ʼ��
		LastRatio = RatioOfZorePassage;//������һ�εĹ�������
		if(InDataBuf[i] >= 0)//�������ֵ���ڵ���0
		{
			while(InDataBuf[i] >= 0)//ֱ������ֵС��0
			{
				i++;//��һ��������	
			}
		}
		else//�������ֵС��0
		{
			while(InDataBuf[i] < 0)//ֱ������ֵ����?
			{
				i++;//��һ��������				
			}
		}
		RatioOfZorePassage = (float(abs(InDataBuf[i]))) 
			/ ( float(abs(InDataBuf[i - 1])) + float(abs(InDataBuf[i])) );

		//���µ�ǰ�������֮��Ŀ��	
		if(i == 590)  
			i = i+0;
		LengthOfZorePassage =LastRatio  + (i - datastart - 1) + (1 - RatioOfZorePassage); 
		if(( LengthOfZorePassage >=  (3.0/ 2.0)*((float) MobileType+1.0) )
			&&(LengthOfZorePassage<(12.0/ 3.0)*((float) MobileType+1.0))) //����Ǵ�	

		{
			if(bit0flag == 1)
			{
				DataHead = 0;//���С��֮���Ǵ󲨣������¿�ʼ��ͷ
				bit0flag = 0;
			}
			DataHead = DataHead<<1;
			DataHead &= 0xFFFFFFFE;	
			//	0xEFEF0101
		}
		else if((LengthOfZorePassage>= (1.0/3.0)*((float) MobileType+1.0))
			&&(LengthOfZorePassage< (3.0/2.0)*((float) MobileType+1.0))&&(i != *endlen)) //�����С��
		{
			if(bit0flag == 0)//����ǵ�һ��С��
			{
				bit0flag = 1;
				continue;
			}
			else//����ǵڶ���С��
			{
				DataHead = DataHead<<1;		
				DataHead |= 0x00000001;
				//	0xEFEF0101
				bit0flag = 0;//����С��Ϊ"1"
			}			
		}
		//else if(LengthOfZorePassage < (2.0/3.0)) //����ǳ�С������Ϊ���Ӳ�
		else 
		{
			DataHead = 0;//��������������¿�ʼ��ͷ
		}


		if(( DataHead == 0x55550101 )||( DataHead == 0xEFEF0101 ))//������Ҫע�⣬����ȥ�ͷ�������ͬ��ͷ�ǲ�ͬ��
		{
			printf("\ndatahead = %x \n",DataHead);	//��ӡ����ͷ
			*endlen = i;
			return 1;//��i�㿪ʼ������������
		}
	}
	printf("no head\n");	//�Ҳ���ͬ��ͷ
	return 0;
}


/*****************************************
����:   ���ȫ������
���������õĺ����嵥: ��
���ñ������ĺ����嵥: main
�������:  *InDataBuf    ����ֵ��ַ
lenth        �ܳ���
startlen     ��ʼ�ĵط�
MmobileType  �ֻ�����
�������:  *OutDataBuf   ���ݱ���ĵط�
*endlen       �⵽�ĵ���
��������ֵ˵��:    0Ϊ����1Ϊ�ɹ����8λ����
ʹ�õ���Դ 
******************************************/
BYTE CFSKModem::GetAllData(BYTE *OutDataBuf, short *InDataBuf,
									unsigned long lenth,unsigned long *endlen,BYTE MobileType)
{	
	unsigned long i = *endlen - 1;//ָ���i����
	unsigned long j = 0;//ָ���j�����������
	unsigned long DataLenth = 0;//�������ݵĳ��ȣ�������ǰ���ֽڱ�ʾ

	float 	LengthOfZorePassage = 0;//�����֮����
	float	LastRatio = 0;///�������һ�εı���
	float	RatioOfZorePassage = 0;//�������һ�εı���
	unsigned long datastart = 0;//��ǰ�ڽ�Ĳ��Ŀ�ʼ��

	BYTE bit0flag = 0;//����ʵ������С����ʾ1
	BYTE bitindex = 0;//����������ݵ�λ��
	unsigned short crc = 0;//����У��
	/************************************/		
	//����Ĳ�������ʵ��0 ��1 ���ȵĶԱ�//
	unsigned short highest = 0;//ÿһλ�У�����ֵ����ߵ�//
	unsigned long sum0 = 0;//0���ܺ�
	unsigned long sum1 = 0;// 1���ܺ�
	unsigned short number0 = 0;//0�ĸ���
	unsigned short number1 = 0;// 1�ĸ���
	unsigned long k = 0;//����ʵ�ֲ���ÿһλ�е���߲����� �� ��ͨѭ��
	float RatioOf0b1 = 0;// ����1 ����߲���֮�� �� ����0����߲���֮��    �ı��ʡ�
	/************************************/	
	/************************************/		
	//����Ĳ�������ʵ�ֲ��μ����Լ��////
	float MaxOf0Wide = 0;//0�п��ƫ��֮��
	float MaxOf1Wide = 0;// 1�п��ƫ��֮��
	/************************************/	
	for(;i<lenth ;)
	{   
		datastart = i;//��ǰ���Ŀ�ʼ��
		LastRatio = RatioOfZorePassage;//������һ�εĹ�������
		if(InDataBuf[i] >= 0)//�������ֵ���ڵ���0
		{
			while(InDataBuf[i] >= 0)//ֱ������ֵС��0
			{
				i++;//��һ��������	
			}
		}
		else//�������ֵС��0
		{
			while(InDataBuf[i] < 0)//ֱ������ֵ����0
			{
				i++;//��һ��������				
			}
		}
		RatioOfZorePassage = \
			(float(abs(InDataBuf[i]))) / ( float(abs(InDataBuf[i - 1])) + float(abs(InDataBuf[i])) );

		if(i == 28036)
		{
			i = i;
		}
		//���µ�ǰ�������֮��Ŀ��	
		LengthOfZorePassage =LastRatio  + (i - datastart - 1) + (1 - RatioOfZorePassage);
		if(( LengthOfZorePassage >=  (3.0/ 2.0)*((float) MobileType+1.0) )
			&&(LengthOfZorePassage<(12.0/ 3.0)*((float) MobileType+1.0))) //����Ǵ�	
		{
			if(bit0flag == 1)//С�������Ǵ󲨾��ǳ�����
			{
				printf("\ndata error,0after1 \n");	
				*endlen = i;
				return 0;
			}
			/********************************************************/
			highest = abs(InDataBuf[datastart]);//�ó�ֵ
			for(k = datastart+1;k < i;k++)//�ҳ���λ�е���߲���ֵ
			{
				if( (abs(InDataBuf[k])) > highest )
				{
					highest = abs(InDataBuf[k]);
				}
			}
			if(highest < 300)//��߲���ֵ��Ӧ�ñ�300����
			{
				printf("\ndata error,0 smoll than 300 \n");	
				*endlen = i;
				return 0;
			}
			number0++;//���ٸ�0
			sum0 += highest;//ͳ��0�Ĳ���ֵ֮��
			/********************************************************/
			if(  fabs(LengthOfZorePassage - (MobileType+1)*2) > fabs(MaxOf0Wide) )
			{
				MaxOf0Wide = (LengthOfZorePassage - (MobileType+1)*2);
			}
			/********************************************************/
			OutDataBuf[j]  &= ~(1<<(7-bitindex));
			bitindex++;
		}

		else if((LengthOfZorePassage>= (1.0/3.0)*((float) MobileType+1.0))
			&&(LengthOfZorePassage< (3.0/2.0)*((float) MobileType+1.0))&&(i != *endlen)) //�����С��
		{
			/********************************************************/
			if( fabs(LengthOfZorePassage - (MobileType+1)) > fabs(MaxOf1Wide) )
			{
				MaxOf1Wide = (LengthOfZorePassage - (MobileType+1));
			}
			/********************************************************/
			if(bit0flag == 0)//����ǵ�һ��С��
			{
				bit0flag = 1;
				continue;
			}
			else//����ǵڶ���С��
			{   
				/********************************************************/
				highest = abs(InDataBuf[datastart]);//�ó�ֵ
				for(k = datastart+1;k < i;k++)//�ҳ���λ�е���߲���ֵ
				{
					if( abs(InDataBuf[k]) > highest )
					{	
						highest = abs(InDataBuf[k]);						
					}
				}
				if(highest < 300)//��߲���ֵ��Ӧ�ñ�300����
				{
					printf("\ndata error,1 smoll than 300 \n");	
					*endlen = i;
					return 0;
				}
				number1++;//���ٸ�1
				sum1 += highest;//ͳ��1�Ĳ���ֵ֮��
				/********************************************************/
				OutDataBuf[j] |= 1<<(7-bitindex);			
				bitindex++;
				bit0flag = 0;//����С��Ϊ"1"
			}			
		}
		else 
		{	
			if(i == *endlen)//��һ����ֻȡ��һ�㣬���Կ϶��Ƿǳ�С��
			{
				continue;
			}
			printf("\ndata error,too long or small \n");	
			*endlen = i;
			return 0;//���������Ĳ���ֱ����Ϊ������
		}


		if( bitindex == 8 )//8λ1�ֽ�
		{
			printf("%02x,", OutDataBuf[j]);//��ӡ����
			j++;
			bitindex = 0;
		}
		if((j == 1) && (bitindex == 0))// һ��ʼ1���ֽ��Ǽ�����
			this->m_cPackageCount = OutDataBuf[0];
		if((j == 3)&&(bitindex == 0))// �����������ֽ������ݳ���
		{
			DataLenth =  OutDataBuf[1] | (OutDataBuf[2] << 8);
		}
		//if(( j == 4)&&(bitindex == 0))
		if(( j == DataLenth + 3 + 2) && (j >= 3))//ȫ���������
		{
#if 0
			MaxOf0Wide = (3*MaxOf0Wide/(MobileType + 1)/2);
			MaxOf1Wide = (3*MaxOf1Wide/(MobileType + 1));
			printf("\nMaxOf0Wide is %f%%\n",(3*MaxOf0Wide/(MobileType + 1)/2)*100);//��ӡ
			printf("\nMaxOf1Wide is %f%%\n",(3*MaxOf1Wide/(MobileType + 1))*100);//��ӡ
			for(k = 2; k < DataLenth+1; k++)//����У�飬������֮�⣬ȫ�����
			{
				crc ^= OutDataBuf[k];
			}						
			if(crc != OutDataBuf[DataLenth + 1])//���У��ͨ����
			{			
				printf("\n CRC error \n");	//��ӡУ�����
				*endlen = i;
				return 0;//У�����
			}
#endif
			/*************************************************************************CRC16**/
			crc = OutDataBuf[DataLenth + 3] | (OutDataBuf[DataLenth + 4] << 8);

			/*	BYTE OutDataBuf111[3000] = {0};
			BYTE OutDataBuf1111[4] = {0x55,0x55,0x01,0x01};
			memset(OutDataBuf111, 0,3000);
			memcpy(OutDataBuf111,OutDataBuf1111,4);
			memcpy(OutDataBuf111+4,OutDataBuf,DataLenth);*/
			if(crc != this->CalculateCRC(OutDataBuf+3,DataLenth))//���У��ͨ����
			{			
				printf("\n CRC error \n");	//��ӡУ�����
				*endlen = i;
				return 0;//У�����
			}
			/***********************************************************************************/


			printf("\n end \n\n");
			*endlen = i;
			return 1;
		}
	}
	printf("\ndata error,no data \n");
	*endlen = i;
	return 0;//û������
}

/*****************************************
����:   �˲� (ƽ��ֵ�˲����ò��θ�ƽ��һЩ)
���������õĺ����嵥: ��
���ñ������ĺ����嵥: Demodulate
�������:  *InDataBuf  �Ӹõ�ַ��ʼ��
length      ����ô��
LowF       ��ȡ��Ƶ
HighF      ��ȡ��Ƶ
SampleRate ������
�������:   
��������ֵ˵��: ��
ʹ�õ���Դ 
******************************************/
/**/
#if 1
void CFSKModem::SmoothingWave(short *InDataBuf,unsigned long length, 
							  unsigned long LowF, unsigned long HighF, unsigned long SampleRate)
{
	unsigned long i, j, k, N;
	long start, end;
	unsigned long l, h;

	/*
	unsigned long NumberOfLow = 0;//С���Ȳ��ĸ���
	for(;i<length;)
	{   
	NumberOfLow = 0;
	while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //ȥ�� ���� 3��С��500�ĵ�
	{
	i++;
	NumberOfLow++;
	if(i == length)
	{
	return;
	}
	}
	if(NumberOfLow < 3)//�����м���ֵ�С���Ȳ�С��3��Ҫ�˻�ȥ
	{
	i -= NumberOfLow;
	}
	//InDataBuf[i] = (InDataBuf[i - 1] + InDataBuf[i] + InDataBuf[i + 1])/3;

	//i++;		
	}*/

#if 0
	for(i = 0; i < length;) {
		FindFrame(InDataBuf + i, length - i, &start, &end);
		if(start >= 0) {
			if(end == -1)
				end = length - i - 1;
			N = GetN(end - start + 512); // ǰ����������256��,ÿ��ֵΪ0
			memset(this->m_fPoint, 0, 2 * N * sizeof(float));
			for (j = 256; j < N; j ++)	{
				this->m_fPoint[2*j] = InDataBuf[i+start+j-256];
			}
			/* Calculate FFT. */
			cdft(N*2, -1, this->m_fPoint);
			/* Filter */
			l = (unsigned long)(LowF/((float)SampleRate/N));
			h = (unsigned long)(HighF/((float)SampleRate/N));
			for(k = 0; k < l; k ++) {
				this->m_fPoint[2*k] = this->m_fPoint[2*k+1] = 0;
			}
			for(k = h; k < N; k ++) {
				this->m_fPoint[2*k] =  this->m_fPoint[2*k+1] = 0;
			}

			/* Clear time-domain samples and calculate IFFT. */
			memset(InDataBuf+i+start, 0, (end-start)*2);
			icdft(N*2, -1, this->m_fPoint);
			for(k = 0; k < end-start; k ++) {
				InDataBuf[i+start+k] = (short)this->m_fPoint[2*k+256];
			}
			i += end;
		}
		else
			break;
	}
#else
	for(i = 0; i < length;) {
		this->FindFrame(InDataBuf + i, length - i, &start, &end);
		if(start >= 0) {
			if(end == -1)
				end = length - i - 1;
			N = this->GetN(end - start + 512); // ǰ����������256��,ÿ��ֵΪ0
			memset(this->m_fPoint, 0, 2 * N * sizeof(float));
			for (j = 256; j < end - start + 256; j ++)	{
				this->m_fPoint[j] = InDataBuf[i+start+j-256];
			}
			/* Calculate FFT. */
			rdft(N, 1, this->m_fPoint);
			/* Filter */
			l = (unsigned long)(LowF/((float)SampleRate/N));
			h = (unsigned long)(HighF/((float)SampleRate/N));
			for(k = 0; k < l; k ++) {
				this->m_fPoint[2*k] = this->m_fPoint[2*k+1] = 0;
			}
			for(k = h; k < N; k ++) {
				this->m_fPoint[2*k] =  this->m_fPoint[2*k+1] = 0;
			}

			/* Clear time-domain samples and calculate IFFT. */
			memset(InDataBuf+i+start, 0, (end-start)*2);

			rdft(N, -1, this->m_fPoint);
			for (j = 0; j <= N - 1; j++) {
				//this->m_fPoint[j] *= 2.0/ N;
				this->m_fPoint[j] /= N;
			}

			for(k = 0; k < end-start; k ++) {
				InDataBuf[i+start+k] = (short)this->m_fPoint[k+256];
			}

			i += end;
		}
		else
			break;
	}

#endif
}
#endif

#if 0
void SmoothingWave(short *InDataBuf,unsigned long lenth)
{
	unsigned long i = 0;//ָ���i����
	unsigned long NumberOfLow = 0;//С���Ȳ��ĸ���
	for(;i<lenth;)
	{   
		NumberOfLow = 0;
		while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //ȥ�� ���� 3��С��500�ĵ�
		{
			i++;
			NumberOfLow++;
			if(i == lenth)
			{
				return;
			}
		}
		if(NumberOfLow < 3)//�����м���ֵ�С���Ȳ�С��3��Ҫ�˻�ȥ
		{
			i -= NumberOfLow;
		}
		//InDataBuf[i] = (InDataBuf[i - 2] + InDataBuf[i - 1]*2 + InDataBuf[i]*4 + InDataBuf[i + 1]*2 + InDataBuf[i + 2])/10;
		InDataBuf[i] = (InDataBuf[i - 1] + InDataBuf[i] + InDataBuf[i + 1])/3;
		i++;		
	}
}
#endif
/*****************************************
����:   ȥ��  (ȥ�������й�С�ĸ��ţ�ʵ��Ҳ���˲�)
���������õĺ����嵥: ��
���ñ������ĺ����嵥: Demodulation
�������:  *InDataBuf    �Ӹõ�ַ��ʼ   
lenth  		 ����	
MobileType	 �Ը�������ȥ��
�������:   
��������ֵ˵��: ��
ʹ�õ���Դ 
******************************************/
void CFSKModem::DisInterference(short *InDataBuf,unsigned long lenth,BYTE MobileType)
{
	unsigned long i = 0;//ָ���i����
	unsigned long j = 0;//���ڳ�С������в���ֵȡ��
	unsigned long NumberOfLow = 0;//С���Ȳ��ĸ���
	float LengthOfZorePassage = 0;//�����֮����
	float LastRatio = 0;///�������һ�εı���
	float	RatioOfZorePassage = 0;//�������һ�εı���


	unsigned long datastart = 0;//��ǰ�ڽ�Ĳ��Ŀ�ʼ��

	for(;i<lenth;)
	{   
		NumberOfLow = 0;
		while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //ȥ�� ���� 3��С��500�ĵ�
		{
			i++;
			NumberOfLow++;
			if(i == lenth)
			{
				return;
			}
		}
		if(NumberOfLow < 3)//�����м���ֵ�С���Ȳ�С��3��Ҫ�˻�ȥ
		{
			i -= NumberOfLow;
		}

		datastart = i;//��ǰ���Ŀ�ʼ��
		LastRatio = RatioOfZorePassage;//������һ�εĹ�������
		if(InDataBuf[i] >= 0)//�������ֵ���ڵ���0
		{
			while(InDataBuf[i] >= 0)//ֱ������ֵС��0
			{
				i++;//��һ��������	
			}
		}
		else//�������ֵС��0
		{
			while(InDataBuf[i] < 0)//ֱ������ֵ����?
			{
				i++;//��һ��������				
			}
		}
		RatioOfZorePassage = (float(abs(InDataBuf[i]))) 
			/ ( float(abs(InDataBuf[i - 1])) + float(abs(InDataBuf[i])) );

		//���µ�ǰ�������֮��Ŀ��	
		LengthOfZorePassage =LastRatio  + (i - datastart - 1) + (1 - RatioOfZorePassage);
		if(LengthOfZorePassage <  ((float)MobileType+1.0)/3.0) //����ǳ�С������Ϊ�Ǹ���		
		{
			for(j = datastart;j < i;j++)
			{
				InDataBuf[j] = 0 - InDataBuf[j];
			}			
		}		

	}

}

/*****************************************
����:  ��� ��InDataBuf��ʼLenth ��ô�������� ���MobileType��ʽ����������ݣ�����OutDataBuf��
����ʱ���⵽�ĸ������OutLenIndix��
���������õĺ����嵥: ��
���ñ������ĺ����嵥: main
�������:  *InDataBuf    ����ֵ��ַ
lenth        �ܳ���
MmobileType  �ֻ�����
�������:  *OutDataBuf   ���ݱ���ĵط�
*OutLenIndix  �⵽����

��������ֵ˵��:    0:����1:û���˲�  2:��Ҫ�˲�
ʹ�õ���Դ 
******************************************/
int    CFSKModem::Demodulate(BYTE *OutDataBuf, short *InDataBuf,
									  unsigned long lenth,unsigned long *OutLenIndix,BYTE MobileType)
{
	BYTE LoopForSmooth = 0;// 0 �ǵ�һ�Σ�1�ǵڶ���
	BYTE DemodulationResult = 0;// ��ͬ��ͷ�ͽ���Ľ����1Ϊ�ɹ���0Ϊʧ��

	for(LoopForSmooth = 0;LoopForSmooth < 2; LoopForSmooth++ )
	{
		if(LoopForSmooth == 1)//����ѭ�����Ȳ��˲����ⲻ�������˲���
		{
			printf("start Smoothing wave\n");//
			unsigned long lLowF = 0;
			unsigned long lHighF = 0;
			lLowF = (unsigned long)((float)(2000*2/(MobileType+1))*(float)(1.0/32.0 * (float)(MobileType+1)+15.0/16.0));
			lHighF = (unsigned long)((float)(15000*2/(MobileType+1))*(float)(1.0/16.0 * (float)(MobileType+1)+7.0/8.0));

			memcpy(InDataBuf,(char*)InDataBuf+lenth*2,lenth*2);
			SmoothingWave(InDataBuf,lenth, lLowF, lHighF, 44100);
			//	SmoothingWave(InDataBuf,lenth);
			*OutLenIndix = 0;
		}
		this->DisInterference(InDataBuf,lenth,MobileType);//ȥ��
		DemodulationResult = this->FindHead(InDataBuf,lenth,OutLenIndix,MobileType);//��ͬ��ͷ
		if( DemodulationResult == 1)//����ҵ��ˣ����
		{
			DemodulationResult = this->GetAllData(OutDataBuf,InDataBuf,lenth,OutLenIndix,MobileType);//���
		}

		if(LoopForSmooth == 0)
		{
			if(DemodulationResult == 1)//continue;//��һ�νⲻ�������˲����ٽ�
			{
				printf("with no need for Smoothing wave\n");//
				return 1;//��һ�ξͽ�����ˣ�˵����û���˲��ͽ������
			}
		}
		else if(LoopForSmooth == 1)
		{
			if(DemodulationResult == 0)
			{
				return 0;//�ڶ��λ��ⲻ������������
			}
			else
			{
				printf("need Smoothing wave\n");//
				return 2;//�ڶ��βŽ������˵����Ҫ�˲�
			}
		}

	}
	return 0;//������
}
