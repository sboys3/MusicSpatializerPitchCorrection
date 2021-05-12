#include "stftRecoverLib/recover.h"
#include "stftRecoverLib/stftTools.h"


#if defined(_MSC_VER)
//  Microsoft 
#define EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
//  GCC
#define EXPORT __attribute__((visibility("default")))
#else
//  do nothing and hope for the best?
#define EXPORT
#pragma warning Unknown dynamic link import/export semantics.
#endif


extern "C" {
	
	EXPORT float f() {
		float a = 0;
		CIGLET::cdft(1, 1, &a);
		CIGLET::cftfsub(1, &a);
		CIGLET::bitrv1(1, &a);
		return a;
	}
	/*EXPORT void recoverPhaseLib(float* inArray, float* outArray, long batches, long length, long frequencies)
	{
		stftRecoverLib::recoverPhase(inArray, outArray, batches, length, frequencies);
	}*/
	EXPORT struct RealtimeAllocationChannelContainer {
		stftRecoverLib::RealtimeBuffer* realtimeBuffer;
		float* offsetBuffer;
		int offsetBufferLength;
		int offsetBufferPos;
		int paddingSamplesLeft;
	};

	EXPORT struct RealtimeAllocationContainer {
		RealtimeAllocationChannelContainer* channelContainers;
		int channels = 0;
	};

#define paddingOffset 512
#define paddingOffsetFirst 2 

	EXPORT RealtimeAllocationContainer* InPlacePitchShiftInit(int channels) {
		printf("initializing InPlacePitchShift\n");
		RealtimeAllocationContainer * container = new RealtimeAllocationContainer();
		container->channels = channels;
		container->channelContainers = new RealtimeAllocationChannelContainer[channels];
		for (int channel = 0; channel < channels; channel++) {
			RealtimeAllocationChannelContainer * channelContainer = &container->channelContainers[channel];
			channelContainer->offsetBuffer = stftRecoverLib::typedCalloc<float>(paddingOffset);
			channelContainer->realtimeBuffer = nullptr;
			channelContainer->paddingSamplesLeft = paddingOffset * (paddingOffsetFirst + 1);
			channelContainer->offsetBufferLength = 0;
			channelContainer->offsetBufferPos = 0;
		}
		return container;
	}

	EXPORT void InPlacePitchShiftFree(RealtimeAllocationContainer* container) {
		int channels = container->channels;
		for (int channel = 0; channel < channels; channel++) {
			RealtimeAllocationChannelContainer* channelContainer = &container->channelContainers[channel];
			free(channelContainer->offsetBuffer);
			stftRecoverLib::freeRealtimeBuffer(&channelContainer->realtimeBuffer);
		}
		delete[] container->channelContainers;
		delete container;
	}

	EXPORT void InPlacePitchShiftRun(RealtimeAllocationContainer* container,float * data, int channelLength, float detuneMultiplier) {
		int channels = container->channels;
		for (int channel = 0; channel < channels; channel++) {
			RealtimeAllocationChannelContainer * channelContainer = &container->channelContainers[channel];
			float* inSamples = stftRecoverLib::typedCalloc<float>(channelLength);
			for (int i = 0; i < channelLength; i++) {
				inSamples[i] = data[i * channels + channel];
			}
			int outSamplesLength = 0;
			float* outSamples = stftRecoverLib::realtimePitchShift(inSamples, channelLength, detuneMultiplier, &channelContainer->realtimeBuffer, &outSamplesLength);
			
			int padding = std::min(channelContainer->paddingSamplesLeft, channelLength);
			channelContainer->paddingSamplesLeft = channelContainer->paddingSamplesLeft - padding;

			int outSamplesPos = 0;
			for (int i = padding; i < channelLength; i++) {
				int sampleLocation = i * channels + channel;
				if (channelContainer->offsetBufferLength - channelContainer->offsetBufferPos> 0) {
					data[sampleLocation] = channelContainer->offsetBuffer[channelContainer->offsetBufferPos];
					channelContainer->offsetBufferPos++;
				} else {
					data[sampleLocation] = outSamples[outSamplesPos];
					outSamplesPos++;
				}
			}
			int offsetBufferLengthRemaining = channelContainer->offsetBufferLength - channelContainer->offsetBufferPos;
			if (offsetBufferLengthRemaining > 0) {
				memmove(channelContainer->offsetBuffer, channelContainer->offsetBuffer + channelContainer->offsetBufferPos, offsetBufferLengthRemaining * sizeof(float));
				channelContainer->offsetBufferLength = offsetBufferLengthRemaining;
			} else {
				channelContainer->offsetBufferLength = 0;
			}
			int outSamplesLengthRemaining = outSamplesLength - outSamplesPos;
			if (outSamplesLengthRemaining > 0) {
				memcpy(channelContainer->offsetBuffer + channelContainer->offsetBufferLength, outSamples + outSamplesPos, outSamplesLengthRemaining * sizeof(float));
				channelContainer->offsetBufferLength += outSamplesLengthRemaining;
			}
			channelContainer->offsetBufferPos = 0;


			if (outSamples != nullptr) {
				//memcpy(outSig + locOut, outSamples, outSamplesLength * sizeof(float));
				//locOut += outSamplesLength;
				free(outSamples);
			}
			free(inSamples);
		}
	}

	EXPORT void Noise(float* data, int length) {
		for (int i = 0; i < length; i++) {
			data[i] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) - 0.5f * 1.0f;
		}
	}
	
	EXPORT void DestroyerOfEars(float* data, int length) {
		for (int i = 0; i < length; i++) {
			data[i] = data[i]*1000;
		}
	}
	
	EXPORT void Scilence(float* data, int length) {
		for (int i = 0; i < length; i++) {
			data[i] = data[i]*0;
		}
	}


	EXPORT void InPlacePitchShiftRunUnity(RealtimeAllocationContainer* container, float* data, int channelLength, float detuneMultiplier) {
		int length = channelLength * container->channels;
		float* tmpArray = stftRecoverLib::typedCalloc<float>(length);
		for (int i = 0; i < length; i++) {
			tmpArray[i] = data[i];
		}
		InPlacePitchShiftRun(container, tmpArray, channelLength, detuneMultiplier);
		for (int i = 0; i < length; i++) {
			//if (isnan(tmpArray[i])) {
			//	tmpArray[i] = 0.0f;
			//}
			data[i] = tmpArray[i];
		}
		free(tmpArray);
	}
	



	EXPORT int LICENSE__this_library_is_only_authorized_for_use_in_plugins_for_beatsaber_contact_SBoys3_for_further_information() { return 1; }
}

#define testStftLengthNum 512*16
void testStftLength() {
	float buf[testStftLengthNum];
#define lengthsLength 10
	int lengths[lengthsLength] = { 1, 511, 512, 513, 512+127, 512+128, 512+129, 1024, 2048, 512*16 };
	for (int i = 0; i < lengthsLength; i++) {

	}
}






#include "AudioFile.h"

void realtimeTest2() {

	AudioFile<float> audioFile;

	//audioFile.load("../../nano_section.wav");
	audioFile.load("../../nano-3.wav");
	//audioFile.load("../../../live.wav");
	//audioFile.load("../../../linus.wav");
	//audioFile.load("../../../despacito.wav");

	AudioFile<float>::AudioBuffer outBuffer;
	int channels = audioFile.getNumChannels();
	int numSamples = audioFile.getNumSamplesPerChannel();
	outBuffer.resize(channels);


	

	int channelLength = audioFile.samples[0].size();
	for (int channel = 0; channel < channels; channel++) {
		outBuffer[channel].resize(channelLength);
	}

	int loc = 0;
	float detuneMultiplier = stftRecoverLib::notesToMultiplier(3);

	RealtimeAllocationContainer * stateContainer = InPlacePitchShiftInit(channels);

	while (loc < channelLength) {
		int sectionLength = std::min(rand() % 200000, channelLength - loc);

		float* samples = stftRecoverLib::typedCalloc<float>(channels* sectionLength);
		for (int channel = 0; channel < channels; channel++) {
			float* data = audioFile.samples[channel].data() + loc;
			for (int i = 0; i < sectionLength; i++) {
				samples[i * channels + channel] = data[i];
			}
		}

		InPlacePitchShiftRun(stateContainer, samples, sectionLength, detuneMultiplier);

		for (int channel = 0; channel < channels; channel++) {
			float* data = outBuffer[channel].data() + loc;
			for (int i = 0; i < sectionLength; i++) {
				data[i] = samples[i * channels + channel];
			}
		}
		free(samples);
		loc += sectionLength;
	}

	InPlacePitchShiftFree(stateContainer);

	AudioFile<float> audioFileOut;
	audioFileOut.setAudioBuffer(outBuffer);
	audioFileOut.save("out.wav");
	printf("finished\n");
}

void realtimeTest() {

	AudioFile<float> audioFile;

	//audioFile.load("../../nano_section.wav");
	audioFile.load("../../nano-3.wav");
	//audioFile.load("../../../live.wav");
	//audioFile.load("../../../linus.wav");
	//audioFile.load("../../../despacito.wav");

	AudioFile<float>::AudioBuffer outBuffer;
	int channels = audioFile.getNumChannels();
	int numSamples = audioFile.getNumSamplesPerChannel();
	outBuffer.resize(channels);



	
	for (int channel = 0; channel < channels; channel++) {
		int length = 0;
		float* inSig = audioFile.samples[channel].data();
		length = audioFile.samples[channel].size();

		int stepSize = 512;
		int outLength = (length) / stepSize * stepSize;
		int stftFrames = length / stepSize;

		outBuffer[channel].resize(outLength);
		float* outSig = outBuffer[channel].data();

		stftRecoverLib::RealtimeBuffer* tmpBuf = nullptr;
		int loc = 0;
		int locOut = 0;
		float detuneMultiplier = stftRecoverLib::notesToMultiplier(3);
		while (loc < length) {
			int sectionLength = std::min(rand() % 200000, length - loc);
			int outSamplesLength = 0;
			float * outSamples=stftRecoverLib::realtimePitchShift(inSig + loc, sectionLength, detuneMultiplier, &tmpBuf, &outSamplesLength);
			if (outSamples != nullptr) {
				memcpy(outSig + locOut, outSamples, outSamplesLength * sizeof(float));
				locOut += outSamplesLength;
				free(outSamples);
			}
			loc += sectionLength;
		}
		stftRecoverLib::freeRealtimeBuffer(&tmpBuf);
		printf("realtimePitchShift done\n");
	}

	AudioFile<float> audioFileOut;
	audioFileOut.setAudioBuffer(outBuffer);
	audioFileOut.save("out.wav");
	printf("finished\n");
}


int main(int argc, char* argv[]) {

	realtimeTest2();
	return 0;

	AudioFile<float> audioFile;

	audioFile.load("../../nano.wav");
	//audioFile.load("../../../live.wav");
	//audioFile.load("../../../linus.wav");
	//audioFile.load("../../../despacito.wav");

	AudioFile<float>::AudioBuffer outBuffer;
	int channels = audioFile.getNumChannels();
	int numSamples = audioFile.getNumSamplesPerChannel();
	outBuffer.resize(channels);

	for (int channel = 0; channel < channels; channel++) {
		int length = 0;
		float* inSig = audioFile.samples[channel].data();
		length = audioFile.samples[channel].size();
		int nhop = 512;
		int freqMult = 4;
		int nfft = nhop * freqMult;
		int nfrm = round(length / nhop);
		int frequencies = nfft / 2 + 1;
		FP_TYPE normfc = 0;
		FP_TYPE* rawMagnitudeBuffer;
		FP_TYPE** Xm = stftRecoverLib::allocContinuous2dBuffer<float>(nfrm, frequencies, & rawMagnitudeBuffer);
		FP_TYPE* rawPhaseBuffer;
		FP_TYPE** Xp = stftRecoverLib::allocContinuous2dBuffer<float>(nfrm, frequencies, & rawPhaseBuffer);
		CIGLET::stft(inSig, length, nhop, nfrm, freqMult, 1, &normfc, NULL, Xm, Xp);
		printf("stft done length:%i normfc:%f\n", nfrm, normfc);

		float detuneMultiplier = stftRecoverLib::notesToMultiplier(2);

		//stftRecoverLib::frequencyShift(rawMagnitudeBuffer, nfrm, frequencies, detuneMultiplier);
		//frequencyShift(rawMagnitudeBuffer, nfrm, frequencies, 12);
		//stftRecoverLib::frequencyStretch(rawMagnitudeBuffer, nfrm, frequencies, detuneMultiplier);
		//frequencyShift(rawPhaseBuffer, nfrm, frequencies,-6);

		//for (int x = 0; x < frequencies; x++) {
		//	printf("%f:", Xp[400][x]);
		//}
		stftRecoverLib::recoverPhase(rawMagnitudeBuffer, rawPhaseBuffer, 1, nfrm, frequencies);
		stftRecoverLib::multplyArrayByScaler(rawPhaseBuffer, nfrm * frequencies, -1);
		//inverseArray(rawPhaseBuffer, nfrm * frequencies);
		//minMaxLog(rawMagnitudeBuffer, nfrm * frequencies);
		//minMaxLog(rawPhaseBuffer, nfrm * frequencies);
		printf("recoverPhase done\n", normfc);
		int outLength = 0;
		float* outsig = CIGLET::istft(Xm, Xp, nhop, nfrm, freqMult, 1, freqMult / 2, &outLength);
		printf("istft done length:%fx512\n", (float)(outLength)/512.0f);
		outBuffer[channel].resize(outLength);
		memcpy(outBuffer[channel].data(), outsig, outLength * sizeof(float));
	}

	AudioFile<float> audioFileOut;
	audioFileOut.setAudioBuffer(outBuffer);
	audioFileOut.save("out.wav");
	printf("finished\n");
	//wavwrite(outsig, outLength, 44100, 16,(char*)"out.wav");
	return 0;
}