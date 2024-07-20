#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef void (__stdcall *Py_Initialize)(void);
typedef int (__stdcall *Py_BytesMain)(int argc, char** argv);

char* find_pth_file(const char* base_path) {
	WIN32_FIND_DATA find_file_data;
	HANDLE h_find = INVALID_HANDLE_VALUE;
	char search_path[MAX_PATH];
	char *pth_file = NULL;
	snprintf(search_path, MAX_PATH, "%s\\*._pth", base_path);
	h_find = FindFirstFile(search_path, &find_file_data);
	if (h_find == INVALID_HANDLE_VALUE) {
		MessageBox(NULL, "Could not find _pth file", "Discl", MB_OK);
		return NULL;
	} else {
		pth_file = (char*)malloc(strlen(find_file_data.cFileName) + strlen(base_path) + 2);
		if (pth_file == NULL) {
			perror("malloc");
			FindClose(h_find);
			return NULL;
		}
		snprintf(pth_file, strlen(find_file_data.cFileName) + strlen(base_path) + 2, "%s\\%s", base_path, find_file_data.cFileName);
		FindClose(h_find);
		return pth_file;
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	LPWSTR lpCmdLineWide = GetCommandLineW();
	int argc;
	LPWSTR *argvWide = CommandLineToArgvW(lpCmdLineWide, &argc);
	char **argv = (char **)malloc(argc * sizeof(char *));
	for (int i = 0; i < argc; ++i)
	{
		int len = WideCharToMultiByte(CP_ACP, 0, argvWide[i], -1, NULL, 0, NULL, NULL);
		argv[i] = (char *)malloc(len);
		WideCharToMultiByte(CP_ACP, 0, argvWide[i], -1, argv[i], len, NULL, NULL);
	}

	char *APPDATA = getenv("LOCALAPPDATA");
	char *DISCL_LOCATION = strcat(APPDATA, "\\discord\\discl_location.txt");
	FILE* location = fopen(DISCL_LOCATION, "r");
	if (location == NULL)
	{
		MessageBox(NULL, "Could not find discl location", "Discl", MB_OK);
		return 1;
	}
	char DISCL_PATH[MAX_PATH];
	fread(DISCL_PATH, MAX_PATH, sizeof(char), location);
	fclose(location);
	bool processStartArgProvided = false;
	bool uninstallArgProvided = false;

	if (argc == 1)
	{
		processStartArgProvided = true;
	}
	else
	{
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "--processStart") == 0 && i < argc - 1 && strcmp(argv[i + 1], "Discord.exe") == 0)
			{
				processStartArgProvided = true;
				break;
			}
			else if (strcmp(argv[i], "--uninstall") == 0)
			{
				uninstallArgProvided = true;
			}
		}
	}

	if (processStartArgProvided)
	{
		char PYTHON_DLL_PATH[MAX_PATH];
		char PYTHON_PATH[MAX_PATH];
		char PYTHON_DIRECTORY_PATH[MAX_PATH];
		snprintf(PYTHON_PATH, sizeof(PYTHON_PATH), "%s\\discl-main\\src\\discl.py", DISCL_PATH);
		snprintf(PYTHON_DIRECTORY_PATH, sizeof(PYTHON_DIRECTORY_PATH), "%s\\python", DISCL_PATH);
		char* dll_path = find_pth_file(PYTHON_DIRECTORY_PATH);
		char* ext_position = strstr(dll_path, "_pth");
		size_t position = ext_position - dll_path;
		strncpy(dll_path + position, "dll", 4);
		snprintf(PYTHON_DIRECTORY_PATH, sizeof(PYTHON_DIRECTORY_PATH), "%s\\python", DISCL_PATH);
		HMODULE hPythonDLL = LoadLibrary(dll_path);
		MessageBox(NULL, dll_path, "Discl", MB_OK);
		if (hPythonDLL == NULL)
		{
			MessageBox(NULL, "Could not find python dll", "Error", MB_OK | MB_ICONERROR);
			return 1;
		}
		Py_Initialize py_Initialize = (Py_Initialize)GetProcAddress(hPythonDLL, "Py_Initialize");
		Py_BytesMain py_BytesMain = (Py_BytesMain)GetProcAddress(hPythonDLL, "Py_BytesMain");
		if (py_Initialize == NULL || py_BytesMain == NULL)
		{
			MessageBox(NULL, "Could not find python functions", "Error", MB_OK | MB_ICONERROR);
			return 1;
		}
		py_Initialize();

		char** pyargv = malloc((argc+1)*sizeof pyargv[0]);
		pyargv[0] = argv[0];
		pyargv[1] = PYTHON_PATH;
		for (int i = 1; i < argc; i++) { pyargv[i+1] = argv[i]; }
		py_BytesMain(argc+1, pyargv);
		FreeLibrary(hPythonDLL);
		return 0;
	}
	else
	{
		if (uninstallArgProvided)
		{
			MessageBox(NULL, "Please uninstall discl via the installer before uninstalling Discord.\nThe installer can be found at github.com/titushm/discl-installer", "Discl", MB_OK);
		}
		else
		{
			char *UPDATE_DISCORD_PATH = strcat(APPDATA, "\\Discord\\Update_Discord.exe");
			char argsString[4096] = "";
			for (int i = 1; i < argc; i++)
			{
				strcat(argsString, argv[i]);
				strcat(argsString, " ");
			}
			char command[4096];
			snprintf(command, sizeof(command), "\"%s\" %s", UPDATE_DISCORD_PATH, argsString);
			WinExec(command, SW_HIDE);
		}
	}

	for (int i = 0; i < argc; ++i)
	{
		free(argv[i]);
	}
	free(argv);
	LocalFree(argvWide);
	return 0;
}