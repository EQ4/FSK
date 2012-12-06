/************************************************************************/
/*   
FSKModem.h
xiaotanyu13
2012/11/12
xiaot.yu@sunyard.com

��װ��FSK���ƽ���ĺ�����������ķ�����ʵ��                                                                   */
/************************************************************************/

#ifndef FSKMODEM_H
#define FSKMODEM_H

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif

#define FS				44100			// ������

// ���͵�Ƶ�ʱ��̶����ˣ����Զ�ʹ�ú�����ʾ
#define MODULATE_FREQUENCY	5512.5		// ���Ƶ�Ƶ��
#define MODULATE_SAMPLE  (int)(FS / MODULATE_FREQUENCY)	// ����ʱ������ڵ���
#define AMP_U			1024*24			// ���� +
#define AMP_D			-1024*24		// ���� -
#define BYTE_LEN		8				// ÿ���ֽڵı�����
#define ZVALUEREF		0   //�ο���   ����Ƚ�
#define E_POINTS		128
#define E_SOUND_THRESHOLD	(1000*E_POINTS)
#define E_SILENCE_THRESHOD	(800*E_POINTS)
#define MAX_N_POINTS		(512*1024)  /* 2**14 */


class CFSKModem
{
private:
	// ����
	bool m_bIobitFlag;			// if true >0 else <0 ,�������ÿ�������������
	char m_cPackageCount;		// ����������ÿ�η��Ͷ���++

	float *m_fPoint;           /* pointer to time-domain samples */

protected:
	int ModulateByte(BYTE byte,short* retData);				// ���һ���ֽ�
	int ModulateBit(int type,short* retData);				// ��һ�����ص����ݵ��Ƴ���Ƶ����

	BYTE* PackField(char* data,int len,int *outLen);		// �鱨��
	int PackSYNC(BYTE* data);								// ���ͬ����
	int PackCount(BYTE* data);								// ��Ӽ�����
	int PackLength(int len,BYTE* data);						// ��ӳ�����
	int PackData(char* dataIn,int len,BYTE* data);			// ���������
	int PackCRC(short crc,BYTE* data);						// ���У����

	void DisInterference(short *InDataBuf,
		unsigned long lenth,BYTE MobileType);				// ȥ��
	void SmoothingWave(short *InDataBuf,unsigned long length, 
		unsigned long LowF, unsigned long HighF, 
		unsigned long SampleRate);							// �˲�
	BYTE GetAllData(BYTE *OutDataBuf,
		short *InDataBuf,unsigned long lenth,
		unsigned long *endlen,BYTE MobileType);				// ���ȫ������
	BYTE FindHead(short * InDataBuf,unsigned long lenth,
		unsigned long *endlen,BYTE MobileType);				// ����ͬ��ͷ
	int GetValidData(short *InDataBuf,short *OutDataBuf,
		unsigned long lenth);								// 
	unsigned long GetN(unsigned long len);					//
	void FindFrame(short *pData, 
		unsigned long len, long *start, long *end);			//

	short CalculateCRC(BYTE *buf,int len);					// ����CRC

public:
	CFSKModem();
	~CFSKModem();

public:
	short* Modulate(char* data,int len,int* outFrameLen);	// ��������  
	int    Demodulate(BYTE *OutDataBuf,
		short *InDataBuf,unsigned long lenth,
		unsigned long *OutLenIndix,BYTE MobileType);		// �������
};


/*
	��������wav�ļ���ͷ
*/
typedef struct _tagMsWavPcmHeader44{
	BYTE ChunkID[4]; // "RIFF"; The "RIFF" the mainchunk;
	unsigned long ChunkSize; // FileSize - 8; The size following this data
	BYTE Format[4]; // "WAVE"; The "WAVE" format consists of two subchunks: "fmt " and "data"

	BYTE SubChunk1ID[4]; // "fmt "
	unsigned long SubChunk1Size; // 16 for PCM. This is the size of the rest of the subchunk which follows this data.
	unsigned short AudioFormat; // 1 for PCM. Linear quantization
	unsigned short NumChannels; // 1->Mono, 2->stereo, etc..
	unsigned long SampleRate; // 8000, 11025, 16000, 44100, 48000, etc..
	unsigned long ByteRate; // = SampleRate * NumChannels * BitsPerSample/8
	unsigned short BlockAlign; // = NumChannels * BitsPerSample / 8
	unsigned short BitsPerSample; // 8->8bits, 16->16bits, etc..

	BYTE SubChunk2ID[4]; // "data"
	unsigned long SubChun2Size; // = NumSamples * NumChannels * BitsPerSample / 8. The size of data
} wav_pcm_header44;
#endif