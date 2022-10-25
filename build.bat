@echo off

pushd bin\

cl.exe /nologo /diagnostics:caret /fsanitize=address /W4 /WX /wd4100 /Zi /GR- /EHa- /Odi /MTd /FC /D_CRT_SECURE_NO_WARNINGS ..\src\main.c /link /incremental:no /out:varia.exe

exit /b %errorlevel%
