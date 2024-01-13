#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
		char *DISCL_PATH = strcat(APPDATA, "\\discl\\discl-main\\src\\discl.py");
		char command[4096];
		snprintf(command, sizeof(command), "python %s", DISCL_PATH);
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		if (CreateProcess(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
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

	return 0;
}
