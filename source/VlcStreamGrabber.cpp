#include <VlcStreamGrabber.h>

void VideoPrerender(void  * pVideoData, uint8_t ** ppPixelBuffer, size_t frameBufferSize);
void AudioPrerender(void  * pAudioData, uint8_t ** ppPcmBuffer, size_t pcmBufferSize);
void VideoPostrender(void * pVideoData, uint8_t *  pPixelBuffer, int width, int height, int pixelPitch, size_t size, mtime_t pts);
void AudioPostrender(void * pAudioData, uint8_t *  pPcmBuffer, unsigned int channels, unsigned int rate, unsigned int samples, unsigned int bitsPerSample, size_t size, mtime_t pts);
void HandleMediaPlayerEvents(const struct libvlc_event_t * pEvent, void * pData);

void ReleaseLocks(VlcStreamGrabber * pGrabber);
void ReleaseAudioLocks(VlcStreamGrabber * pGrabber);
void ReleaseVideoLocks(VlcStreamGrabber * pGrabber);

void VlcStreamGrabberUpdateTrackInfo(VlcStreamGrabber * pGrabber);

int StreamGrabberInit(VlcStreamGrabber ** ppGrabber)
{
	VlcStreamGrabber * pGrabber = NULL;
	bool failed = false;
	char * arszVlcArgs[] = { ""
		"--ignore-config",
		"--quiet",
		//"--verbose=2",
		//"--network-caching=1000",
	};
	
	if (ppGrabber == NULL) return VLC_ENOITEM;
	*ppGrabber = NULL;

	pGrabber = (VlcStreamGrabber *) calloc( 1, sizeof(VlcStreamGrabber));

	if ( pGrabber == NULL ) return VLC_ENOMEM;

	pGrabber->pVlcInstance = libvlc_new(sizeof(arszVlcArgs) / sizeof(arszVlcArgs[0]), arszVlcArgs);
	if (pGrabber->pVlcInstance == NULL) goto FAILED;
	
	vlc_mutex_init(&pGrabber->mutex);

	vlc_sem_init(&pGrabber->videoFrameRequestSemaphore, 0);
	vlc_sem_init(&pGrabber->videoFrameReadySemaphore, 0);
	vlc_sem_init(&pGrabber->audioSampleRequestSemaphore, 0);
	vlc_sem_init(&pGrabber->audioSampleReadySemaphore, 0);

	pGrabber->terminating = false;
	pGrabber->audioStartTimestamp = 0;
	pGrabber->videoStartTimestamp = 0;
	
	pGrabber->audioBufferWasRead[0] = true;
	pGrabber->audioBufferWasRead[1] = true;
	pGrabber->audioBufferWriteIndex = 0;
	pGrabber->audioBufferReadIndex = 1;
	
	pGrabber->videoBufferWasRead[0] = true;
	pGrabber->videoBufferWasRead[1] = true;
	pGrabber->videoBufferWriteIndex = 0;
	pGrabber->videoBufferReadIndex = 1;

	*ppGrabber = pGrabber; pGrabber = NULL;

	goto FINALLY;
FAILED:
	failed = true;
FINALLY:
	if (pGrabber && pGrabber->pVlcInstance) StreamGrabberFree(pGrabber);
	if (pGrabber) free(pGrabber);
	
	if (failed) return VLC_EGENERIC;
	else return VLC_SUCCESS;
}

void ReleaseAudioLocks(VlcStreamGrabber * pGrabber)
{
	if(pGrabber)
	{
		vlc_sem_post(&pGrabber->audioSampleRequestSemaphore);
		vlc_sem_post(&pGrabber->audioSampleReadySemaphore);
	}
}

void ReleaseVideoLocks(VlcStreamGrabber * pGrabber)
{
	if(pGrabber)
	{
		vlc_sem_post(&pGrabber->videoFrameRequestSemaphore);
		vlc_sem_post(&pGrabber->videoFrameReadySemaphore);
	}
}

void ReleaseLocks(VlcStreamGrabber * pGrabber)
{
	if(pGrabber)
	{
		pGrabber->terminating = true;
		ReleaseVideoLocks(pGrabber);
		ReleaseAudioLocks(pGrabber);
	}
}

void StreamGrabberFree( VlcStreamGrabber * pGrabber )
{
	libvlc_event_manager_t * pEventManager = NULL;

	if (pGrabber != NULL)
	{
		ReleaseLocks(pGrabber);

		if (pGrabber->pVlcMediaPlayer)
		{
			libvlc_media_player_stop(pGrabber->pVlcMediaPlayer);

			pEventManager = libvlc_media_player_event_manager(pGrabber->pVlcMediaPlayer);
			libvlc_event_detach(pEventManager, libvlc_MediaPlayerEncounteredError, HandleMediaPlayerEvents, pGrabber);
			libvlc_event_detach(pEventManager, libvlc_MediaPlayerPlaying, HandleMediaPlayerEvents, pGrabber);
			libvlc_event_detach(pEventManager, libvlc_MediaPlayerStopped, HandleMediaPlayerEvents, pGrabber);
			libvlc_event_detach(pEventManager, libvlc_MediaPlayerTimeChanged, HandleMediaPlayerEvents, pGrabber);
			libvlc_event_detach(pEventManager, libvlc_MediaPlayerEndReached, HandleMediaPlayerEvents, pGrabber);
			
			libvlc_media_player_release(pGrabber->pVlcMediaPlayer);
			pGrabber->pVlcMediaPlayer = NULL;
		}

		vlc_mutex_destroy(&pGrabber->mutex);
		
		vlc_sem_destroy(&pGrabber->videoFrameRequestSemaphore);
		vlc_sem_destroy(&pGrabber->videoFrameReadySemaphore);
		vlc_sem_destroy(&pGrabber->audioSampleRequestSemaphore);
		vlc_sem_destroy(&pGrabber->audioSampleReadySemaphore);

		libvlc_release(pGrabber->pVlcInstance);
		
		free(pGrabber->pAudioBuffers[0]);
		free(pGrabber->pAudioBuffers[1]);
		free(pGrabber->pVideoBuffers[0]);
		free(pGrabber->pVideoBuffers[1]);

		free(pGrabber);
	}
}

int StreamGrabberSetMedia(VlcStreamGrabber * pGrabber, libvlc_media_t * pMedia, bool paceControl)
{
	char vcodec[] = "RV32";
	char acodec[] = "s16l";
	char pszOptions[1024] = {0};
	libvlc_event_manager_t * pEventManager = NULL;

	if (pGrabber == NULL || pMedia == NULL) return VLC_ENOITEM;	

	sprintf_s( 
		pszOptions,
		1024,
		":sout=#transcode{"
			 "vcodec=%s,"
			 "acodec=%s,"
			 "threads=2,"
		"}:smem{"
			"%s,"
			"audio-prerender-callback=%lld,"
			"audio-postrender-callback=%lld,"
			"video-prerender-callback=%lld,"
			"video-postrender-callback=%lld,"
			"audio-data=%lld,"
			"video-data=%lld"
		"}",
		vcodec,
		acodec,
		(paceControl ? "time-sync" : "no-time-sync"),
		(long long int)(intptr_t)(void*) &AudioPrerender,
		(long long int)(intptr_t)(void*) &AudioPostrender,
		(long long int)(intptr_t)(void*) &VideoPrerender,
		(long long int)(intptr_t)(void*) &VideoPostrender,
		(long long int)(intptr_t)(void*) pGrabber,
		(long long int)(intptr_t)(void*) pGrabber
	);

	libvlc_media_add_option(pMedia, pszOptions);
	
	if (pGrabber->pVlcMediaPlayer)
	{
		libvlc_media_player_release(pGrabber->pVlcMediaPlayer);
	}
	pGrabber->pVlcMediaPlayer = libvlc_media_player_new_from_media(pMedia);

	pEventManager = libvlc_media_player_event_manager(pGrabber->pVlcMediaPlayer);
	libvlc_event_attach(pEventManager, libvlc_MediaPlayerEncounteredError, HandleMediaPlayerEvents, pGrabber);
	libvlc_event_attach(pEventManager, libvlc_MediaPlayerPlaying, HandleMediaPlayerEvents, pGrabber);
	libvlc_event_attach(pEventManager, libvlc_MediaPlayerStopped, HandleMediaPlayerEvents, pGrabber);
	libvlc_event_attach(pEventManager, libvlc_MediaPlayerTimeChanged, HandleMediaPlayerEvents, pGrabber);
	libvlc_event_attach(pEventManager, libvlc_MediaPlayerEndReached, HandleMediaPlayerEvents, pGrabber);
	
	pEventManager = libvlc_media_event_manager(pMedia);
	libvlc_event_attach(pEventManager, libvlc_MediaParsedChanged, HandleMediaPlayerEvents, pGrabber);
	
	if (!libvlc_media_is_parsed(pMedia)) libvlc_media_parse(pMedia);
	
	if (pGrabber->pVlcMediaPlayer)
	{
		pGrabber->paceControl = paceControl;
		return VLC_SUCCESS;
	}
	else return VLC_EGENERIC;
}

bool StreamGrabberGetVideoFrame(VlcStreamGrabber * pGrabber)
{
	int r_i, w_i;
	if (pGrabber && !pGrabber->terminating)
	{
		if (pGrabber->paceControl)
		{
			bool simpleGet = false;;
			
			vlc_mutex_lock(&pGrabber->mutex);
			r_i = pGrabber->videoBufferReadIndex;
			w_i = pGrabber->videoBufferWriteIndex;

			if (!pGrabber->videoBufferWriting && 
				pGrabber->videoFrameTimestamps[w_i] > pGrabber->videoFrameTimestamps[r_i] &&
				!pGrabber->videoBufferWasRead[w_i])
			{
				pGrabber->videoBufferReadIndex = w_i;
				pGrabber->videoBufferWriteIndex = r_i;
				
				r_i = pGrabber->videoBufferReadIndex;
				w_i = pGrabber->videoBufferWriteIndex;
			}

			simpleGet = !pGrabber->videoBufferWasRead[r_i];

			vlc_mutex_unlock(&pGrabber->mutex);

			if (!simpleGet)
			{
				vlc_mutex_lock(&pGrabber->mutex);
				pGrabber->videoFrameRequested = true;
				vlc_mutex_unlock(&pGrabber->mutex);

				vlc_sem_wait(&pGrabber->videoFrameReadySemaphore);
				if (pGrabber->terminating || pGrabber->noVideo) return false;
			}
		}
		else
		{
			vlc_sem_post(&pGrabber->videoFrameRequestSemaphore);
			vlc_sem_wait(&pGrabber->videoFrameReadySemaphore);
			if (pGrabber->terminating || pGrabber->noVideo) return false;
		}
			
		vlc_mutex_lock(&pGrabber->mutex);
		r_i = pGrabber->videoBufferReadIndex;
		pGrabber->videoBufferWasRead[r_i] = true;
		vlc_mutex_unlock(&pGrabber->mutex);

		return true;
	}
	
	return false;
}

bool StreamGrabberGetAudioSample(VlcStreamGrabber * pGrabber)
{
	int r_i, w_i;
	if (pGrabber && !pGrabber->terminating)
	{		
		if (pGrabber->paceControl)
		{
			bool simpleGet = false;;
			
			vlc_mutex_lock(&pGrabber->mutex);
			r_i = pGrabber->audioBufferReadIndex;
			w_i = pGrabber->audioBufferWriteIndex;

			if (!pGrabber->audioBufferWriting && 
				pGrabber->audioSampleTimestamps[w_i] > pGrabber->audioSampleTimestamps[r_i] &&
				!pGrabber->audioBufferWasRead[w_i])
			{
				pGrabber->audioBufferReadIndex = w_i;
				pGrabber->audioBufferWriteIndex = r_i;
				
				r_i = pGrabber->audioBufferReadIndex;
				w_i = pGrabber->audioBufferWriteIndex;
			}

			simpleGet = !pGrabber->audioBufferWasRead[r_i];
			
			vlc_mutex_unlock(&pGrabber->mutex);
			
			if (!simpleGet)
			{
				vlc_mutex_lock(&pGrabber->mutex);
				pGrabber->audioSampleRequested = true;
				vlc_mutex_unlock(&pGrabber->mutex);

				vlc_sem_wait(&pGrabber->audioSampleReadySemaphore);
				if (pGrabber->terminating || pGrabber->noAudio) return false;
			}
		}
		else
		{
			vlc_sem_post(&pGrabber->audioSampleRequestSemaphore);
			vlc_sem_wait(&pGrabber->audioSampleReadySemaphore);
			if (pGrabber->terminating || pGrabber->noAudio) return false;
		}
			
		vlc_mutex_lock(&pGrabber->mutex);
		r_i = pGrabber->audioBufferReadIndex;
		pGrabber->audioBufferWasRead[r_i] = true;
		vlc_mutex_unlock(&pGrabber->mutex);

		return true;
	}
	else
	{
		return false;
	}
}

void StreamGrabberLockAudioBuffer(VlcStreamGrabber * pGrabber)
{
	vlc_mutex_lock( &pGrabber->mutex);
	pGrabber->audioBufferLocked = true;
	vlc_mutex_unlock( &pGrabber->mutex);
}

void StreamGrabberUnlockAudioBuffer(VlcStreamGrabber * pGrabber)
{
	vlc_mutex_lock( &pGrabber->mutex);
	pGrabber->audioBufferLocked = false;
	vlc_mutex_unlock( &pGrabber->mutex);
}

void StreamGrabberLockVideoBuffer(VlcStreamGrabber * pGrabber)
{
	vlc_mutex_lock( &pGrabber->mutex);
	pGrabber->videoBufferLocked = true;
	vlc_mutex_unlock( &pGrabber->mutex);
}

void StreamGrabberUnlockVideoBuffer(VlcStreamGrabber * pGrabber)
{
	vlc_mutex_lock( &pGrabber->mutex);
	pGrabber->videoBufferLocked = false;
	vlc_mutex_unlock( &pGrabber->mutex);
}


void VlcStreamGrabberUpdateTrackInfo(VlcStreamGrabber * pGrabber)
{
	unsigned int i;
	unsigned int trackCount = 0;
	libvlc_media_t * pMedia;
	libvlc_media_track_t ** ppTracks = NULL;
	int w_vi, w_ai;

	if (pGrabber && pGrabber->pVlcMediaPlayer)
	{
		pMedia = libvlc_media_player_get_media(pGrabber->pVlcMediaPlayer);
		if (pMedia)
		{
			trackCount = libvlc_media_tracks_get(pMedia, &ppTracks);
			
			vlc_mutex_lock( &pGrabber->mutex);

			if (trackCount > 0)
			{
				pGrabber->noAudio = true;
				pGrabber->noVideo = true;
			}

			w_vi = pGrabber->videoBufferWriteIndex;
			w_ai = pGrabber->audioBufferWriteIndex;

			for (i = 0; i < trackCount; i++) 
			{
				if (ppTracks[i]->i_type == libvlc_track_video)
				{
					pGrabber->videoTracks[w_vi] = *ppTracks[i]->video;
					pGrabber->noVideo = false;
				}
				else if (ppTracks[i]->i_type == libvlc_track_audio)
				{
					pGrabber->audioTracks[w_ai] = *ppTracks[i]->audio;
					pGrabber->noAudio = false;
				}
			}

			vlc_mutex_unlock( &pGrabber->mutex);

			libvlc_media_tracks_release(ppTracks, trackCount);
		}
	
		if (pGrabber->noVideo)
		{
			ReleaseVideoLocks(pGrabber);
		}
	
		if (pGrabber->noAudio)
		{
			ReleaseAudioLocks(pGrabber);
		}
	}
}

void AudioPrerender(void * pAudioData, uint8_t ** ppPcmBuffer, size_t pcmBufferSize)
{
	VlcStreamGrabber * pGrabber = (VlcStreamGrabber *) pAudioData;
	if (!pGrabber || pGrabber->terminating || pGrabber->noAudio)
	{
		*ppPcmBuffer = NULL;
		return;
	}
	
	vlc_mutex_lock(&pGrabber->mutex);
	int w_i = pGrabber->audioBufferWriteIndex;
	int r_i = pGrabber->audioBufferReadIndex;
	pGrabber->audioBufferWriting = true;
	vlc_mutex_unlock(&pGrabber->mutex);

	if (pGrabber->audioAllocatedBufferSizes[w_i] < pcmBufferSize)
	{
		pGrabber->pAudioBuffers[w_i] = (uint8_t*) realloc(pGrabber->pAudioBuffers[w_i], pcmBufferSize * sizeof(uint8_t));
		pGrabber->audioAllocatedBufferSizes[w_i] = pcmBufferSize;
	}

	*ppPcmBuffer = pGrabber->pAudioBuffers[w_i];
}

void VideoPrerender(void  * pVideoData, uint8_t ** ppPixelBuffer, size_t frameBufferSize)
{
	VlcStreamGrabber * pGrabber = (VlcStreamGrabber *) pVideoData;
	if (!pGrabber || pGrabber->terminating || pGrabber->noVideo)
	{
		*ppPixelBuffer = NULL;
		return;
	}
	
	vlc_mutex_lock(&pGrabber->mutex);
	int w_i = pGrabber->videoBufferWriteIndex;
	int r_i = pGrabber->videoBufferReadIndex;
	pGrabber->videoBufferWriting = true;
	vlc_mutex_unlock(&pGrabber->mutex);

	if (pGrabber->videoAllocatedBufferSizes[w_i] < frameBufferSize)
	{
		pGrabber->pVideoBuffers[w_i] = (uint8_t*) realloc(pGrabber->pVideoBuffers[w_i], frameBufferSize * sizeof(uint8_t));
		pGrabber->videoAllocatedBufferSizes[w_i] = frameBufferSize;
	}

	*ppPixelBuffer = pGrabber->pVideoBuffers[w_i];
}

void AudioPostrender(void * pAudioData, uint8_t * pPcmBuffer, unsigned int channels, unsigned int rate, unsigned int samples, unsigned int bitsPerSample, size_t size, mtime_t pts)
{
	VlcStreamGrabber * pGrabber = (VlcStreamGrabber *) pAudioData;
	if (pGrabber == NULL || pGrabber->terminating || pGrabber->noAudio) return;

	vlc_mutex_lock(&pGrabber->mutex);
	int w_i = pGrabber->audioBufferWriteIndex;
	int r_i = pGrabber->audioBufferReadIndex;
	vlc_mutex_unlock(&pGrabber->mutex);
	
	if (!pGrabber->audioStartTimestamp)
	{
		pGrabber->audioStartTimestamp = pts;;
	}
	
	pGrabber->audioSampleTimestamps[w_i] = pts;
	pGrabber->audioSampleChannels[w_i] = channels;
	pGrabber->audioSampleRate[w_i] = rate;
	pGrabber->audioSampleSampleCounts[w_i] = samples;
	pGrabber->audioSampleBitsPerSample[w_i] = bitsPerSample;
	pGrabber->audioBufferSizes[w_i] = size;

	vlc_mutex_lock(&pGrabber->mutex);
	pGrabber->audioBufferWasRead[w_i] = false;
	vlc_mutex_unlock(&pGrabber->mutex);
	
	if (!pGrabber->paceControl)
	{
		vlc_sem_wait(&pGrabber->audioSampleRequestSemaphore);
		if (pGrabber->terminating || pGrabber->noAudio) return;

		vlc_mutex_lock(&pGrabber->mutex);
		pGrabber->audioTracks[r_i] = pGrabber->audioTracks[w_i];

		pGrabber->audioBufferWriteIndex = r_i;
		pGrabber->audioBufferReadIndex = w_i;
		vlc_mutex_unlock(&pGrabber->mutex);
		
		vlc_sem_post(&pGrabber->audioSampleReadySemaphore);
	}
	else 
	{
		vlc_mutex_lock(&pGrabber->mutex);

		if (pGrabber->audioSampleRequested)
		{
			pGrabber->audioTracks[r_i] = pGrabber->audioTracks[w_i];

			pGrabber->audioBufferWriteIndex = r_i;
			pGrabber->audioBufferReadIndex = w_i;

			pGrabber->audioSampleRequested = false;

			vlc_sem_post(&pGrabber->audioSampleReadySemaphore);
		}
		else if (!pGrabber->audioBufferLocked)
		{
			pGrabber->audioBufferWriteIndex = r_i;
			pGrabber->audioBufferReadIndex = w_i;
		}
		else
		{
			printf("Audio sample dropped.\r\n");
		}

		vlc_mutex_unlock(&pGrabber->mutex);
	}

	vlc_mutex_lock(&pGrabber->mutex);
	pGrabber->audioBufferWriting = false;
	vlc_mutex_unlock(&pGrabber->mutex);
}

void VideoPostrender(void * pVideoData, uint8_t *  pPixelBuffer, int width, int lines, int pixelPitch, size_t size, mtime_t pts)
{
	VlcStreamGrabber * pGrabber = (VlcStreamGrabber *) pVideoData;
	if (pGrabber == NULL || pGrabber->terminating || pGrabber->noVideo) return;
	
	vlc_mutex_lock(&pGrabber->mutex);
	int w_i = pGrabber->videoBufferWriteIndex;
	int r_i = pGrabber->videoBufferReadIndex;
	vlc_mutex_unlock(&pGrabber->mutex);

	if (!pGrabber->videoStartTimestamp)
	{
		pGrabber->videoStartTimestamp = pts;;
	}

	video_format_Setup(&pGrabber->videoFormats[w_i], VLC_CODEC_RGB32, width, lines, pGrabber->videoTracks[w_i].i_width, pGrabber->videoTracks[w_i].i_height, 1, 1);
	pGrabber->videoFrameTimestamps[w_i] = pts;
	pGrabber->videoBufferSizes[w_i] = size;
	pGrabber->videoFrameBitsPerPixel[w_i] = pixelPitch;
	pGrabber->videoFrameWidth[w_i] = width;
	pGrabber->videoFrameLines[w_i] = lines;
	
	vlc_mutex_lock(&pGrabber->mutex);
	pGrabber->videoBufferWasRead[w_i] = false;
	vlc_mutex_unlock(&pGrabber->mutex);
	
	if (!pGrabber->paceControl)
	{
		vlc_sem_wait(&pGrabber->videoFrameRequestSemaphore);
		if (pGrabber->terminating || pGrabber->noVideo) return;
		
		vlc_mutex_lock(&pGrabber->mutex);
		pGrabber->videoTracks[r_i] = pGrabber->videoTracks[w_i];

		pGrabber->videoBufferWriteIndex = r_i;
		pGrabber->videoBufferReadIndex = w_i;

		vlc_mutex_unlock(&pGrabber->mutex);
			
		vlc_sem_post(&pGrabber->videoFrameReadySemaphore);
	}
	else
	{
		vlc_mutex_lock(&pGrabber->mutex);

		if (pGrabber->videoFrameRequested)
		{
			pGrabber->videoTracks[r_i] = pGrabber->videoTracks[w_i];

			pGrabber->videoBufferWriteIndex = r_i;
			pGrabber->videoBufferReadIndex = w_i;

			pGrabber->videoFrameRequested = false;
			
			vlc_sem_post(&pGrabber->videoFrameReadySemaphore);
		}
		else if (!pGrabber->videoBufferLocked)
		{
			pGrabber->videoBufferWriteIndex = r_i;
			pGrabber->videoBufferReadIndex = w_i;
		}
		else
		{
			printf("Video frame dropped.\r\n");
		}

		vlc_mutex_unlock(&pGrabber->mutex);
	}

	vlc_mutex_lock(&pGrabber->mutex);
	pGrabber->videoBufferWriting = false;
	vlc_mutex_unlock(&pGrabber->mutex);
}

void HandleMediaPlayerEvents(const struct libvlc_event_t * pEvent, void * pData)
{
	VlcStreamGrabber * pGrabber = (VlcStreamGrabber *) pData;
	if (pGrabber == NULL) return;

	bool releaseLocks = pGrabber->terminating;
	const char * szError = NULL;

	switch (pEvent->type)
	{
	case libvlc_MediaPlayerEncounteredError:
		releaseLocks = true;
		szError = libvlc_errmsg();
		if (!szError) szError = "libVLC media player encountered error";
		printf(szError);
		break;
	case libvlc_MediaPlayerStopped:
	case libvlc_MediaPlayerEndReached:
		releaseLocks = true;
		break;
	case libvlc_MediaPlayerPlaying:
		break;
	case libvlc_MediaParsedChanged:
		if (pEvent->u.media_parsed_changed.new_status & ITEM_PREPARSED ||
			pEvent->u.media_parsed_changed.new_status & ITEM_ART_FETCHED)
			VlcStreamGrabberUpdateTrackInfo(pGrabber);
		break;
	case libvlc_MediaPlayerTimeChanged:
		break;
	}

	if (releaseLocks) ReleaseLocks(pGrabber);;
}
