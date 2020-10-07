SET LIBRARY_DIRECTORY=%cd%/lib
SET BINARY_DIRECTORY=%cd%/bin

FOR %%A IN (glfw,glm,glbinding,globjects) DO CALL :Copy %%A

goto End

:Copy

md "%BINARY_DIRECTORY%"
robocopy "%LIBRARY_DIRECTORY%/%1" "%BINARY_DIRECTORY%" *.dll
robocopy "%LIBRARY_DIRECTORY%/%1/bin" "%BINARY_DIRECTORY%" *.dll

GOTO :eof

:End
