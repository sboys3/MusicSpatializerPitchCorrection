#pragma once

#include "recover.h"
#include "ciglet/lib.h"
#include <malloc.h>
#include <stdlib.h>

namespace stftRecoverLib {
	template <class type>
	type** Continuous2dBuffer(int length, int width, type* rawbuf) {
		type* buffer = rawbuf;
		type** array2d = (type **)calloc(length, sizeof(type*));
		for (size_t x = 0; x < length; x++) {
			array2d[x] = (type*)((size_t)buffer + width * x * sizeof(type));
		}
		return array2d;
	}

	template <class type>
	type** allocContinuous2dBuffer(int length, int width, type** rawbuf) {
		type* buffer = (type*)calloc(length * width, sizeof(type));
		*rawbuf = buffer;
		return Continuous2dBuffer<type>(length, width, buffer);
	}


	template <class type>
	inline type* typedCalloc(size_t size) {
		return (type*)calloc(size, sizeof(type));
	}
	
	void multplyArrayByScaler(float* arr, int size, float scaler) {
		for (int x = 0; x < size; x++) {
			arr[x] = arr[x] * scaler;
		}
	}

	float notesToMultiplier(float notes) {
		return powf(2, notes / 12);
	}

	struct RealtimeBuffer {
		float* samples;
		int length = 0;
		float* phaseState;
		int firsts = 0;
	};


	/*
	=       ==
	==      ===
	===     +--
	+--		====
	====	-+--
	-+--	======
	=====	--++--
	--+--	  ======
	 =====	  --++--
	 --+--	
	*/

	//bufferState should be a reference to a null pointer. It wil be automaticaly initialised on the first run.
	float* realtimePitchShift(float* samples, int samplesLength,float pitchMultiplier, RealtimeBuffer** bufferState,int * outSampleslength) {
		int stepSize = 512;
		int paddingSteps = 2;
		RealtimeBuffer** bufferStateDoublePointer = (RealtimeBuffer * *)bufferState;


		int stftFreqMult = 4;
		int nfft = stepSize * stftFreqMult;
		int stftFrequencies = nfft / 2 + 1;

		RealtimeBuffer* statefulBuffer = *bufferStateDoublePointer;
		if (statefulBuffer == nullptr) {
			statefulBuffer = new RealtimeBuffer();
			*bufferState = statefulBuffer;
			statefulBuffer->length = 0;
			statefulBuffer->firsts = 0;
			statefulBuffer->samples = typedCalloc<float>(stepSize * (paddingSteps * 2 + 1));
			statefulBuffer->phaseState = typedCalloc<float>(stftFrequencies);
		}


		int statefulBufferLength = statefulBuffer->length;
		int availableLength = statefulBufferLength + samplesLength;
		int length = (availableLength) / stepSize * stepSize;
		int stftFrames = length / stepSize;

		float* inBuffer = typedCalloc<float>(availableLength);
		memcpy(inBuffer, statefulBuffer->samples, statefulBufferLength * sizeof(float));
		memcpy(inBuffer + statefulBufferLength, samples, samplesLength * sizeof(float));
		

		int usableSteps = stftFrames - paddingSteps * 2;
		
		
		
		printf("usableSteps: %i availableLength: %i statefulBufferLength: %i samplesLength:%i\n", usableSteps, availableLength, statefulBufferLength, samplesLength);
		

		//int newStatefulBufferLength = std::min<int>(stepSize * (paddingSteps * 2) + availableLength, availableLength);
		int newStatefulBufferLength = std::min<int>(availableLength- stepSize* usableSteps, availableLength);
		//printf("newStatefulBufferLength:%i length:%i\n", newStatefulBufferLength, length);
		memcpy(statefulBuffer->samples, inBuffer + (availableLength - newStatefulBufferLength), newStatefulBufferLength * sizeof(float));
		statefulBuffer->length = newStatefulBufferLength;
		
		*outSampleslength = 0;

		usableSteps += paddingSteps - statefulBuffer->firsts;
		if (statefulBuffer->firsts < paddingSteps && usableSteps > 0) {
			statefulBuffer->firsts = std::min<int>(usableSteps + statefulBuffer->firsts, paddingSteps);
		}

		float* outBuffer = nullptr;
		if (usableSteps > 0) {
			int usableSamples = usableSteps * stepSize;
			*outSampleslength = usableSamples;
			int startStep = stftFrames - usableSteps - paddingSteps;
			int lastStep = startStep + usableSteps - 1;
			float* rawMagnitudeBuffer;
			float** magnitudeBuffer = allocContinuous2dBuffer<float>(stftFrames, stftFrequencies, &rawMagnitudeBuffer);
			float* rawPhaseBuffer;
			float** phaseBuffer = allocContinuous2dBuffer<float>(stftFrames, stftFrequencies, &rawPhaseBuffer);
			float normfc = 0;
			CIGLET::stft(inBuffer, length, stepSize, stftFrames, stftFreqMult, 1, &normfc, NULL, magnitudeBuffer, phaseBuffer);

			frequencyShiftAuto(rawMagnitudeBuffer, stftFrames, stftFrequencies, pitchMultiplier);

			if (startStep - 1 > -1) {
				memcpy(rawPhaseBuffer + stftFrequencies * (startStep - 1), statefulBuffer->phaseState, stftFrequencies * sizeof(float));
			}

			int recoverStartingStep = std::max(startStep - 1, 0);

			recoverPhase(rawMagnitudeBuffer + stftFrequencies * recoverStartingStep, rawPhaseBuffer + stftFrequencies * recoverStartingStep, 1, stftFrames - recoverStartingStep, stftFrequencies);

			memcpy(statefulBuffer->phaseState, rawPhaseBuffer + stftFrequencies * (lastStep), stftFrequencies * sizeof(float));
			
			multplyArrayByScaler(rawPhaseBuffer, stftFrames * stftFrequencies, -1);

			int outLength = 0;
			float* outBufferFull = CIGLET::istft(magnitudeBuffer, phaseBuffer, stepSize, stftFrames, stftFreqMult, 1, stftFreqMult / 2, &outLength);
			outBuffer = typedCalloc<float>(usableSteps * stepSize);

			memcpy(outBuffer, outBufferFull + (startStep * stepSize), usableSamples * sizeof(float));

			free(outBufferFull);
			free(rawMagnitudeBuffer);
			free(rawPhaseBuffer);
		}
		free(inBuffer);

		return outBuffer;
	}

	void freeRealtimeBuffer(RealtimeBuffer** bufferState) {
		RealtimeBuffer** bufferStateDoublePointer = (RealtimeBuffer * *)bufferState;
		RealtimeBuffer* buffer = *bufferStateDoublePointer;
		free(buffer->samples);
		free(buffer->phaseState);
		delete buffer;
		*bufferState = nullptr;
	}
}