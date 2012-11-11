#ifndef __AESH__
#define __AESH__


#define calcKey
#define DECODE
//#define ENCODE

//void AESEncode(unsigned char* block, unsigned char* key);
void AESDecode(unsigned char* block, unsigned char* key);
void AESCalcDecodeKey(unsigned char* key);
	 

#endif // __AESH__
