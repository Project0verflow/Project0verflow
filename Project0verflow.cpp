//To do:
//fix LZPack possibly can be sized to 62

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>//min
#include <vector>
#include <conio.h>//getch
#include <filesystem>
#include "SummerRadishVacation1_1.h"
#include "GameBase.h"

using namespace std;

GameBase** GameList = new GameBase*[20]{
	new GameBase(), //Large PonPon						0verflow resource			0
	new GameBase(), //Pure Mail							PureMail					1
	new GameBase(), //PureChatter													2
	new GameBase(), //QuizDE PonPon													3
	new GameBase(), //PureMail Gaiden												4
	new GameBase(), //PureMail After												5
	new GameBase(), //Staff Room													6
	new GameBase(), //Snow Radish Vacation!!			PureMail					7
	new FORIS_OS("Imouto de Ikou!", 1, 1),//			FORIS-OS					8
	new FORIS_OS("Summer Radish Vacation 1.1", 1, 1),//	FORIS-OS					9
	new FORIS_OS("Magical Unity", 1, 1), //				FORIS-OS					10
	new FORIS_OS("Miss Each Other", -1, 0, false), //	FORIS-OS					11			Needs custom packer, stores files unpacked
	new FORIS_OS("Summer Radish Vacation!! 2",0,2),//	FORIS-OS					12
	new GameBase(), //School Days (Likely won't add)								13
	new GameBase(), //Summer Days (Likely won't add)								14
	new GameBase(), //Cross Days													15
	new GameBase(), //Shiny Days													16
	new GameBase(), //Island Days													17
	new GameBase(), //Strip Battle Days 2											18
	new GameBase() //Summer Radish Vacation 1.0										19
};

//exe -p folder name
//exe -e file folder
int main(int argc, char *argv[])
{
	//_getch();
	int id = stoi(argv[1]);
	cout << GameList[id]->gameName() << endl;
	if (argc==5 && argv[2][1]=='p' )//pack
	{
		ofstream output(argv[4], fstream::binary);
		GameList[id]->pack(&output, argv[3]);
	}
	else if (argc == 5 && argv[2][1] == 'e')//extract
	{
		ifstream file(argv[3], std::ifstream::ate | std::ifstream::binary);
		GameList[id]->extract(&file, argv[4]);
	}
	else if (argc == 5 && argv[2][1] == 's')
	{
		ofstream output(argv[4], fstream::binary);
		GameList[id]->generateScript(&output, argv[3]);
	}
	else
	{
		cout << "arguments are:" << endl << "Project0verflow.exe -pack folder_name output_name" << endl << "Project0verflow.exe -extract input_file output_folder";
		return -1;
	}
	cout << "done" << endl;
}
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
