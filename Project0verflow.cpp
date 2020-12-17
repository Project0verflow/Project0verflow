//To do:
//fix LZPack limits offset should be 0x7ff, size 62

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>//min
#include <vector>
#include <conio.h>//getch
#include <filesystem>

using namespace std;

struct entry {
	uint32_t offset;
	wstring name;
	bool isPacked;
};


void BlockCopy(unsigned char * src, int srcOffset, unsigned char * dst, int dstOffset, int count)
{
	for (int i = 0; i < count; i++)
	{
		dst[i + dstOffset] = src[i + srcOffset];
	}
}

void CopyOverlapped(unsigned char * data, int src, int dst, int count)//copy data[dst] into data[src] for count number of times
{
	if (dst > src)
	{
		while (count > 0)
		{
			int preceding = min(dst - src, count);
			BlockCopy(data, src, data, dst, preceding);
			dst += preceding;
			count -= preceding;
		}
	}
	else
	{
		BlockCopy(data, src, data, dst, count);
	}
}

vector<unsigned char> * LZPack(unsigned char * data, int length)
{
	vector<unsigned char> * toReturn = new vector<unsigned char>();
	
	uint16_t currentPacketIndex = 0;
	uint16_t currentUncompressedPacketSize = 0;
	uint16_t currentMatchSize = 0;
	uint32_t temp=length;

	for (int i=0; i< 4; i++)
	{
		toReturn->push_back(temp);
		temp = temp >> 8;
	}
	for (int index = 0; index < length; index++)
	{
		for (int backtrackIndex = 1;  index - backtrackIndex >= 0 && backtrackIndex<0xff; backtrackIndex++)//do real math later to make sure it doesn't go past the max offset val
		{
			if (data[index - backtrackIndex] == data[index])
			{
				for (int backtrackIndexSecondary = 1; index - backtrackIndex + backtrackIndexSecondary < index; backtrackIndexSecondary++)
				{
					if (data[index - backtrackIndex + backtrackIndexSecondary] != data[index + backtrackIndexSecondary]||backtrackIndexSecondary>30)//might be possible to bump this to 62
						break;
					if (backtrackIndexSecondary % 2 == 1 && currentMatchSize<backtrackIndexSecondary)
					{
						if (currentUncompressedPacketSize != 0)
						{
							toReturn->at(currentPacketIndex) = currentUncompressedPacketSize-1;
							currentUncompressedPacketSize = 0;
						}
						currentPacketIndex = backtrackIndex-1;//lzunpack lists size-offset-1, so this should work
						currentMatchSize = backtrackIndexSecondary+1;
					}
				}
			}
		}
		if (currentMatchSize != 0)
		{
			uint16_t packetData = 0x8000;//sets the flag
			packetData += currentPacketIndex;
			packetData += (currentMatchSize-2) << 10;//some issues with high values being cut off
			toReturn->push_back(packetData>>8);
			toReturn->push_back((uint8_t)packetData);
			//make the thingy
			index += currentMatchSize-1;//because the loop will move it forward
			currentMatchSize = 0;
			currentPacketIndex = 0;
		}
		else
		{
			if (currentUncompressedPacketSize & 128 == 1)//if packet is so big it triggers lz flag, close out the current packet and get ready for next
			{
				toReturn->at(currentPacketIndex) = currentUncompressedPacketSize-1;
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
		toReturn->at(currentPacketIndex) = currentUncompressedPacketSize-1;
	}
	
	return toReturn;

}

unsigned char * LazyLZPack(unsigned char * data, int length, int outputLength)
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
		toReturn[returnIndex++] = data[inputIndex+i];
	}
	return toReturn;
}

vector<unsigned char> * ExtraLazyLZPack(unsigned char * data, int length, int outputLength)
{
	vector<unsigned char> * tempVec = new vector<unsigned char>();
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
		tempVec->push_back(data[inputIndex+i]);
	}
	return tempVec;
}

void LzUnpack(unsigned char * input, unsigned char * output, int length, int inputIndex = 0)
{
	int outputIndex = 0;
	while (outputIndex < length)
	{
		char current = input[inputIndex++];
		if ((current & 128) !=0 )//if >= 0x80
		{
			uint16_t num = (uint16_t) current;
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

uint64_t readLittleEndian(unsigned char* array, int startPos, int length)//max len is 4
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

wstring getUTF8String(unsigned char * data, int offset, int length)
{
	wstring toReturn;
	for (int i = 0; i < length; i += 2)
	{
		wchar_t current = readLittleEndian(data, offset + i, 2);
		toReturn += current;
	}
	return toReturn;
}

vector<entry> * getEntries(unsigned char* uncompressedIndexData, int uncompressedIndexSize)
{
	vector<entry> * entries = new vector<entry>();
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
		wstring name = getUTF8String(uncompressedIndexData, uncompressedIndexPos, nameLength);//get name
		entries->push_back({ offset, name, isPacked });

		uncompressedIndexPos += nameLength;
		offset = readLittleEndian(uncompressedIndexData, uncompressedIndexPos, 4);
		uncompressedIndexPos += 4;
	}
	return entries;
}

vector<unsigned char> * serializeEntries(vector<entry>* toRead, uint32_t indexPos=0)
{
	vector<unsigned char> * toReturn = new vector<unsigned char>();

	for (entry e : *toRead)
	{
		uint32_t offset = e.offset;
		for (int i = 0; i < 4; i++)
		{
			toReturn->push_back((unsigned char)offset);
			offset = offset >> 8;
		}
		toReturn->push_back((unsigned char)((uint8_t) e.name.length()));
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

//exe -p folder name
//exe -e file folder
int main(int argc, char *argv[])
{
	unsigned char key[]{ 0x0f,0x25,0x3e,0x5c,0x2a,0x4b,0x77,0x90,0x05,0x8a,0x8e,0x46,0xeb,0x3d,0x11,0x43 };
	unsigned char footer[]{ 0x5f,0x64,0x7d,0x17 };
	if (argc != 4)
	{
		cout << "arguments are:" << endl << "Project0verflow.exe -pack folder_name output_name" << endl << "Project0verflow.exe -extract input_file output_folder";
		return -1;
	}
	if (argc==4 && argv[1][1]=='p' )//pack
	{
		cout << "in pack";
		uint32_t offsetCounter = 0;
		vector<entry> entries;
		ofstream output(argv[3], fstream::binary);
		for (const auto & currentFile : filesystem::directory_iterator(argv[2]))
		{
			if (currentFile.is_regular_file() && filesystem::path(currentFile).filename()!="keyfile.bin")
			{
				entries.push_back({ (uint32_t)offsetCounter, filesystem::path(currentFile).filename(), true });
				ifstream current(currentFile.path(), std::ifstream::ate | std::ifstream::binary);
				streamsize size = current.tellg();
				current.seekg(0, ios::beg);
				unsigned char* data = new unsigned char[size];
				current.read((char*)data, size);//load file in
				vector<unsigned char> * packedData = LZPack(data, size);
				for (unsigned char ch : *packedData)
				{
					output << ch;
					output.flush();
					offsetCounter++;
				}
				current.close();
				delete packedData;
				delete data;

			}
		}
		vector<unsigned char> * entriesData = serializeEntries(&entries, offsetCounter);
		vector<unsigned char> * packedEntries = LZPack(&(*entriesData)[0], entriesData->size());
		int keyStartPos = 0;
		int keyLength = 16;
		for (int i = 0; i < packedEntries->size() - 4; i++)
		{
			cout << (int)packedEntries->at(i + 4) << " " << (int)key[(keyStartPos + i) % keyLength] << endl;
			packedEntries->at(i + 4) ^= key[(keyStartPos + i) % keyLength];
		}
		for (unsigned char ch : *packedEntries)
		{
			output << ch;
			output.flush();
		}
		for (int i = 0; i < 4; i++)
		{
			output << footer[i];
			output.flush();
		}

		for (int i = 0; i < 4; i++)
		{
			output << (char)offsetCounter;
			offsetCounter = offsetCounter >> 8;
			output.flush();
		}
		output.close();
		delete entriesData;
		delete packedEntries;

	}
	else if (argc == 4 && argv[1][1] == 'e')//extract
	{
		ifstream file;
		file.open(argv[2], std::ifstream::ate | std::ifstream::binary);
		streamsize size = file.tellg();
		file.seekg(0, ios::beg);
		unsigned char* data = new unsigned char[size];
		file.read((char*)data, size);//load file in

		uint64_t indexOffset = readLittleEndian(data, size - 4, 4);
		uint64_t uncompressedIndexSize = readLittleEndian(data, indexOffset, 4);//find index data
		uint64_t compressedIndexSize = size - indexOffset - 4;
		file.seekg(indexOffset + 4, ios::beg); //step past size
		unsigned char* compressedIndexData = new unsigned  char[compressedIndexSize];
		unsigned char* uncompressedIndexData = new unsigned  char[uncompressedIndexSize];
		file.read((char*)compressedIndexData, compressedIndexSize);//load index in

		int keyLength = 16;
		int keyStartPos = 0;//may be nessecary later on but is fine for now
		for (int i = 0; i < compressedIndexSize; i++)
		{
			compressedIndexData[i] ^= key[(keyStartPos + i) % keyLength];
		}
		LzUnpack(compressedIndexData, uncompressedIndexData, uncompressedIndexSize);

		vector<entry> * entries = getEntries(uncompressedIndexData, uncompressedIndexSize);


		filesystem::create_directory(argv[3]);
		string originalOutputPath = argv[3];
		int asize = 0;
		for (int i = 0; argv[3][i] != '\0'; i++)
			asize++;
		wstring outputPath;
		for (int i = 0; i < asize; i++)
			outputPath += originalOutputPath[i];
		wcout << outputPath << "_" << endl;
		for (entry e : *entries)
		{
			ofstream out(outputPath + e.name, fstream::binary);
			file.seekg(e.offset, ios::beg);
			int currentFileSize = readLittleEndian(data, e.offset, 4);//fix
			unsigned char * unPacked = new unsigned char[currentFileSize];
			LzUnpack(data, unPacked, currentFileSize, e.offset+4);
			for (int i = 0; i < currentFileSize; i++)
			{
				out << unPacked[i];
				out.flush();
			}
			out.close();
			delete unPacked;
			ifstream check(outputPath + e.name);
			if (check)
				wcout << e.name << " extracted" << endl;
			else
				wcout << "ERROR EXTRACTING " << e.name << endl;
		}
		ofstream out((string)argv[3]+ (string)"//keyfile.bin", fstream::binary);
		for (char c : key)
		{
			out << c;
			out.flush();
		}
		out.close();
	}
	cout << "done" << endl;
}
