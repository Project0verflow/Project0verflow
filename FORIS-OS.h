#pragma once
#include <vector>
#include "GameBase.h"
class FORIS_OS: public GameBase
{
public:
	struct entry {
		uint32_t offset;
		std::wstring name;
		bool isPacked;
		uint32_t size;
	};

	FORIS_OS(std::string name, int keyID, int footerID, bool compression=true);

	void BlockCopy(unsigned char * src, int srcOffset, unsigned char * dst, int dstOffset, int count);
	void CopyOverlapped(unsigned char * data, int src, int dst, int count);//copy data[dst] into data[src] for count number of times
	std::vector<unsigned char> * LZPack(unsigned char * data, int length);
	unsigned char * LazyLZPack(unsigned char * data, int length, int outputLength);
	std::vector<unsigned char> * ExtraLazyLZPack(unsigned char * data, int length, int outputLength);
	void LzUnpack(unsigned char * input, unsigned char * output, int length, int inputIndex = 0);
	uint64_t readLittleEndian(unsigned char* array, int startPos, int length);//max len is 4
	std::wstring getUTF8String(unsigned char * data, int offset, int length);
	std::vector<entry> * getEntries(unsigned char* uncompressedIndexData, int uncompressedIndexSize);
	std::vector<unsigned char> * serializeEntries(std::vector<entry>* toRead, uint32_t indexPos = 0);

	// Inherited via GameBase
	bool extract(std::ifstream *file, std::string outPutFolder) override;
	bool pack(std::ofstream *output, std::string scriptFolder) override;
	bool respaceArchive(std::ifstream *file) override;
	bool respaceFolder(std::string outPutFolder) override;
	bool generateScript(std::ofstream *outputFile, std::string scriptDirectory) override;
	std::string gameName() override;
private:
	unsigned char *key;
	unsigned char keys[3][16]{
		{0x6C, 0x14, 0xF2,0x03,0xE3,0x62,0x32,0xAC,0x03,0x04,0xAC,0xF2,0xD3,0x84,0xF8,0xCA},
		{0x0f,0x25,0x3e,0x5c,0x2a,0x4b,0x77,0x90,0x05,0x8a,0x8e,0x46,0xeb,0x3d,0x11,0x43 }, //Summer Radish Vacation 1.1
		//{0x67,0xD8,0x16,0x28,0x21,0x05,0x50,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCE } //Lost M
		//{0x67,0xD8,0x16,0x28,0x21,0x05,0x50,0x0A,0x6C,0xC3,0x89,0x6A,0xde,0x5C,0x00,0xCE } //Lost M
		{0x67,0xD8,0x16,0x28,0x21,0x05,0x50,0x0A,0x6C,0xC3,0x89,0x71,0xDB,0x5C,0x03,0xCE } //Lost M
		//1A is probably higher that 80, there might be multiple characters being pulled
		//First 8 are 100%
	};
	unsigned char *footer;
	unsigned char footers[4][4]{
		{0x50, 0x41, 0x43, 0x4B},
		{0x5f, 0x64, 0x7d, 0x17},
		{0x3C, 0x55, 0xB1, 0x48},
		{0x37, 0x99, 0x55, 0x63} //Lost M
	};
	std::string name;
	bool doCompression;
};