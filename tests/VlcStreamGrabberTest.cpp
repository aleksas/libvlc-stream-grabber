#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace VlcSteamGrabberTests
{		
	TEST_CLASS(VlcSteamGrabberUnitTest)
	{
	public:

		static void * VideoFrameReader(void * pData)
		{
			VlcStreamGrabber * pGrabber = (VlcStreamGrabber *)pData;
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

			} while (true);

			if (locked) StreamGrabberUnlockVideoBuffer(pGrabber);

			return NULL;
		}

		static void * AudioSampleReader(void * pData)
		{
			VlcStreamGrabber * pGrabber = (VlcStreamGrabber *)pData;
			bool locked;
			int r_i;

			do
			{
				StreamGrabberLockAudioBuffer(pGrabber); locked = true;

				if (!StreamGrabberGetAudioSample(pGrabber)) break;

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

		TEST_METHOD(VlcSteamGrabberInitTest)
		{
			int result = VLC_SUCCESS;

			VlcStreamGrabber * pGrabber = NULL;

			result = StreamGrabberInit(&pGrabber);
			
			Assert::AreEqual(VLC_SUCCESS, result, L"StreamGrabberInit() failed", LINE_INFO());

			if (pGrabber) StreamGrabberFree(pGrabber);
		}

		TEST_METHOD(VlcSteamGrabberInitMediaTest)
		{
			int result = VLC_SUCCESS;
			libvlc_media_t * pMedia;
			bool paceControl = true;

			VlcStreamGrabber * pGrabber = NULL;

			result = StreamGrabberInit(&pGrabber);

			Assert::AreEqual(VLC_SUCCESS, result, L"StreamGrabberInit() failed", LINE_INFO());

			pMedia = libvlc_media_new_path(pGrabber->pVlcInstance, Data::SampleVideoPath.c_str());

			Assert::AreNotEqual<void*>(NULL, pMedia, L"libvlc_media_new_path() failed", LINE_INFO());

			result = StreamGrabberSetMedia(pGrabber, pMedia, paceControl);

			Assert::AreEqual(VLC_SUCCESS, result, L"StreamGrabberSetMedia() failed", LINE_INFO());

			if (pMedia) libvlc_media_release(pMedia); pMedia = NULL;

			if (pGrabber) StreamGrabberFree(pGrabber);
		}

		TEST_METHOD(VlcSteamGrabberReadVideoTest)
		{
			vlc_thread_t videoFrameReaderThread;
			vlc_thread_t audioSampleReaderThread;
			libvlc_media_t * pMedia;
			void * pVideoThreadResult;
			void * pAudioThreadResult;

			int result = VLC_SUCCESS;
			bool paceControl = true;

			VlcStreamGrabber * pGrabber = NULL;

			result = StreamGrabberInit(&pGrabber);
			Assert::AreEqual(VLC_SUCCESS, result, L"StreamGrabberInit() failed", LINE_INFO());

			pMedia = libvlc_media_new_path(pGrabber->pVlcInstance, Data::SampleVideoPath.c_str());
			Assert::AreNotEqual<void*>(NULL, pMedia, L"libvlc_media_new_path() failed", LINE_INFO());

			result = StreamGrabberSetMedia(pGrabber, pMedia, paceControl);
			Assert::AreEqual(VLC_SUCCESS, result, L"StreamGrabberSetMedia() failed", LINE_INFO());

			if (pMedia) libvlc_media_release(pMedia); pMedia = NULL;

			result = vlc_clone(&videoFrameReaderThread, &this->VideoFrameReader, pGrabber, 0);
			Assert::AreEqual(VLC_SUCCESS, result, L"libvlc_media_player_play() failed", LINE_INFO());

			result = vlc_clone(&audioSampleReaderThread, &this->AudioSampleReader, pGrabber, 0);
			Assert::AreEqual(VLC_SUCCESS, result, L"libvlc_media_player_play() failed", LINE_INFO());

			result = libvlc_media_player_play(pGrabber->pVlcMediaPlayer);
			Assert::AreEqual(VLC_SUCCESS, result, L"libvlc_media_player_play() failed", LINE_INFO());

			vlc_join(videoFrameReaderThread, &pVideoThreadResult);
			vlc_join(audioSampleReaderThread, &pAudioThreadResult);

			if (pGrabber) StreamGrabberFree(pGrabber);
		}


		TEST_METHOD(VlcSteamGrabberReadAudioTest)
		{
			vlc_thread_t audioSampleReaderThread;
			libvlc_media_t * pMedia;
			void * pAudioThreadResult;

			int result = VLC_SUCCESS;
			bool paceControl = true;

			VlcStreamGrabber * pGrabber = NULL;

			result = StreamGrabberInit(&pGrabber);
			Assert::AreEqual(VLC_SUCCESS, result, L"StreamGrabberInit() failed", LINE_INFO());

			pMedia = libvlc_media_new_path(pGrabber->pVlcInstance, Data::SampleVideoPath.c_str());
			Assert::AreNotEqual<void*>(NULL, pMedia, L"libvlc_media_new_path() failed", LINE_INFO());

			result = StreamGrabberSetMedia(pGrabber, pMedia, paceControl);
			Assert::AreEqual(VLC_SUCCESS, result, L"StreamGrabberSetMedia() failed", LINE_INFO());

			if (pMedia) libvlc_media_release(pMedia); pMedia = NULL;

			result = vlc_clone(&audioSampleReaderThread, &this->AudioSampleReader, pGrabber, 0);
			Assert::AreEqual(VLC_SUCCESS, result, L"libvlc_media_player_play() failed", LINE_INFO());

			result = libvlc_media_player_play(pGrabber->pVlcMediaPlayer);
			Assert::AreEqual(VLC_SUCCESS, result, L"libvlc_media_player_play() failed", LINE_INFO());

			vlc_join(audioSampleReaderThread, &pAudioThreadResult);

			if (pGrabber) StreamGrabberFree(pGrabber);
		}

	};
}