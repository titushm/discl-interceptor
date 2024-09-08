package main

import (
	"os"
	"strings"
	"syscall"
	"unsafe"
)

func MessageBox(hwnd uintptr, caption, title string, flags uint) int {
	captionPointer, _ := syscall.UTF16PtrFromString(caption)
	titlePointer, _ := syscall.UTF16PtrFromString(title)
	ret, _, _ := syscall.NewLazyDLL("user32.dll").NewProc("MessageBoxW").Call(
		uintptr(hwnd),
		uintptr(unsafe.Pointer(captionPointer)),
		uintptr(unsafe.Pointer(titlePointer)),
		uintptr(flags))
	return int(ret)
}

func exitMessage(message string) {
	MessageBox(0, message, "Error", 0x10)
	os.Exit(0)
}

func main() {
	APPDATA_PATH := os.Getenv("LOCALAPPDATA")
	DISCORD_PATH := APPDATA_PATH + "\\Discord"
	processStartArg := false
	uninstallArg := false

	if len(os.Args) < 2 {
		processStartArg = true
	}
	for _, arg := range os.Args {
		if arg == "--processStart" {
			processStartArg = true
		}
		if arg == "--uninstall" {
			uninstallArg = true
		}
	}
	if processStartArg {
		data, err := os.ReadFile(DISCORD_PATH + "\\discl_location.txt")
		if err != nil {
			exitMessage("Could not find discl_location.txt in Discord directory.\nPlease reinstall discl via the installer.\nThe installer can be found at github.com/titushm/discl-installer. " + err.Error())
		}
		DISCL_PATH := string(data)
		if _, err := os.Stat(DISCL_PATH); os.IsNotExist(err) {
			exitMessage("Discl is not installed. This means that your discl installation is corrupted. Please reinstall discl via the installer.\nThe installer can be found at github.com/titushm/discl-installer. " + err.Error())
		}
		PYTHON_PATH := DISCL_PATH + "\\python"
		files, err := os.ReadDir(PYTHON_PATH)
		if err != nil {
			exitMessage("Could not index python directory. " + err.Error())
		}
		dllPath := ""
		for _, file := range files {
			if file.IsDir() {
				continue
			}
			if strings.HasSuffix(file.Name(), "._pth") {
				dllPath = PYTHON_PATH + "\\" + strings.Replace(file.Name(), "._pth", ".dll", 1)
			}
		}
		if dllPath == "" {
			exitMessage("Could not find the discl dll in the python directory.")
		}
		handle, err := syscall.LoadLibrary(dllPath)
		if err != nil {
			exitMessage("Could not load the discl dll. " + err.Error())
		}

		pyInitialize, err := syscall.GetProcAddress(handle, "Py_Initialize")
		if err != nil {
			exitMessage("Could not find the Py_Initialize function in the discl dll. " + err.Error())
		}
		pyMain, err := syscall.GetProcAddress(handle, "Py_Main")
		if err != nil {
			exitMessage("Could not find the Py_Main function in the discl dll. " + err.Error())
		}
		_, _, err = syscall.SyscallN(pyInitialize)
		pyArgv := []*uint16{syscall.StringToUTF16Ptr(os.Args[0]), syscall.StringToUTF16Ptr(DISCL_PATH + "\\discl-main\\src\\discl.py")}
		syscall.SyscallN(uintptr(pyMain), uintptr(len(pyArgv)), uintptr(unsafe.Pointer(&pyArgv[0])), 0)
	} else if uninstallArg {
		exitMessage("Please uninstall discl via the installer before uninstalling Discord.\nThe installer can be found at github.com/titushm/discl-installer")
	} else {
		os.StartProcess(DISCORD_PATH+"\\Update_Discord.exe", os.Args, &os.ProcAttr{Files: []*os.File{os.Stdin, os.Stdout, os.Stderr}})
	}
	os.Exit(0)

}
