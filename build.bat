IF EXIST build (
	rmdir /s /q build
)

mkdir build

gcc src/Update.c src/icon.res -o build/Update.exe -mwindows
mt.exe -manifest src/Update.exe.manifest -outputresource:build/Update.exe;#1
@echo Build complete!