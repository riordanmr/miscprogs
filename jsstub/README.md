# jsstub

jsstub is a Windows program that acts as a stub to create executable files 
from JavaScript source.

Build jsstub.exe and then append your JavaScript program, like this:

    copy /b jsstub.exe+myjsprog.js myjsprog.exe

Now you can execute myjsprog.exe. Your users won't even know that
they are running a JavaScript program.

jsstub uses the Windows Scripting Engine - which is built in to Windows -
so your JavaScript can use only objects that come with the Microsoft
implementation.  This notably includes COM objects that come with Windows,
such as Scripting.FileSystemObject.
