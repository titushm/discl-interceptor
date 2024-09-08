IF EXIST build (
	rmdir /s /q build
)

mkdir build
cd src
go build -o ../build -ldflags -H=windowsgui

@echo Build complete!
pause