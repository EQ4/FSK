/*
	xiaotanyu13
*/
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include "FSKModem.h"
using namespace std;


void TestModulate()
{
	CFSKModem *fsk = new CFSKModem();
	char a[] = {0x01,0x54,0x33,0x45,0x55};
	int len = 5;
	int outLen = 0;
	short* ss = NULL;
	ss = fsk->Modulate(a,len,&outLen);

	for(int i = 0; i < outLen; i ++)
	{
		if(i % MODULATE_SAMPLE == 0)
			printf("\n");
		printf("%d ",ss[i]);
	}
	free(ss);
}


/*****************************************
    ����:���ļ� ���Դ�ӡ����
    ���������õĺ����嵥: ��
    ���ñ������ĺ����嵥: main
    �������:  *DataBuf   
    �������:  ���ݳ���
    ��������ֵ˵��:   
    ʹ�õ���Դ 
******************************************/
unsigned long GetHeadData(void)
{
	wav_pcm_header44 hwav;//���ڴ��WAV���ļ�ͷ
	FILE *fpWav = fopen("123.wav", "rb");
	fread(&hwav, sizeof(wav_pcm_header44), 1,fpWav);

	if ( (0==memcmp(hwav.ChunkID, "RIFF", 4)) &&(0==memcmp(hwav.Format, "WAVE", 4)) &&
			(0==memcmp(hwav.SubChunk1ID, "fmt ", 4))  &&(1==hwav.AudioFormat)&&(0==memcmp(hwav.SubChunk2ID, "data", 4))
	    )
		{
			printf("Wave audio data format:\n");
			printf("Channel number: %d\n", hwav.NumChannels);
			printf("SampleRate: %dHz\n", hwav.SampleRate);
			printf("BitsPerSample: %dbits\n", hwav.BitsPerSample);
			printf("Audio data size:%d\n\n\n\n", hwav.SubChun2Size);
			return hwav.SubChun2Size;
		}
	else
		{
			printf("not WAV\n" );	
			printf("please entry any key\n");
			char c = getchar();
			putchar(c);
			return 0;
		
		}
}

int  TestDemodulate()
{
	CFSKModem *fsk = new CFSKModem();
	char a[] = {0x01,0x54,0x33,0x45,0x55};
	int len = 5;
	int outLen = 0;
	short* ss = NULL;
	ss = fsk->Modulate(a,len,&outLen);

	unsigned long LenthOfWAV = 0;//WAV�ļ��ĳ���
	unsigned long OutLenIndix;//�⵽�ĵ���

	LenthOfWAV = GetHeadData(); //��ӡ�ļ����ԣ�����ȡ�ļ�����
	if(LenthOfWAV == 0)
	{
		printf("no WAV");//û��wav�ļ�
		return 0;//���û����ȷ���ݣ��˳�
	}

	unsigned char retdatabuf[2000];//���ڴ�Ž����������,���1.5k

	short *databuf = (short*)malloc((LenthOfWAV+44)*2);//���ݳ��ȣ������ڴ�
	FILE *logfile = fopen("123.wav","rb");
	fread(databuf,1,LenthOfWAV+44,logfile);
	memcpy( ((char*)databuf+LenthOfWAV+44),databuf,(LenthOfWAV+44));
	//	int leee = GetValidData(databuf+22,(short *)tempbuf1,LenthOfWAV/2);
	//fsk->Demodulate(retdatabuf, databuf+22,LenthOfWAV/2,&OutLenIndix,1);
	fsk->Demodulate(retdatabuf, ss,outLen,&OutLenIndix,3);

	/**********************************************/
	//�����˲���Ĳ���
	FILE* file=fopen("234.wav","wb");
	if(file)
		//		fwrite(tempbuf1,leee*2,1,file);//
			fwrite(databuf,LenthOfWAV+44,1,file);
	else
		printf("open 234.wav fail\n");
	/**********************************************/

	if(logfile)
		fclose(logfile);
	if(file)
		fclose(file);
	if(databuf)
		free(databuf);
	printf("please entry any key\n");
	return 0;
}


int main()
{
	TestModulate();
	
	//CFSKModem *fsk = new CFSKModem();

	//TestDemodulate();
	
	return 0;
}