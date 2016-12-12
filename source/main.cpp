#include "VlcStreamGrabber.h"

void * VideoFrameReader(void * pData);
void * AudioSampleReader(void * pData);

int main(int argc, char* argv[])
{
	vlc_thread_t videoFrameReaderThread;
	vlc_thread_t audioSampleReaderThread;
	libvlc_media_t * pMedia;
	bool paceControl = false;

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
	int r_i;

	while(StreamGrabberGetVideoFrame(pGrabber))
	{
		r_i = pGrabber->audioBufferReadIndex;
		
		pGrabber->pVideoBuffers[r_i];

		printf(
			"V t:%lld w:%d h:%d l:%d bpp:%d fr:%d/%d\r\n",
			(pGrabber->videoFrameTimestamps[r_i] - pGrabber->videoStartTimestamp) / 1000,
			pGrabber->videoFormats[r_i].i_visible_width,
			pGrabber->videoFormats[r_i].i_visible_height,
			pGrabber->videoFormats[r_i].i_height,
			pGrabber->videoFrameBitsPerPixel[r_i],
			pGrabber->videoTracks[r_i].i_frame_rate_num,
			pGrabber->videoTracks[r_i].i_frame_rate_den
		);
	}

	return NULL;
}

void * AudioSampleReader(void * pData)
{
	VlcStreamGrabber * pGrabber = (VlcStreamGrabber *) pData;
	int r_i;

	while(StreamGrabberGetAudioSample(pGrabber))
	{
		r_i = pGrabber->audioBufferReadIndex;

		printf(
			"A t:%lld c:%d r:%d s:%d bps:%d\r\n",
			(pGrabber->audioSampleTimestamps[r_i] - pGrabber->audioStartTimestamp) / 1000,
			pGrabber->audioSampleChannels[r_i],
			pGrabber->audioSampleRate[r_i],
			pGrabber->audioSampleSampleCounts[r_i],
			pGrabber->audioSampleBitsPerSample[r_i]
		);
	}

	return NULL;
}

