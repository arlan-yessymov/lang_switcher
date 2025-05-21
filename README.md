Утилита для переключения языка на Windows

По сочетанию Ctrl+Shift - переключаются русский и английский, остальные языки пропускаются

По Alt+1...Alt+9 можно настроить переключение на другие языки

Сборка cmd:

windres resource.rc -o resource.o 

gcc main.c resource.o -mwindows -o LangSwitcher.exe 

