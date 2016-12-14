#include "VlcStreamGrabber.h"

void * VideoFrameReader(void * pData);
void * AudioSampleReader(void * pData);

int main(int argc, char* argv[])
{
	vlc_thread_t videoFrameReaderThread;
	vlc_thread_t audioSampleReaderThread;
	libvlc_media_t * pMedia;
	bool paceControl = true;

	void * pVideoThreadResult;
	void * pAudioThreadResult;

	VlcStreamGrabber * pGrabber = NULL;

	if (StreamGrabberInit(&pGrabber) != VLC_SUCCESS) goto FINALLY;

	pMedia = libvlc_media_new_path(pGrabber->pVlcInstance, "..\\..\\data\\SampleVideo_1280x720_1mb.mp4");
	//pMedia = libvlc_media_new_path(pGrabber->pVlcInstance, "..\\..\\data\\Vivaldi - Spring from Four Seasons.mp3");
	
	if (!pMedia) goto FINALLY;

	StreamGrabberSetMedia(pGrabber, pMedia, paceControl);

	libvlc_media_release(pMedia); pMedia = NULL;

	vlc_clone(&videoFrameReaderThread, &VideoFrameReader, pGrabber, 0);
	vlc_clone(&audioSampleReaderThread, &AudioSampleReader, pGrabber, 0);

	libvlc_media_player_play(pGrabber->pVlcMediaPlayer);
	
	vlc_join(videoFrameReaderThread, &pVideoThreadResult);
	vlc_join(audioSampleReaderThread, &pAudioThreadResult);

FINALLY:
	if (pGrabber) StreamGrabberFree(pGrabber);

	printf("Done");
}

void * VideoFrameReader(void * pData)
{
	VlcStreamGrabber * pGrabber = (VlcStreamGrabber *) pData;
	bool locked;
	int r_i;

	do
	{
		StreamGrabberLockVideoBuffer(pGrabber); locked = true;

		if (!StreamGrabberGetVideoFrame(pGrabber)) break;

		r_i = pGrabber->videoBufferReadIndex;

		printf(
			"V t:%lld w:%d h:%d l:%d bpp:%d fr:%d/%d addr:%lld size:%d\r\n",
			(pGrabber->videoFrameTimestamps[r_i] - pGrabber->videoStartTimestamp),
			pGrabber->videoFormats[r_i].i_visible_width,
			pGrabber->videoFormats[r_i].i_visible_height,
			pGrabber->videoFormats[r_i].i_height,
			pGrabber->videoFrameBitsPerPixel[r_i],
			pGrabber->videoTracks[r_i].i_frame_rate_num,
			pGrabber->videoTracks[r_i].i_frame_rate_den,
			(long long int)pGrabber->pVideoBuffers[r_i],
			pGrabber->videoBufferSizes[r_i]
		);

		// copy data to your buffer here

		StreamGrabberUnlockVideoBuffer(pGrabber); locked = false;

		// process frame here

	} while(true);
	
	if (locked) StreamGrabberUnlockVideoBuffer(pGrabber);

	return NULL;
}

void * AudioSampleReader(void * pData)
{
	VlcStreamGrabber * pGrabber = (VlcStreamGrabber *) pData;
	bool locked;
	int r_i;

	do
	{
		StreamGrabberLockAudioBuffer(pGrabber); locked = true;

		if(!StreamGrabberGetAudioSample(pGrabber)) break;

		r_i = pGrabber->audioBufferReadIndex;

		printf(
			"A t:%lld ch:%d r:%d sc:%d bps:%d addr:%lld size:%d\r\n",
			(pGrabber->audioSampleTimestamps[r_i] - pGrabber->audioStartTimestamp),
			pGrabber->audioSampleChannels[r_i],
			pGrabber->audioSampleRate[r_i],
			pGrabber->audioSampleSampleCounts[r_i],
			pGrabber->audioSampleBitsPerSample[r_i],
			(long long int) pGrabber->pAudioBuffers[r_i],
			pGrabber->audioBufferSizes[r_i]
		);
		
		// copy data to your buffer here

		StreamGrabberUnlockAudioBuffer(pGrabber); locked = false;
		
		// process sample here

	} while (true);

	if (locked) StreamGrabberUnlockAudioBuffer(pGrabber);

	return NULL;
}

