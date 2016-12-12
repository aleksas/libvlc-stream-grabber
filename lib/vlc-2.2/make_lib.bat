"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"

mkdir x86
mkdir x64

lib /def:"libvlc.def" /out:"x86\libvlc.lib" /machine:x86
lib /def:"libvlc.def" /out:"x64\libvlc.lib" /machine:x64

lib /def:"libvlccore.def" /out:"x86\libvlccore.lib" /machine:x86
lib /def:"libvlccore.def" /out:"x64\libvlccore.lib" /machine:x64