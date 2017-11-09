$source = "http://download.videolan.org/pub/videolan/vlc/2.2.6/vlc-2.2.6.tar.xz"
$destination = ".\~tmp\vlc\vlc-2.2.6.tar.xz"

if(!(Test-Path -Path ".\~tmp\vlc" )){
	New-Item -Path ".\~tmp\vlc" -ItemType directory
}

if(!(Test-Path -Path $destination )){
	Invoke-WebRequest $source -OutFile $destination
}

if(!(Test-Path -Path ".\~tmp\vlc\vlc-2.2.6.tar" )){
	Invoke-Expression "& .\7z.exe -o`".\~tmp\vlc`" x `".\~tmp\vlc\vlc-2.2.6.tar.xz`""
}

if(!(Test-Path -Path ".\~tmp\vlc\vlc-2.2.6\" )){
	Invoke-Expression "& .\7z.exe -o`".\~tmp\vlc`" x `".\~tmp\vlc\vlc-2.2.6.tar`""
}

if(!(Test-Path -Path ".\include\vlc-2.2\vlc\" )){
	Copy-Item ".\~tmp\vlc\vlc-2.2.6\include\*" ".\include\vlc-2.2\"
	Copy-Item ".\~tmp\vlc\vlc-2.2.6\include\vlc\*" ".\include\vlc-2.2\vlc\"
}

$source = "http://download.videolan.org/pub/videolan/vlc/2.2.6/win32/vlc-2.2.6-win32.zip"
$destination = ".\~tmp\vlc\vlc-2.2.6-win32.zip"

if(!(Test-Path -Path $destination )){
	Invoke-WebRequest $source -OutFile $destination
}

if(!(Test-Path -Path ".\~tmp\vlc\vlc-2.2.6-win32\" )){
	Invoke-Expression "& .\7z.exe -o`".\~tmp\vlc\vlc-2.2.6-win32`" x `"$destination`""
}

if(!(Test-Path -Path ".\bin\x86\" )){
	New-Item -Path ".\bin\x86\" -ItemType directory
	
	Copy-Item ".\~tmp\vlc\vlc-2.2.6-win32\vlc-2.2.6\libvlc*.dll" ".\bin\x86\"
	Copy-Item ".\~tmp\vlc\vlc-2.2.6-win32\vlc-2.2.6\plugins\" ".\bin\x86\plugins\" -recurse
}

$source = "http://download.videolan.org/pub/videolan/vlc/2.2.6/win64/vlc-2.2.6-win64.zip"
$destination = ".\~tmp\vlc\vlc-2.2.6-win64.zip"

if(!(Test-Path -Path $destination )){
	Invoke-WebRequest $source -OutFile $destination
}

if(!(Test-Path -Path ".\~tmp\vlc\vlc-2.2.6-win64\" )){
	Invoke-Expression "& .\7z.exe -o`".\~tmp\vlc\vlc-2.2.6-win64`" x `"$destination`""
}

if(!(Test-Path -Path ".\bin\x64\" )){
	New-Item -Path ".\bin\x64\" -ItemType directory
	
	Copy-Item ".\~tmp\vlc\vlc-2.2.6-win64\vlc-2.2.6\libvlc*.dll" ".\bin\x64\"
	Copy-Item ".\~tmp\vlc\vlc-2.2.6-win64\vlc-2.2.6\plugins\" ".\bin\x64\plugins\" -recurse
}

