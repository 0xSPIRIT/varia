@echo off
pushd bin\
varia test.c
popd
exit %errorlevel%