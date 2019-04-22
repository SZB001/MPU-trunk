

for /R  %%i in (*.c, *.h) do (
    path d:\Astyle\bin
    Astyle.exe --options=d:\Astyle\bin\custom --ascii %%i
)