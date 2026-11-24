@echo off
rem Arma la carpeta Ejecutable con todo lo que necesita el programa para correr.
rem Se corre desde la raiz del repositorio, despues de compilar en Release | x86.

set ORIGEN=ProyectoFinal
set DESTINO=Ejecutable

if not exist "Release\ProyectoFinal.exe" (
    echo No se encontro Release\ProyectoFinal.exe
    echo Compila primero en Visual Studio con la configuracion Release ^| x86.
    pause
    exit /b 1
)

if exist "%DESTINO%" rmdir /s /q "%DESTINO%"
mkdir "%DESTINO%"

copy "Release\ProyectoFinal.exe" "%DESTINO%\" >nul
copy "%ORIGEN%\glew32.dll" "%DESTINO%\" >nul
copy "%ORIGEN%\assimp-vc140-mt.dll" "%DESTINO%\" >nul

xcopy "%ORIGEN%\Shader" "%DESTINO%\Shader\" /e /i /q >nul
xcopy "%ORIGEN%\assets" "%DESTINO%\assets\" /e /i /q >nul
xcopy "%ORIGEN%\Imagenes" "%DESTINO%\Imagenes\" /e /i /q >nul

echo Listo. Prueba el programa con doble clic en %DESTINO%\ProyectoFinal.exe
pause
