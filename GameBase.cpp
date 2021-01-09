#include "GameBase.h"

bool GameBase::extract(std::ifstream * file, std::string outPutFolder)
{
	return false;
}

bool GameBase::pack(std::ofstream * output, std::string scriptFolder)
{
	return false;
}

bool GameBase::respaceArchive(std::ifstream * file)
{
	return false;
}

bool GameBase::respaceFolder(std::string outPutFolder)
{
	return false;
}

bool GameBase::generateScript(std::ofstream * outputFile, std::string scriptDirectory)
{
	return false;
}

std::string GameBase::gameName()
{
	return "Current game is not implemented";
}
