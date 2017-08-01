#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>


//arg stuff
#define MAX_ARGS 10
char* args[MAX_ARGS];
unsigned int num_args = 0;




//hack machine state
#define RAM_SIZE 0x10000
char* RAM;
#define ROM_SIZE 0x10000
char* ROM;

short Reg_A;
short Reg_D;


void error(char* msg) {
	MessageBoxA(0, msg, "Error!", MB_OK);
}



void ParseArgs(char *cmdline) {

	args[0] = "";

	while (*cmdline && num_args <= MAX_ARGS) {

		while ((*cmdline) < 33 || (*cmdline) > 126) {
			cmdline++;
		}

		args[num_args++] = cmdline;

		while ((*cmdline) >= 33 && (*cmdline) <= 126){
			cmdline++;
		}
		if (*cmdline) {
			*cmdline++ = 0;
		}
	}
}

char* GetArg(int index) {
	if (index >= num_args) {
		return NULL;
	}

	return args[index];
}


void Init() {
	RAM = (char*)malloc(RAM_SIZE);
	memset(RAM, 0, RAM_SIZE);
	ROM = (char*)malloc(RAM_SIZE);
	memset(ROM, 0, ROM_SIZE);
}


void LoadProgram(char* filename) {
	OFSTRUCT file_struct;
	HFILE file_handle;
	size_t file_size;

	file_handle = OpenFile(filename, &file_struct, OF_READ);
	if (!file_handle) {
		error("error opening rom file");
		CloseHandle((HANDLE)file_handle);
		exit(1);
	}

	file_size = (size_t)GetFileSize((HANDLE)file_handle, 0);

	if (file_size > ROM_SIZE) {
		error("rom file exceeds max rom size!");
		exit(1);
	}


	ReadFile((HANDLE)file_handle, ROM, file_size, 0, 0);
	CloseHandle((HANDLE)file_handle);
}



void FetchAndExecuteInstruction() {
	short instruction = ROM[Reg_A];

	if (instruction & (1 << 15) == 0) {
		// D instruction
	} else {
		// A instruction;
		Reg_A = instruction;
	}
}



int WinMain(HINSTANCE instance, HINSTANCE previnstance,	LPSTR lpCmdLine, int cCmdShow) {
	ParseArgs(lpCmdLine);

	if (num_args < 1) {
		error(0, "Must provide program to run");
		exit(1);
	}



	Init();
	LoadProgram(args[0]);	

	while (1) {
		FetchAndExecuteInstruction();
	}


}