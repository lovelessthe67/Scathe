@echo off
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "scathe.sln" /p:Configuration=Release /p:Platform=x64 /m /v:minimal
