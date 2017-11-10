[![Build status](https://ci.appveyor.com/api/projects/status/7vf4u9wgylkvf9k1?svg=true)](https://ci.appveyor.com/project/aleksas/libvlc-stream-grabber)

# libvlc-stream-grabber
Access video frames and audio sample using libvlc smem module.

Application written in C using Visual Studio 2012 under Windows, but requires almost no changes for use under other os.

Instead of using libvlc_video_set_callbacks, libvlc_video_set_format_callbacks, libvlc_audio_set_callbacks and libvlc_audio_set_format_callbacks smem module is used. The main reason for that is that smem provides all info for video frame as well as audio sample. Said callbacks do not do that for video frames. Additionally smem allows toggling playback pace control which comes handy when you are processing data.

Follow TODO.txt instructions to compile and run the code OR execute requirenments.ps1 .