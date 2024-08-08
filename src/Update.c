#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef void (__stdcall *Py_Initialize)(void);
typedef int (__stdcall *Py_Main)(int argc, wchar_t** argv);

int exitMsg(bool err, wchar_t* err_msg, void* argv) {
	if (err && err_msg != NULL){
		MessageBox(NULL, err_msg, L"Discl Error", MB_OK | MB_ICONERROR);
	}
	LocalFree(argv);
	return err?1:0;
}

// returns the full path to the first _pth file in the base_path directory
// the user needs to free the return value
wchar_t* find_pth_file(const wchar_t* base_path) {
	#define pth_ext L"\\*._pth"
	size_t search_path_len = wcslen(base_path)+wcslen(pth_ext);
	wchar_t* search_path = calloc(search_path_len+1, sizeof(search_path[0]));
	if (search_path == NULL) {
		exitMsg(false, L"malloc failed probably out of RAM", NULL);
		return NULL;
	}
	swprintf(search_path, search_path_len+1, "%s"pth_ext, base_path);

	WIN32_FIND_DATA find_file_data;
	HANDLE h_find = FindFirstFile(search_path, &find_file_data);
	free(search_path);
	if (h_find == INVALID_HANDLE_VALUE) {
		MessageBox(NULL, L"Could not find _pth file", L"Discl", MB_OK);
		return NULL;
	}

	size_t pth_file_len = wcslen(find_file_data.cFileName) + wcslen(L"\\") + wcslen(base_path) + 1;
	wchar_t* pth_file = calloc(pth_file_len, sizeof(pth_file[0]));
	if (pth_file == NULL) {
		exitMsg(false, L"malloc failed probably out of RAM", NULL);
		FindClose(h_find);
		return NULL;
	}
	swprintf(pth_file, pth_file_len, L"%s\\%s", base_path, find_file_data.cFileName);
	FindClose(h_find);
	return pth_file;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	wchar_t* APPDATA = _wgetenv(L"LOCALAPPDATA");
	#define discord_discl_location L"\\discord\\discl_location.txt"
	size_t discl_location_len = wcslen(APPDATA)+wcslen(discord_discl_location);
	wchar_t* discl_location = calloc(discl_location_len+1, sizeof(discl_location[0]));
	if (discl_location == NULL) {
		return exitMsg(true, L"malloc failed probably out of RAM", argv);
	}
	swprintf(discl_location, discl_location_len+1, L"%s"discord_discl_location, APPDATA);
	FILE* location = _wfopen(discl_location, L"r");
	// free(discl_location);
	if (location == NULL) {
		MessageBox(NULL, discl_location, L"Discl", MB_OK);
		return exitMsg(true, L"Could not find discl location", argv);
	}
	fseek(location, 0, SEEK_END);
	size_t location_f_size = ftell(location);
	fseek(location, 0, SEEK_SET);
	char* discl_path_utf = calloc(location_f_size+1, 1);
	if (discl_path_utf == NULL) {
		fclose(location);
		return exitMsg(true, L"malloc failed probably out of RAM", argv);
	}
	fread(discl_path_utf, location_f_size, 1, location);
	fclose(location);

	// convert the utf8 of the file to wide charstring that windows uses
	size_t discl_path_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, discl_path_utf, -1, NULL, 0);
	if (discl_path_size == 0) {
		free(discl_path_utf);
		DWORD err = GetLastError();
		wchar_t* errMsg;
		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			0,
			(LPWSTR)&errMsg,
			0,
			NULL
		);
		int rv = exitMsg(true, errMsg, argv);
		LocalFree(errMsg);
		return rv;
	}
	wchar_t* discl_path = malloc(discl_path_size);
	if (discl_path == NULL) {
		free(discl_path_utf);
		return exitMsg(true, L"malloc failed probably out of RAM", argv);
	}
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, discl_path_utf, -1, discl_path, discl_path_size);
	free(discl_path_utf);

	bool processStartArgProvided = false;
	bool uninstallArgProvided = false;

	if (argc == 1) {
		processStartArgProvided = true;
	} else {
		for (int i = 1; i < argc; i++) {
			if (
				wcscmp(argv[i], L"--processStart") == 0 &&
				i < argc - 1 &&
				wcscmp(argv[i + 1], L"Discord.exe") == 0
			) {
				processStartArgProvided = true;
				break;
			} else if (wcscmp(argv[i], L"--uninstall") == 0) {
				uninstallArgProvided = true;
			}
		}
	}

	if (processStartArgProvided) {
		#define slash_python L"\\python"
		size_t python_directory_path_len = wcslen(discl_path) + wcslen(slash_python);
		wchar_t* python_directory_path = calloc(python_directory_path_len+1, sizeof(python_directory_path[0]));
		if (python_directory_path == NULL) {
			return exitMsg(true, L"malloc failed probably out of RAM", argv);
		}
		swprintf(python_directory_path, python_directory_path_len+1, L"%s"slash_python, discl_path);

		wchar_t* dll_path = find_pth_file(python_directory_path);
		free(python_directory_path);
		if (dll_path == NULL) {
			return exitMsg(true, NULL, argv);
		}

		wchar_t* ext_position = wcsstr(dll_path, L"_pth");
		wcsncpy(ext_position, L"dll", 4);
		HMODULE hPythonDLL = LoadLibrary(dll_path);
		free(dll_path);
		if (hPythonDLL == NULL) {
			return exitMsg(true, L"Could not find python dll", argv);
		}
		Py_Initialize py_Initialize = (Py_Initialize)GetProcAddress(hPythonDLL, "Py_Initialize");
		Py_Main py_BytesMain = (Py_Main)GetProcAddress(hPythonDLL, "Py_Main");
		if (py_Initialize == NULL || py_BytesMain == NULL) {
			return exitMsg(true, L"Could not find python functions", argv);
		}
		py_Initialize();

		#define discl_main_src_discl_py L"\\discl-main\\src\\discl.py"
		size_t python_path_len = wcslen(discl_path) + wcslen(discl_main_src_discl_py);
		wchar_t* python_path = calloc(python_path_len+1, sizeof(python_path[0]));
		if (python_path == NULL) {
			return exitMsg(true, L"malloc failed probably out of RAM", argv);
		}
		swprintf(python_path, python_path_len+1, L"%s"discl_main_src_discl_py, discl_path);

		wchar_t** pyargv = calloc(argc+1, sizeof(pyargv[0]));
		if (pyargv == NULL) {
			free(python_path);
			return exitMsg(true, L"malloc failed probably out of RAM", argv);
		}
		pyargv[0] = argv[0];
		pyargv[1] = python_path;
		for (int i = 1; i < argc; i++) {
			pyargv[i+1] = argv[i];
		}
		int rv = py_BytesMain(argc+1, pyargv);
		free(python_path);
		free(pyargv);
		FreeLibrary(hPythonDLL);
		exitMsg(false, NULL, argv);
		return rv;
	}
	if (uninstallArgProvided) {
		MessageBox(NULL, L"Please uninstall discl via the installer before uninstalling Discord.\nThe installer can be found at github.com/titushm/discl-installer", L"Discl", MB_OK);
		return exitMsg(false, NULL, argv);
	}
	#define discord_update_discord L"\\Discord\\Update_Discord.exe"
	size_t command_len = wcslen(APPDATA)+wcslen(discord_update_discord)+wcslen(L" ");
	wchar_t* command = calloc(command_len+1, sizeof(command[0]));
	swprintf(command, command_len+1, L"%s"discord_update_discord,APPDATA);
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));
	if (!CreateProcess(command, GetCommandLineW(), NULL, NULL, false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)){
		free(command);
		return exitMsg(true, L"failed to start the default update.exe", argv);
	}
	free(command);
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD rv;
	GetExitCodeProcess(pi.hProcess, &rv);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	exitMsg(false, NULL, argv);
	return (int)rv;
}
