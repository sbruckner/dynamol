SET LIBRARY_DIRECTORY=%cd%/lib
SET BINARY_DIRECTORY=%cd%/bin

FOR %%A IN (glfw,glm,glbinding,globjects) DO CALL :Copy %%A

goto End

:Copy

md "%BINARY_DIRECTORY%/Debug"
robocopy "%LIBRARY_DIRECTORY%/%1" "%BINARY_DIRECTORY%/Debug" *d.dll
robocopy "%LIBRARY_DIRECTORY%/%1/bin" "%BINARY_DIRECTORY%/Debug" *d.dll

md "%BINARY_DIRECTORY%/Release"
robocopy "%LIBRARY_DIRECTORY%/%1" "%BINARY_DIRECTORY%/Release" *.dll /XF *d.dll
robocopy "%LIBRARY_DIRECTORY%/%1/bin" "%BINARY_DIRECTORY%/Release" *.dll /XF *d.dll


GOTO :eof

:End
