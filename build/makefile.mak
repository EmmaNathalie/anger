CurDir = $(MAKEDIR)
SolutionDir = $(CurDir)\..
Bin86 = c:\Users\user\Desktop\anger\bin\Debug\x86
Bin64 = c:\Users\user\Desktop\anger\bin\Debug\x64

prepare_dirs:
	@echo $(SolutionDir)
	@if exist $(SolutionDir)\bin rmdir /S /Q $(SolutionDir)\bin

build: prepare_dirs loader
	@echo Build anger...
	@echo $(CurDir)
	

rebuild:


clean:


maker: installer
	@echo Build maker
	cd ..\\maker
	msbuild.exe maker.vcxproj /p:Configuration=Debug;Platform=Win32

installer:
	@echo Build installer
	cd ..\\installer
	@set CL=/DSCHEDULER
	rmdir /S /Q Debug
	msbuild.exe installer.vcxproj /p:Configuration=Debug;Platform=Win32
	set CL=/DSERVICE
	rmdir /S /Q Debug
	msbuild.exe installer.vcxproj /p:Configuration=Debug;Platform=Win32
	set CL=/DSVCHOST
	rmdir /S /Q Debug
	msbuild.exe installer.vcxproj /p:Configuration=Debug;Platform=Win32

loader:
	@echo Build loader...
#	@if not exist $(SolutionDir)\bin mkdir Bin86 && mkdir Bin64
	cd ..\\loader
	@echo  scheduler version
	rmdir /S /Q Debug
	@set CL=/DSCHEDULER
	msbuild.exe loader.vcxproj /p:Configuration=Debug;Platform=Win32 /t:Build /noconlog
#	rmdir /S /Q Debug
	msbuild.exe loader.vcxproj /p:Configuration=Debug;Platform=x64 /t:Build /noconlog
	@echo  svchost version
	rmdir /S /Q Debug
	@set CL=/DSVCHOST
	msbuild.exe loader.vcxproj /p:Configuration=Debug;Platform=Win32 /t:Build /noconlog
#	rmdir /S /Q Debug
	msbuild.exe loader.vcxproj /p:Configuration=Debug;Platform=x64 /t:Build /noconlog
	@echo  service version
	rmdir /S /Q Debug
	@set CL=/DSERVICE
	msbuild.exe loader.vcxproj /p:Configuration=Debug;Platform=Win32 /t:Build /noconlog
#	rmdir /S /Q Debug
	msbuild.exe loader.vcxproj /p:Configuration=Debug;Platform=x64 /t:Build /noconlog
