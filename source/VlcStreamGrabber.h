#ifndef VLC_STREAM_GRABBER_H_INCLUDED
#define VLC_STREAM_GRABBER_H_INCLUDED

#include "libvlc_win.h"

typedef struct VlcStreamGrabber_
{
	libvlc_instance_t * pVlcInstance;
	libvlc_media_player_t * pVlcMediaPlayer;	
	vlc_mutex_t mutex;
	bool terminating;
	bool paceControl;

	int videoBufferReadIndex;
	int videoBufferWriteIndex;
	
	bool noVideo;
	bool videoFrameRequested;
	vlc_sem_t videoFrameRequestSemaphore;
	vlc_sem_t videoFrameReadySemaphore;
	video_format_t videoFormats[2];
	int videoFrameWidth[2];
	int videoFrameLines[2];
    int videoFrameBitsPerPixel[2];
    libvlc_video_track_t videoTracks[2];
	uint8_t * pVideoBuffers[2];
	size_t videoAllocatedBufferSizes[2];
	size_t videoBufferSizes[2];
	int64_t videoFrameTimestamps[2];
	int64_t videoStartTimestamp;
	bool videoBufferRead[2];
	

	int audioBufferReadIndex;
	int audioBufferWriteIndex;
	
	bool noAudio;
	bool audioSampleRequested;
	vlc_sem_t audioSampleRequestSemaphore;
	vlc_sem_t audioSampleReadySemaphore;
    libvlc_audio_track_t audioTracks[2];
	unsigned int audioSampleChannels[2];
	unsigned int audioSampleRate[2];
	unsigned int audioSampleSampleCounts[2];
	unsigned int audioSampleBitsPerSample[2];
	uint8_t * pAudioBuffers[2];
	size_t audioAllocatedBufferSizes[2];
	size_t audioBufferSizes[2];
	int64_t audioSampleTimestamps[2];
	int64_t audioStartTimestamp;
	bool audioBufferRead[2];
	unsigned int channels;
} VlcStreamGrabber;

int  StreamGrabberInit(VlcStreamGrabber ** ppGrabber);
void StreamGrabberFree(VlcStreamGrabber * pGrabber);
int  StreamGrabberSetMedia(VlcStreamGrabber * pGrabber, libvlc_media_t * pMedia, bool paceControl);

bool StreamGrabberGetVideoFrame(VlcStreamGrabber * pGrabber);
bool StreamGrabberGetAudioSample(VlcStreamGrabber * pGrabber);

#endif //!VLC_STREAM_GRABBER_H_INCLUDED