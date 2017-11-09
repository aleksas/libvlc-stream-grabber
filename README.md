[![Build status](https://ci.appveyor.com/api/projects/status/7vf4u9wgylkvf9k1?svg=true)](https://ci.appveyor.com/project/aleksas/libvlc-stream-grabber)

# libvlc-stream-grabber
Access video frames and audio sample using libvlc smem module.

Application written in C using Visual Studio 2012 under Windows, but requires almost no changes for use under other os.

Instead of using libvlc_video_set_callbacks, libvlc_video_set_format_callbacks, libvlc_audio_set_callbacks and libvlc_audio_set_format_callbacks smem module is used. The main reason for that is that smem provides all info for video frame as well as audio sample. Said callbacks do not do that for video frames. Additionally smem allows toggling playback pace control which comes handy when you are processing data.


Follow TODO.txt instructions to compile and run the code.

### WITH VLC 2.2.4:

But there is also a downside for using smem: you have to use it with transcoding which crashes on some RTSP streams (not so for files) contrary to the said callbacks. The crash constitute itself in "Unhandled exception at 0x605B88BB (libstream_out_transcode_plugin.dll) in VlcStreamGrabber.exe: 0xC0000094: Integer division by zero."
And strangely enaugh libvlc_media_player_stop and libvlc_media_player_release crash application so these calls are commented out in the code.

### WITH VLC 3.0.0 nightly:

No issues seen.

To use 3.0.0 substitute 2.2.4 binaries and headers indicated in TODO.txt with 3.0.0 ones.
Also uncomment lines 108, 117, 118 in VlcStreamGrabber.cpp file as they perform normaly in 3.0.0.

