#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>


#define DEBUG


//arg stuff
#define MAX_ARGS 10
char* args[MAX_ARGS];
unsigned int num_args = 0;


//hack machine state
#define RAM_SIZE 0x10000
short* RAM;
#define ROM_SIZE 0x10000
short* ROM;

short Reg_A;
short Reg_D;
short ProgramCounter;


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


char* GetExtension(char* filename) {
	while (*filename) {
		if (*(filename++) == '.') {
			return filename;
		}
	}
	return filename;
}


void InitHack() {
	RAM = (char*)malloc(RAM_SIZE);
	memset(RAM, 0, RAM_SIZE);
	ROM = (char*)malloc(RAM_SIZE);
	memset(ROM, 0, ROM_SIZE);

	ProgramCounter = 0;
	Reg_A = 0;
	Reg_D = 0;
}




int HackToBin(char* hack_text, short* bin_buffer) {
	int count = 0;
	short value;
	while (*hack_text && count < ROM_SIZE) {
		value = 0;
		while (*hack_text) {
			if (*hack_text == '0') {
				value *= 2;
				value += 0;
				hack_text++;
			} else if (*hack_text == '1') {
				value *= 2;
				value += 1;
				hack_text++;
			} else if (*hack_text == '\n') {
				*bin_buffer = value;
				bin_buffer++;
				hack_text++;
				count++;
				break;
			} else if (*hack_text == '\r'){
				hack_text++;
			}
		}
	}
	return count;
}



void LoadBinFile(char* filename) {
	OFSTRUCT file_struct;
	HFILE file_handle;
	size_t file_size;

	file_handle = OpenFile(filename, &file_struct, OF_READ);
	if (!file_handle) {
		error("error opening rom file");
		CloseHandle((HANDLE)file_handle);
		return 1;
	}

	file_size = (size_t)GetFileSize((HANDLE)file_handle, 0);
	/*
	if (file_size > ROM_SIZE) {
		error("rom file exceeds max rom size!");
		return 1;
	}
	*/
	
	ReadFile((HANDLE)file_handle, ROM, file_size, 0, 0);
	CloseHandle((HANDLE)file_handle);
	return 0;
}



int LoadHackFile(char* filename) {
	OFSTRUCT file_struct;
	HFILE file_handle;
	size_t file_size;

	file_handle = OpenFile(filename, &file_struct, OF_READ);
	if (!file_handle) {
		error("error opening rom file");
		CloseHandle((HANDLE)file_handle);
		return 1;
	}

	file_size = (size_t)GetFileSize((HANDLE)file_handle, 0);

	char* text_buffer = (char*)malloc(file_size + 1);
	ReadFile((HANDLE)file_handle, text_buffer, file_size, 0, 0);
	text_buffer[file_size] = 0;
	HackToBin(text_buffer, ROM);

	CloseHandle((HANDLE)file_handle);
	free(text_buffer);
	return 0;
}



void FetchAndExecuteInstruction() {
	//chapter 4, pages 66-69
	short instruction = ROM[ProgramCounter];
	short computed_value;


#ifdef DEBUG
	if (instruction == 0) {
		__debugbreak();
	}
#endif


	if (instruction & (1 << 15)) {
		// C-instruction

		if (instruction & (1 << 12)) {
			//memory C-instruction
			switch ((instruction >> 6) & 0x3f) {
			case 0x30:
				computed_value = RAM[Reg_A];
				break;
			case 0x31:
				computed_value = ~(RAM[Reg_A]);
				break;
			case 0x33:
				computed_value = -(RAM[Reg_A]);
				break;
			case 0x37:
				computed_value = RAM[Reg_A] + 1;
				break;
			case 0x32:
				computed_value = RAM[Reg_A] - 1;
				break;
			case 0x02:
				computed_value = Reg_D + RAM[Reg_A];
				break;
			case 0x13:
				computed_value = Reg_D - RAM[Reg_A];
				break;
			case 0x07:
				computed_value = RAM[Reg_A] - Reg_D;
				break;
			case 0x00:
				computed_value = Reg_D & RAM[Reg_A];
				break;
			case 0x15:
				computed_value = Reg_D | RAM[Reg_A];
				break;
			default:
				error("unrecognized instruction!");
				break;
			}
		} else {
			//register C-instruction
			switch ((instruction >> 6) & 0x3f) {
			case 0x2A:
				computed_value = 0;
				break;
			case 0x3F:
				computed_value = 1;
				break;
			case 0x3A:
				computed_value = Reg_D;
				break;
			case 0x30:
				computed_value = Reg_A;
				break;
			case 0x0D:
				computed_value = ~Reg_D;
				break;
			case 0x31:
				computed_value = ~Reg_A;
				break;
			case 0x0F:
				computed_value = -Reg_D;
				break;
			case 0x33:
				computed_value = -Reg_A;
				break;
			case 0x1F:
				computed_value = Reg_D + 1;
				break;
			case 0x37:
				computed_value = Reg_A + 1;
				break;
			case 0x0C:
				computed_value = Reg_D - 1;
				break;
			case 0x32:
				computed_value = Reg_A - 1;
				break;
			case 0x02:
				computed_value = Reg_D + Reg_A;
				break;
			case 0x13:
				computed_value = Reg_D - Reg_A;
				break;
			case 0x07:
				computed_value = Reg_A - Reg_D;
				break;
			case 0x00:
				computed_value = Reg_D & Reg_A;
				break;
			case 0x15:
				computed_value = Reg_D | Reg_A;
				break;
			default:
				error("unrecognized instruction!");
				break;
			}
		}

		//destination
		if ((instruction >> 3) & 1) {
			RAM[Reg_A] = computed_value;
		}
		if ((instruction >> 4) & 1) {
			Reg_D = computed_value;
		}
		if ((instruction >> 5) & 1) {
			Reg_A = computed_value;
		}

		// jumping
		// increment by default;  if there is a jump condition, it will
		// overwrite this;
		ProgramCounter++;


		switch (instruction & 7) {
		case 1:
			if (computed_value > 0) {
				ProgramCounter = Reg_A;
			}
			break;
		case 2:
			if (computed_value == 0) {
				ProgramCounter = Reg_A;
			}
			break;
		case 3:
			if (computed_value >= 0) {
				ProgramCounter = Reg_A;
			}
			break;
		case 4:
			if (computed_value < 0) {
				ProgramCounter = Reg_A;
			}
			break;
		case 5:
			if (computed_value != 0) {
				ProgramCounter = Reg_A;
			}
			break;
		case 6:
			if (computed_value <= 0) {
				ProgramCounter = Reg_A;
			}
			break;
		case 7:
			ProgramCounter = Reg_A;
			break;
		}
			 

	} else {
		// A-instruction;
		Reg_A = instruction;
		ProgramCounter++;
	}
}



int WinMain(HINSTANCE instance, HINSTANCE previnstance,	LPSTR lpCmdLine, int cCmdShow) {
	ParseArgs(lpCmdLine);

	if (num_args < 1) {
		error(0, "Must provide program to run");
		exit(1);
	}



	InitHac();

	if (strcmp(GetExtension(args[0]), "hack") == 0) {
		LoadHackFile(args[0]);
	} else if (strcmp(GetExtension(args[0]), "bin") == 0) {
		LoadBinFile(args[0]);
	} else {
		error("unrecognized extension!");
	}

	while (1) {
		FetchAndExecuteInstruction();
	}


}