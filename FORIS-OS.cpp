#include "FORIS-OS.h"
#include <alg.h>
#include <iostream>
#include <fstream>
#include <filesystem>

FORIS_OS::FORIS_OS(std::string n, int keyID, int footerID, bool compression)
{
	name = std::move(n);
	if (keyID < 0)
		key = nullptr;
	else
		key = keys[keyID];
	footer = footers[footerID];
	doCompression = compression;
}

void FORIS_OS::BlockCopy(unsigned char * src, int srcOffset, unsigned char * dst, int dstOffset, int count)
{
	for (int i = 0; i < count; i++)
	{
		dst[i + dstOffset] = src[i + srcOffset];
	}
}

void FORIS_OS::CopyOverlapped(unsigned char * data, int src, int dst, int count)//copy data[dst] into data[src] for count number of times
{
	if (dst > src)
	{
		while (count > 0)
		{
			int preceding = min(dst - src, count);
			FORIS_OS::BlockCopy(data, src, data, dst, preceding);
			dst += preceding;
			count -= preceding;
		}
	}
	else
	{
		FORIS_OS::BlockCopy(data, src, data, dst, count);
	}
}

std::vector<unsigned char> * FORIS_OS::LZPack(unsigned char * data, int length)
{
	std::vector<unsigned char> * toReturn = new std::vector<unsigned char>();

	uint64_t currentPacketIndex = 0;
	uint16_t currentUncompressedPacketSize = 0;
	uint16_t currentMatchSize = 0;
	uint32_t temp = length;

	for (int i = 0; i < 4; i++)
	{
		toReturn->push_back(temp);
		temp = temp >> 8;
	}
	for (int index = 0; index < length; index++)
	{
		for (int backtrackIndex = 1; index - backtrackIndex >= 0 && backtrackIndex < 0x7ff; backtrackIndex++)//do real math later to make sure it doesn't go past the max offset val
		{
			if (data[index - backtrackIndex] == data[index])
			{
				for (int backtrackIndexSecondary = 1; index - backtrackIndex + backtrackIndexSecondary < index; backtrackIndexSecondary++)
				{
					if (data[index - backtrackIndex + backtrackIndexSecondary] != data[index + backtrackIndexSecondary] || backtrackIndexSecondary > 31)//might be possible to bump this to 62
						break;
					if (backtrackIndexSecondary % 2 == 1 && currentMatchSize < backtrackIndexSecondary)
					{
						if (currentUncompressedPacketSize != 0)
						{
							toReturn->at(currentPacketIndex) = currentUncompressedPacketSize - 1;
							currentUncompressedPacketSize = 0;
						}
						currentPacketIndex = backtrackIndex - 1;//lzunpack lists size-offset-1, so this should work
						currentMatchSize = backtrackIndexSecondary + 1;
					}
				}
			}
		}
		if (currentMatchSize != 0)
		{
			uint16_t packetData = 0x8000;//sets the flag
			packetData += currentPacketIndex;
			packetData += (currentMatchSize - 2) << 10;//some issues with high values being cut off
			toReturn->push_back(packetData >> 8);
			toReturn->push_back((uint8_t)packetData);
			//make the thingy
			index += currentMatchSize - 1;//because the loop will move it forward
			currentMatchSize = 0;
			currentPacketIndex = 0;
		}
		else
		{
			if (currentUncompressedPacketSize & 128 == 1)//if packet is so big it triggers lz flag, close out the current packet and get ready for next
			{
				toReturn->at(currentPacketIndex) = currentUncompressedPacketSize - 1;
				currentUncompressedPacketSize = 0;
			}
			if (currentUncompressedPacketSize == 0)//if start of new packet push the data
			{
				currentPacketIndex = toReturn->size();
				toReturn->push_back(0);
			}
			toReturn->push_back(data[index]);
			//currentMatchSize++;
			currentUncompressedPacketSize++;
		}
	}
	if (currentUncompressedPacketSize != 0)
	{
		toReturn->at(currentPacketIndex) = currentUncompressedPacketSize - 1;
	}

	return toReturn;

}

unsigned char * FORIS_OS::LazyLZPack(unsigned char * data, int length, int outputLength)
{
	unsigned char* toReturn = new unsigned char[outputLength];
	int returnIndex = 0;
	int temp = length;
	for (; returnIndex < 4; returnIndex++)
	{
		toReturn[returnIndex] = temp;
		temp = temp << 8;
	}
	int inputIndex = 0;
	while (inputIndex <= length - 121)
	{
		toReturn[returnIndex++] = 0x79;
		for (int i = 0; i <= 121; i++)
		{
			toReturn[returnIndex++] = data[inputIndex + i];
		}
		toReturn[returnIndex++] = 0x80;
		toReturn[returnIndex++] = 0x00;
		inputIndex += 122;
	}
	int toFinish = length - inputIndex;
	toReturn[returnIndex++] = toFinish;
	for (int i = 0; i < toFinish; i++)
	{
		toReturn[returnIndex++] = data[inputIndex + i];
	}
	return toReturn;
}

std::vector<unsigned char> * FORIS_OS::ExtraLazyLZPack(unsigned char * data, int length, int outputLength)
{
	std::vector<unsigned char> * tempVec = new std::vector<unsigned char>();
	unsigned char* toReturn = new unsigned char[outputLength];
	int returnIndex = 0;
	int temp = length;
	/*for (; returnIndex < 4; returnIndex++)
	{
		toReturn[returnIndex] = temp;
		temp = temp << 8;
	}*/
	int inputIndex = 0;
	while (inputIndex <= length - 121)
	{
		tempVec->push_back(0x79);
		for (int i = 0; i <= 121; i++)
		{
			tempVec->push_back(data[inputIndex + i]);
		}
		inputIndex += 122;
	}
	int toFinish = length - inputIndex;
	tempVec->push_back(toFinish);
	for (int i = 0; i < toFinish; i++)
	{
		tempVec->push_back(data[inputIndex + i]);
	}
	return tempVec;
}

void FORIS_OS::LzUnpack(unsigned char * input, unsigned char * output, int length, int inputIndex)
{
	int outputIndex = 0;
	while (outputIndex < length)
	{
		char current = input[inputIndex++];
		if ((current & 128) != 0)
		{
			uint16_t num = (uint16_t)current;
			num = num << 8;
			num += input[inputIndex++];
			//num += (current << 8);
			int offset = num & 0x7ff;//removes the 128 bit
			int count = min(((num >> 10) & 0x1E) + 2, length - outputIndex);//3-2 or end of array 
			//>> 10 gives 0011 1111
			//1E gives    0001 1110
			//functionaly 00[1]. ..10
			CopyOverlapped(output, outputIndex - offset - 1, outputIndex, count);

			outputIndex += count;
		}
		else
		{
			int count = min(current + 1, length - outputIndex);
			for (int i = 0; i < count; i++)//length of non-compressed stream is disreguarded, so step forward one
			{
				output[outputIndex++] = input[inputIndex + i];
			}
			inputIndex += count;
		}
	}

}

uint64_t FORIS_OS::readLittleEndian(unsigned char* array, int startPos, int length)//max len is 4
{
	length--;
	uint64_t total = 0;
	while (length >= 0)
	{
		total = total << 8;
		total += array[startPos + length--];
	}
	return total;
}

std::wstring FORIS_OS::getUTF8String(unsigned char * data, int offset, int length)
{
	std::wstring toReturn;
	for (int i = 0; i < length; i += 2)
	{
		wchar_t current = FORIS_OS::readLittleEndian(data, offset + i, 2);
		toReturn += current;
	}
	return toReturn;
}

std::vector<FORIS_OS::entry> * FORIS_OS::getEntries(unsigned char* uncompressedIndexData, int uncompressedIndexSize)
{
	std::vector<entry> * entries = new std::vector<entry>();
	int uncompressedIndexPos = 0;
	uncompressedIndexSize -= 8;
	uint32_t offset = readLittleEndian(uncompressedIndexData, uncompressedIndexPos, 4);
	uncompressedIndexPos += 4;
	while (uncompressedIndexPos < uncompressedIndexSize)
	{
		uint8_t nameLength = uncompressedIndexData[uncompressedIndexPos];
		if (nameLength == 0)
			break;
		bool isPacked = uncompressedIndexData[uncompressedIndexPos + 1] != 0;
		uncompressedIndexPos += 6;
		nameLength *= 2;
		std::wstring name = getUTF8String(uncompressedIndexData, uncompressedIndexPos, nameLength);//get name

		uncompressedIndexPos += nameLength;
		uint32_t nextOffset = readLittleEndian(uncompressedIndexData, uncompressedIndexPos, 4);
		entries->push_back({ offset, name, isPacked, nextOffset-offset });
		offset = nextOffset;

		uncompressedIndexPos += 4;
	}
	return entries;
}

std::vector<unsigned char> * FORIS_OS::serializeEntries(std::vector<entry>* toRead, uint32_t indexPos)
{
	std::vector<unsigned char> * toReturn = new std::vector<unsigned char>();

	for (entry e : *toRead)
	{
		uint32_t offset = e.offset;
		for (int i = 0; i < 4; i++)
		{
			toReturn->push_back((unsigned char)offset);
			offset = offset >> 8;
		}
		toReturn->push_back((unsigned char)((uint8_t)e.name.length()));
		toReturn->push_back(e.isPacked);
		for (int i = 0; i < 3; i++)
		{
			toReturn->push_back((unsigned char)0x00);
		}
		toReturn->push_back((unsigned char)0x02);
		for (wchar_t ch : e.name)
		{
			toReturn->push_back(ch);
			toReturn->push_back(ch >> 8);
		}
	}
	for (int i = 0; i < 4; i++)
	{
		toReturn->push_back((unsigned char)indexPos);
		indexPos = indexPos >> 8;
	}
	for (int i = 0; i < 4; i++)
	{
		toReturn->push_back('\0');
	}
	return toReturn;
}

bool FORIS_OS::extract(std::ifstream *file, std::string outPutFolder)
{
	std::streamsize size = file->tellg();
	file->seekg(0, std::ios::beg);
	unsigned char* data = new unsigned char[size];
	file->read((char*)data, size);//load file in

	uint64_t indexOffset = readLittleEndian(data, size - 4, 4);
	uint64_t uncompressedIndexSize = readLittleEndian(data, indexOffset, 4);//find index data
	uint64_t compressedIndexSize = size - indexOffset - 4;
	file->seekg(indexOffset + 4, std::ios::beg); //step past size
	unsigned char* compressedIndexData = new unsigned  char[compressedIndexSize];
	unsigned char* uncompressedIndexData = new unsigned  char[uncompressedIndexSize];
	file->read((char*)compressedIndexData, compressedIndexSize);//load index in
	if (key != nullptr)
	{
		int keyLength = 16;
		int keyStartPos = 0;//may be nessecary later on but is fine for now
		for (int i = 0; i < compressedIndexSize; i++)
		{
			compressedIndexData[i] ^= key[(keyStartPos + i) % keyLength];
		}
	}
	LzUnpack(compressedIndexData, uncompressedIndexData, uncompressedIndexSize);

	std::vector<entry> * entries = getEntries(uncompressedIndexData, uncompressedIndexSize);


	std::filesystem::create_directory(outPutFolder);
	std::string originalOutputPath = outPutFolder;
	int asize = 0;
	for (int i = 0; outPutFolder[i] != '\0'; i++)
		asize++;
	originalOutputPath += "\\";
	asize++;
	std::wstring outputPath;
	for (int i = 0; i < asize; i++)
		outputPath += originalOutputPath[i];
	for (entry e : *entries)//add support for unpacked entries, needs entry size
	{
		std::ofstream out(outputPath + e.name, std::fstream::binary);
		file->seekg(e.offset, std::ios::beg);
		if (e.isPacked)
		{
			int currentFileSize = readLittleEndian(data, e.offset, 4);//fix //?
			unsigned char * unPacked = new unsigned char[currentFileSize];
			LzUnpack(data, unPacked, currentFileSize, e.offset + 4);
			for (int i = 0; i < currentFileSize; i++)
			{
				out << unPacked[i];
				out.flush();
			}
			delete unPacked;
		}
		else
		{
			for (int i = 0; i < e.size; i++)
			{
				out << data[e.offset + i];
				out.flush();
			}
		}
		out.close();
		std::ifstream check(outputPath + e.name);//this will always pass dumbo
		if (check)
			std::wcout << e.name << " extracted" << std::endl;
		else
			std::wcout << "ERROR EXTRACTING " << e.name << std::endl;
	}
	/*std::ofstream out((std::string)outPutFolder + (std::string)"//keyfile.bin", std::fstream::binary);
	for (char c : key)
	{
		out << c;
		out.flush();
	}
	out.close();*/
	file->close();
}

bool FORIS_OS::pack(std::ofstream *output, std::string scriptFolder)
{
	uint32_t offsetCounter = 0;
	std::vector<entry> entries;
	for (const auto & currentFile : std::filesystem::directory_iterator(scriptFolder))
	{
		if (currentFile.is_regular_file() && std::filesystem::path(currentFile).filename() != "keyfile.bin")
		{
			entries.push_back({ (uint32_t)offsetCounter, std::filesystem::path(currentFile).filename(), true });
			std::ifstream current(currentFile.path(), std::ifstream::ate | std::ifstream::binary);
			std::streamsize size = current.tellg();
			current.seekg(0, std::ios::beg);
			unsigned char* data = new unsigned char[size];
			current.read((char*)data, size);//load file in
			std::vector<unsigned char> * packedData;
			if (doCompression)
				packedData = LZPack(data, size);
			else
				packedData = new std::vector<unsigned char>(data, data + size);
			for (int i = 0; i < packedData->size(); i++)
			{
				*output << packedData->at(i);
				output->flush();
				offsetCounter++;
			}
			current.close();
			delete packedData;
			delete data;

		}
	}
	std::vector<unsigned char> * entriesData = serializeEntries(&entries, offsetCounter);
	std::vector<unsigned char> * packedEntries = LZPack(&(*entriesData)[0], entriesData->size());
	if (key != nullptr)
	{
		int keyStartPos = 0;
		int keyLength = 16;
		for (int i = 0; i < packedEntries->size() - 4; i++)
		{
			packedEntries->at(i + 4) ^= key[(keyStartPos + i) % keyLength];
		}
	}
	for (unsigned char ch : *packedEntries)
	{
		*output << ch;
		output->flush();
	}
	for (int i = 0; i < 4; i++)
	{
		*output << footer[i];
		output->flush();
	}

	for (int i = 0; i < 4; i++)
	{
		*output << (char)offsetCounter;
		offsetCounter = offsetCounter >> 8;
		output->flush();
	}
	output->close();
	delete entriesData;
	delete packedEntries;
}

bool FORIS_OS::respaceArchive(std::ifstream *file)
{
	return false;
}

bool FORIS_OS::respaceFolder(std::string outPutFolder)
{
	return false;
}

bool FORIS_OS::generateScript(std::ofstream *outputFile, std::string scriptDirectory)
{
	int totalLines = 0;
	for (const auto & currentFile : std::filesystem::directory_iterator(scriptDirectory))
	{
		if (currentFile.is_regular_file() && std::filesystem::path(currentFile).filename() != "keyfile.bin")
		{
			std::ifstream reading(currentFile);
			std::string currentLine;
			bool isInresource = false;
			while (getline(reading, currentLine))
			{
				if (currentLine.find("<RESOURCE no=") != std::string::npos)
					isInresource = true;
				else if (currentLine.find("</RESOURCE") != std::string::npos)
					isInresource = false;
				else if (currentLine.find("<CREATE type=\"STR\">") != std::string::npos && isInresource)
				{
					size_t start = currentLine.find(">") + 1;
					size_t end = currentLine.find("</");
					std::string s = currentLine.substr(start, end - start);
					*outputFile << s << std::endl;
					totalLines++;
					isInresource = false;
				}
			}
			reading.close();
		}
	}
	outputFile->close();
	std::cout << "Total lines in game is: " << totalLines << std::endl;
}

std::string FORIS_OS::gameName()
{
	return name;
}
