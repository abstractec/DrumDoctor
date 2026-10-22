#pragma once

#include "analysis/PlayabilityAnalyzer.h"

#include <array>
#include <atomic>
#include <deque>
#include <juce_audio_processors/juce_audio_processors.h>

class DrumDoctorProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::AsyncUpdater
{
public:
    DrumDoctorProcessor();
    ~DrumDoctorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Drum Doctor"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool loadMidiFile (const juce::File& file);
    void startCapture();
    bool stopCapture();
    bool isCapturing() const noexcept { return capturing.load(); }
    double getPositionStampMs() const noexcept { return positionStampMs.load(); }

    void setStandalonePlaying (bool shouldPlay);
    void rewindStandalone();
    bool isStandalonePlaying() const noexcept { return standalonePlaying.load(); }
    bool isTransportPlaying() const;

    double getFilePlayheadPpq() const;
    bool hasClip() const;

    void drainIncoming();
    std::shared_ptr<const drumdoctor::AnalysisResult> getAnalysis() const;
    uint32_t getAnalysisGeneration() const;
    drumdoctor::DifficultyPoint meterPoint();

    juce::AudioProcessorValueTreeState apvts;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void reanalyze();
    bool ingestMidi (const juce::MemoryBlock& bytes);
    void pushIncoming (const drumdoctor::DrumHit& hit);
    double clusterWindowSeconds() const;
    drumdoctor::KitMap currentKitMap() const;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double currentSampleRate = 44100.0;

    static constexpr int queueSize = 4096;
    juce::AbstractFifo incomingFifo { queueSize };
    std::array<drumdoctor::DrumHit, queueSize> incomingStorage {};

    juce::CriticalSection clipLock;
    std::vector<drumdoctor::DrumHit> clipHits;
    drumdoctor::ScoreMap clipScore;
    juce::MemoryBlock clipBytes;
    bool clipLoaded = false;
    std::shared_ptr<const drumdoctor::AnalysisResult> analysis = std::make_shared<const drumdoctor::AnalysisResult>();
    uint32_t generation = 1;

    std::deque<drumdoctor::DrumHit> liveHits;
    std::vector<drumdoctor::DrumHit> captureHits;

    std::atomic<bool> capturing { false };
    std::atomic<bool> standalonePlaying { false };
    std::atomic<bool> hostValid { false };
    std::atomic<bool> hostIsPlaying { false };
    std::atomic<double> hostPpq { 0.0 };
    std::atomic<double> hostSeconds { 0.0 };
    std::atomic<double> hostBpm { 120.0 };
    std::atomic<double> positionStampMs { 0.0 };
    std::atomic<double> standalonePpq { 0.0 };
    std::atomic<double> standaloneSeconds { 0.0 };
    std::atomic<double> playbackBpm { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumDoctorProcessor)
};
