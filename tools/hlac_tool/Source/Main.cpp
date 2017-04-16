/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#define COMPRESSION_BLOCK_SIZE 4096

#include "HiseLosslessAudioFormat.h"

class StdLogger : public Logger
{
public:

	void logMessage(const String &message) override
	{
#if JUCE_DEBUG
		DBG(message);
#else
		NewLine nl;
		std::cout << message << nl;
#endif
	}
};

int main(int argc, char **argv)
{
	

	ScopedPointer<Logger> l = new StdLogger();

	Logger::setCurrentLogger(l);

	if (argc != 2)
	{
		Logger::writeToLog("HISE Lossless Audio Codec Test tool");
		Logger::writeToLog("-----------------------------------");
		Logger::writeToLog("Usage: hlac_tool [FOLDER_WITH_TEST_FILES]");
		Logger::writeToLog("(put '_' before filename to skip samples)");
		Logger::setCurrentLogger(nullptr);

		return 1;
	}

	UnitTestRunner runner;
	runner.setAssertOnFailure(false);

	//runner.runAllTests();

	File root(argv[1]);

	Array<File> testSamples;

	root.findChildFiles(testSamples, File::findFiles, true);

	bool useBlock = true;
	bool useDelta = true;
	bool useDiff = true;
	bool checkWithFlac = true;

	double blockRatio = 0.0f;
	double deltaRatio = 0.0f;
	double diffRatio = 0.0f;
	double flacRatio = 0.0f;

	double blockSpeed = 0.0;
	double flacSpeed = 0.0;
	double pcmSpeed = 0.0;
	double deltaSpeed = 0.0;
	double diffSpeed = 0.0;

	double r;
	double s;

	int numFilesChecked = 0;

	for (auto f : testSamples)
	{
		if (f.getFileName().startsWith("_"))
			continue;

		++numFilesChecked;

		Logger::writeToLog("");
		Logger::writeToLog("Compressing file " + f.getFileName());
		Logger::writeToLog("--------------------------------------------------------------------");

		AudioSampleBuffer b;

		try
		{
			b = CompressionHelpers::loadFile(f, s);
		}
		catch (String error)
		{
			Logger::writeToLog(error);
			Logger::setCurrentLogger(nullptr);
			return 1;
		}
		

#if 0

		b.clear();

		float* check = b.getWritePointer(0);

		check[1012] = 0.0f;
		check[1013] = 0.251f;
		check[1014] = 0.51f;
		check[1015] = 0.752f;
		check[1016] = 1.0f;
		check[1017] = 0.751f;
		check[1018] = 0.49f;
		check[1019] = 0.253f;
		check[1020] = 0.0f;
		check[1021] = 0.32f;
		check[1022] = 0.68f;
		check[1023] = 1.0f;

		CompressionHelpers::dump(b);

	
#endif
		pcmSpeed += s;

		if (checkWithFlac)
		{
			auto fr = CompressionHelpers::getFLACRatio(f, s);

			flacSpeed += s;
			flacRatio += fr;

			Logger::writeToLog("Compressing with FLAC:  " + String(fr, 3));

		}

		
		MemoryOutputStream mos;

		HlacEncoder encoder;
		HlacDecoder decoder;

		HiseLosslessAudioFormat hlac;

		StringPairArray emptyMetadata;

		if (useBlock)
		{
			MemoryOutputStream* blockMos = new MemoryOutputStream();

			ScopedPointer<HiseLosslessAudioFormatWriter> blockWriter = dynamic_cast<HiseLosslessAudioFormatWriter*>(hlac.createWriterFor(blockMos, 44100, b.getNumChannels(), 16, emptyMetadata, 5));
			
			if (blockWriter == nullptr)
				return 1;

			HlacEncoder::CompressorOptions wholeBlock;

			wholeBlock.fixedBlockWidth = 512;
			wholeBlock.removeDcOffset = false;
			wholeBlock.useDeltaEncoding = false;
			wholeBlock.useDiffEncodingWithFixedBlocks = false;

			blockWriter->setOptions(wholeBlock);
			blockWriter->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
			
			r = blockWriter->getCompressionRatioForLastFile();

			AudioSampleBuffer b2(b.getNumChannels(), CompressionHelpers::getPaddedSampleSize(b.getNumSamples()));
			MemoryInputStream* blockMis = new MemoryInputStream(blockMos->getMemoryBlock(), true);
			ScopedPointer<HiseLosslessAudioFormatReader> blockReader = dynamic_cast<HiseLosslessAudioFormatReader*>(hlac.createReaderFor(blockMis, false));

			blockReader->read(&b2, 0, b2.getNumSamples(), 0, true, true);

			blockSpeed += blockReader->getDecompressionPerformanceForLastFile();
			CompressionHelpers::checkBuffersEqual(b2, b);

			Logger::writeToLog("Compressing with blocks: " + String(r, 3));

			blockRatio += r;

			blockWriter = nullptr;
			blockReader = nullptr;
		}

		if (useDelta)
		{
			MemoryOutputStream* deltaMos = new MemoryOutputStream();
			ScopedPointer<HiseLosslessAudioFormatWriter> deltaWriter = dynamic_cast<HiseLosslessAudioFormatWriter*>(hlac.createWriterFor(deltaMos, 44100, b.getNumChannels(), 16, emptyMetadata, 5));

			if (deltaWriter == nullptr)
				return 1;


			HlacEncoder::CompressorOptions delta;

			delta.fixedBlockWidth = -1;
			delta.removeDcOffset = false;
			delta.useDeltaEncoding = true;
			delta.useDiffEncodingWithFixedBlocks = false;
			delta.reuseFirstCycleLengthForBlock = true;
			delta.deltaCycleThreshhold = 0.1f;

			deltaWriter->setOptions(delta);

			deltaWriter->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());

			r = deltaWriter->getCompressionRatioForLastFile();


			AudioSampleBuffer b3(b.getNumChannels(), CompressionHelpers::getPaddedSampleSize(b.getNumSamples()));
			
			MemoryInputStream* deltaMis = new MemoryInputStream(deltaMos->getMemoryBlock(), true);
			ScopedPointer<HiseLosslessAudioFormatReader> deltaReader = dynamic_cast<HiseLosslessAudioFormatReader*>(hlac.createReaderFor(deltaMis, false));

			deltaReader->read(&b3, 0, b3.getNumSamples(), 0, true, true);

			deltaSpeed += deltaReader->getDecompressionPerformanceForLastFile();

			CompressionHelpers::checkBuffersEqual(b3, b);

			Logger::writeToLog("Compressing with delta:  " + String(r, 3));

			deltaRatio += r;
			mos.reset();
			encoder.reset();
		}

		if (useDiff)
		{
			MemoryOutputStream* diffMos = new MemoryOutputStream();

			ScopedPointer<HiseLosslessAudioFormatWriter> diffWriter = dynamic_cast<HiseLosslessAudioFormatWriter*>(hlac.createWriterFor(diffMos, 44100, b.getNumChannels(), 16, emptyMetadata, 5));

			if (diffWriter == nullptr)
				return 1;

			HlacEncoder::CompressorOptions diff;

			diff.fixedBlockWidth = 1024;
			diff.removeDcOffset = false;
			diff.useDeltaEncoding = false;
			diff.bitRateForWholeBlock = 4;
			diff.useDiffEncodingWithFixedBlocks = true;

			diffWriter->setOptions(diff);
			diffWriter->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());

			r = diffWriter->getCompressionRatioForLastFile();

			diffRatio += r;
			encoder.reset();

			AudioSampleBuffer b2(b.getNumChannels(), CompressionHelpers::getPaddedSampleSize(b.getNumSamples()));
			MemoryInputStream mis(mos.getMemoryBlock(), true);

			MemoryInputStream* diffMis = new MemoryInputStream(diffMos->getMemoryBlock(), true);
			ScopedPointer<HiseLosslessAudioFormatReader> diffReader = dynamic_cast<HiseLosslessAudioFormatReader*>(hlac.createReaderFor(diffMis, false));

			diffReader->read(&b2, 0, b2.getNumSamples(), 0, true, true);

			diffSpeed += diffReader->getDecompressionPerformanceForLastFile();

			CompressionHelpers::checkBuffersEqual(b2, b);

			Logger::writeToLog("Compressing with diff: " + String(r, 3));

			mos.reset();
		}
	}

	flacRatio /= (float)numFilesChecked;
	deltaRatio /= (float)numFilesChecked;
	diffRatio /= (float)numFilesChecked;
	blockRatio /= (float)numFilesChecked;

	blockSpeed /= (double)numFilesChecked;
	pcmSpeed /= (double)numFilesChecked;
	flacSpeed /= (double)numFilesChecked;
	deltaSpeed /= (double)numFilesChecked;
	diffSpeed /= (double)numFilesChecked;

	Logger::writeToLog("=====================================================");
	if (checkWithFlac) Logger::writeToLog("FLAC ratio:\t" + String(flacRatio, 3));
	if (useBlock) Logger::writeToLog("Block ratio:\t" + String(blockRatio, 3));
	if (useDelta) Logger::writeToLog("Delta ratio:\t" + String(deltaRatio, 3));
	if (useDiff) Logger::writeToLog("Diff ratio:\t" + String(diffRatio, 3));
	Logger::writeToLog("=====================================================");
	Logger::writeToLog("PCM speed:\t" + String(pcmSpeed, 1));
	if (checkWithFlac) Logger::writeToLog("FLAC speed:\t" + String(flacSpeed, 1));
	if (useBlock) Logger::writeToLog("Block speed:\t" + String(blockSpeed, 1));
	if (useDelta) Logger::writeToLog("Delta speed:\t" + String(deltaSpeed, 1));
	if (useDiff) Logger::writeToLog("Diff speed:\t" + String(diffSpeed, 1));

	Logger::setCurrentLogger(nullptr);

	return 0;
}