@echo off
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "discord_test.vcxproj" /p:Configuration=Release /p:Platform=x64 /m /v:minimal
