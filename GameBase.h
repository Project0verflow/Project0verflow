#pragma once
#include <fstream>
class GameBase
{
public:
	virtual bool extract(std::ifstream *file, std::string outPutFolder);
	virtual bool pack(std::ofstream *output, std::string scriptFolder);
	virtual bool respaceArchive(std::ifstream *file);
	virtual bool respaceFolder(std::string outPutFolder);
	virtual bool generateScript(std::ofstream *outputFile, std::string scriptDirectory);
	virtual std::string gameName();
};