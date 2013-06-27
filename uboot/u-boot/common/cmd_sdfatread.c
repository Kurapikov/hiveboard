#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <u-boot/zlib.h>
#include <bzlib.h>
#include <environment.h>
#include <lmb.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>



#define SHORT2INT(sht) sht[0]+sht[1]*0x100
#define LONG2INT(sht) sht[0]+sht[1]*0x100+sht[2]*0x10000+sht[3]*0x1000000


typedef unsigned char  U8;
typedef unsigned long  U32;     
typedef long  S32; 
void do_Read2mem(const char * filename,const char * memaddr);
long int stringnum2int(char *str);

extern void    ReadMultiSD(U32 sector_start, S32 nob, U32 ram_start);


struct _dpt{
	unsigned char boot_flg;
	unsigned char phy_sec_start[3];
	unsigned char filetype;
	unsigned char phy_sec_end[3];
	unsigned char logical_sec[4];
	unsigned char  sec_cnt[4];
};
struct _mbr_sec{
	unsigned char code[446];
	struct _dpt dpt[4];
	unsigned char  tag[2];
};
struct _dbr_fat16{
	unsigned char jmpCode[3];
	unsigned char oemId[8];
	unsigned char bytesPerSector[2];
	unsigned char sectorsPerCluster;
	unsigned char reservedSector[2];
	unsigned char fatCnt;
	unsigned char rootEntries[2];
	unsigned char smallSector[2];
	unsigned char media;
	unsigned char sectorsPerFat[2];
	unsigned char sectorsPerTrack[2];
	unsigned char heads[2];
	unsigned char hiddenSectors[4];
	unsigned char largeSectors[4];
	unsigned char driveNo;
	unsigned char reserved;
	unsigned char extBootSignature;
	unsigned char volSerialNo[2];
	unsigned char label[11];
	unsigned char fileSysType[8];
	unsigned char bootCode[448];
	unsigned char tag[4];
};
struct _fdt_fat16{
	unsigned char majorname[8];
	unsigned char minorname[3];
	unsigned char attribute;
	unsigned char reserved[10];
	unsigned char time[2];
	unsigned char date[2];
	unsigned char cluNo[2];
	unsigned char fSize[4];
};
typedef struct _dpt DPT;
typedef struct _mbr_sec MBR_SEC;
typedef struct _dbr_fat16 DBR_FAT16;
typedef struct _fdt_fat16 FDT;

int do_sdfatread (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]){
	//printf��ӡ������
	do_Read2mem(argv[1],argv[2]);
	

}


long int stringnum2int(char *str){
	long int i,j;
	char valuebox[10];
	int value;
	if(str[0]!='0'&&(str[1]!='x'||str[1]!='X')){
		printf("please input add like 0x.....\n");
		return 0;
	}
	i=2;
	value=0;
	while((str[i]>='0'&&str[i]<='9')||(str[i]>='a'&&str[i]<='f')||str[i]>='A'&&str[i]<='F'){
		if(str[i]>='0'&&str[i]<='9'){
			value=value*0x10+str[i]-'0';
		}
		if(str[i]>='a'&&str[i]<='f'){
			value=value*0x10+str[i]-'a'+10;
		}
		if(str[i]>='A'&&str[i]<='F'){
			value=value*0x10+str[i]-'A'+10;
		}
		i++;
	}
	return value;
}

void do_Read2mem(const char * filename,const char * memaddr){
	int i;
	DPT * dpt0;
	printf("I' in do_Read2mem");
	
	/*********����dpt0������******************/
	ReadMultiSD(0,1,0x40A00000);
	dpt0=(DPT *)(0x40A001BE);
	//dpt0 file system ���ʹ���
	unsigned char fs_type=dpt0->filetype;
	//logic sectors dbr��offset
	unsigned char * tmp_c=dpt0->logical_sec;
	long int logic_sec=LONG2INT(tmp_c);

	/************��dbr�л����Ϣ*****************/
	//��ȡdbr����
	ReadMultiSD(logic_sec,1,0x40A00000);
	DBR_FAT16 *dbr=(DBR_FAT16 *)(0x40A00000);
	//ÿ�����������ֽ�
	tmp_c=dbr->bytesPerSector;
	long int bPerSec=SHORT2INT(tmp_c);
	//ÿ���ض��ٸ�����
	long int secPerClu=dbr->sectorsPerCluster;
	//��������
	tmp_c=dbr->reservedSector;
	long int resSec=SHORT2INT(tmp_c);
	//fat����
	long int fatCnt=dbr->fatCnt;
	//��Ŀ¼����
	tmp_c=dbr->rootEntries;
	long int rootEntrs=SHORT2INT(tmp_c);
	//С������
	tmp_c=dbr->smallSector;
	long int smallSec=SHORT2INT(tmp_c);
	//ÿfat������
	tmp_c=dbr->sectorsPerFat;
	long int secPerFat=SHORT2INT(tmp_c);
	//��������������ͬ�ڱ���������
	tmp_c=dbr->hiddenSectors;
	long int hiddenSec=LONG2INT(tmp_c);
	//��������
	tmp_c=dbr->largeSectors;
	long int largeSec=LONG2INT(tmp_c);

	/********����fatƫ���������Լ���Ŀ¼ƫ������*************/
	long int fatBase[fatCnt];
	//��һ��fat��ͷ��ƫ������
	fatBase[0]=logic_sec+resSec;
	//����fat��ͷ��ƫ������
	int m=1;
	for(m=1;m<fatCnt;m++){
		fatBase[m]=fatBase[m-1]+secPerFat;
	}
	//��Ŀ¼ƫ������
	long int rootDir=fatBase[0]+fatCnt*secPerFat;
	//data��ƫ������
	long int dataBase=rootDir+32;

	/******���ļ���Ϊfilename���ļ����Ƶ��ڴ���***********/
	//���ļ�������չ�ļ�����ȡ(���Դ�����ļ��������ļ����в�Ҫ�з���/���ֵ�)
	char majorname[8];
	char minorname[3];
	int j=0;
	while(filename[j]!='.'&& filename[j] != '\0'){
		if(filename[j]>='a'&&filename[j]<='z')
			majorname[j]=filename[j]-'a'+'A';
		else
		majorname[j]=filename[j];
		j++;
	}
	if (filename[j] == '\0') //����չ��
		m = j;
	else   //filename[j] ==��.�� ����չ��
	m=j+1;
	for(;j<8;j++)
		majorname[j]=0x20;

	j=0;
	while(filename[m]!='\0'){
		if(filename[m]>='a'&&filename[m]<='z')
			minorname[j]=filename[m]-'a'+'A';
		else
		minorname[j]=filename[m];
		m++;
		j++;
	}
	for(;j<3;j++)
		minorname[j]=0x20;

	//�ڸ�Ŀ¼�в���filename�ļ�
	ReadMultiSD(rootDir,32,0x40200000);//rootEntrs*32/bPerSec+1
	ReadMultiSD(fatBase[0],secPerFat,0x40B00000);
	FDT *fdt=(FDT *)(0x40200000);
	FDT *tmp_fdt;
	unsigned char *fat16=(unsigned char*)(0x40B00000);
	long int addr=stringnum2int(memaddr);
	long int secCnt;
	long int firstClu;
	long int cluNo;
	long int offsetSec;
	long int size;
	unsigned char * tmpFatBase;
	firstClu=0;
	size=0;
	for(i=0;i<rootEntrs;i++){
	char *tmp=(char *)(0x40A00000);
		j=0;
		tmp_fdt=&fdt[i];
		while(majorname[j]==tmp_fdt->majorname[j]&&j<8){
			j++;
		}
		if(j==8){
			j=0;

			while(minorname[j]==tmp_fdt->minorname[j]&&j<3){
				j++;
			}
			if(j==3) j=10;
		}
		if(j==10){
			tmp_c=tmp_fdt->cluNo;
			firstClu=SHORT2INT(tmp_c);
			tmp_c=tmp_fdt->fSize;
			size=LONG2INT(tmp_c);
			break;
		}
	}
	if(size==0) printf("can't find the file\n");
	//����ҵ��ļ����������ڴ���
	else{
		cluNo=firstClu;
		while(cluNo!=0xffff){
			offsetSec=dataBase+(cluNo-2)*secPerClu;
			//printf("offset sectors is 0x%x\n",offsetSec);
			ReadMultiSD(offsetSec,secPerClu,addr);

			addr+=secPerClu*bPerSec;
			tmp_c=&fat16[cluNo*2];
			cluNo=SHORT2INT(tmp_c);
		}
		
	}
}



U_BOOT_CMD(sdfatread,3,1,do_sdfatread,"sdfatread filename memaddr","read file from fat16 of sd to mem");

