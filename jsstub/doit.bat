del jsstub.exe
del dirlist.exe
cl /EHsc jsstub.c /link ole32.lib oleaut32.lib && copy /b jsstub.exe+dirlist.js dirlist.exe && dirlist.exe
