#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

DrumDoctorProcessor::DrumDoctorProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    apvts.addParameterListener ("clusterMs", this);
    apvts.addParameterListener ("kitMap", this);
}

DrumDoctorProcessor::~DrumDoctorProcessor()
{
    cancelPendingUpdate();
    apvts.removeParameterListener ("clusterMs", this);
    apvts.removeParameterListener ("kitMap", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout DrumDoctorProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "clusterMs", 1 },
        "Cluster window",
        juce::NormalisableRange<float> (1.0f, 40.0f, 0.1f),
        10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ppqOffset", 1 },
        "Playhead offset",
        juce::NormalisableRange<float> (-64.0f, 64.0f, 0.25f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("q")));

    juce::StringArray maps;
    for (int index = 0; index < static_cast<int> (drumdoctor::KitMap::count); ++index)
        maps.add (drumdoctor::kitMapName (static_cast<drumdoctor::KitMap> (index)));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "kitMap", 1 },
        "MIDI map",
        maps,
        0));
    return { params.begin(), params.end() };
}

void DrumDoctorProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
}

void DrumDoctorProcessor::releaseResources() {}

bool DrumDoctorProcessor::isBusesLayoutSupported (const BusesLayout&) const
{
    return true;
}

void DrumDoctorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || currentSampleRate <= 0.0)
        return;

    const bool standalone = wrapperType == juce::AudioProcessor::wrapperType_Standalone;
    double blockPpq = standalonePpq.load();
    double blockSeconds = standaloneSeconds.load();
    double bpm = playbackBpm.load();

    if (! standalone)
    {
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition())
            {
                if (const auto ppq = position->getPpqPosition())
                {
                    blockPpq = *ppq;
                    hostPpq.store (blockPpq);
                    hostValid.store (true);
                    hostIsPlaying.store (position->getIsPlaying());
                    positionStampMs.store (juce::Time::getMillisecondCounterHiRes());

                    if (const auto positionBpm = position->getBpm())
                    {
                        if (*positionBpm > 1.0)
                        {
                            bpm = *positionBpm;
                            hostBpm.store (bpm);
                        }
                    }

                    if (const auto seconds = position->getTimeInSeconds())
                    {
                        blockSeconds = *seconds;
                        hostSeconds.store (blockSeconds);
                    }
                }
            }
        }
    }

    const double blockMs = juce::Time::getMillisecondCounterHiRes();
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (! message.isNoteOn())
            continue;

        const double delta = metadata.samplePosition / currentSampleRate;
        drumdoctor::DrumHit hit;
        hit.note = message.getNoteNumber();
        hit.ppq = blockPpq + delta * bpm / 60.0;
        hit.timeSeconds = blockSeconds + delta;
        hit.receivedMs = blockMs + delta * 1000.0;
        pushIncoming (hit);
    }

    if (standalone && (standalonePlaying.load() || capturing.load()))
    {
        const double delta = static_cast<double> (numSamples) / currentSampleRate;
        standalonePpq.store (blockPpq + delta * bpm / 60.0);
        standaloneSeconds.store (blockSeconds + delta);
        positionStampMs.store (juce::Time::getMillisecondCounterHiRes());
    }
}

double DrumDoctorProcessor::clusterWindowSeconds() const
{
    if (auto* value = apvts.getRawParameterValue ("clusterMs"))
        return static_cast<double> (value->load()) * 0.001;
    return 0.010;
}

drumdoctor::KitMap DrumDoctorProcessor::currentKitMap() const
{
    if (auto* value = apvts.getRawParameterValue ("kitMap"))
    {
        const int index = juce::roundToInt (value->load());
        if (index >= 0 && index < static_cast<int> (drumdoctor::KitMap::count))
            return static_cast<drumdoctor::KitMap> (index);
    }

    return drumdoctor::KitMap::generalMidi;
}

void DrumDoctorProcessor::pushIncoming (const drumdoctor::DrumHit& hit)
{
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    incomingFifo.prepareToWrite (1, start1, size1, start2, size2);
    if (size1 > 0)
        incomingStorage[static_cast<size_t> (start1)] = hit;
    incomingFifo.finishedWrite (size1 > 0 ? 1 : 0);
}

void DrumDoctorProcessor::drainIncoming()
{
    const int ready = incomingFifo.getNumReady();
    if (ready <= 0)
        return;

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    incomingFifo.prepareToRead (ready, start1, size1, start2, size2);

    std::vector<drumdoctor::DrumHit> batch;
    batch.reserve (static_cast<size_t> (size1 + size2));
    for (int i = 0; i < size1; ++i)
        batch.push_back (incomingStorage[static_cast<size_t> (start1 + i)]);
    for (int i = 0; i < size2; ++i)
        batch.push_back (incomingStorage[static_cast<size_t> (start2 + i)]);
    incomingFifo.finishedRead (size1 + size2);

    const bool record = capturing.load();
    for (const auto& hit : batch)
    {
        liveHits.push_back (hit);
        if (record && captureHits.size() < 200000)
            captureHits.push_back (hit);
    }

    const double cutoff = liveHits.empty() ? 0.0 : liveHits.back().timeSeconds - 2.0;
    while (! liveHits.empty() && liveHits.front().timeSeconds < cutoff)
        liveHits.pop_front();

    while (liveHits.size() > 512)
        liveHits.pop_front();
}

void DrumDoctorProcessor::reanalyze()
{
    std::vector<drumdoctor::DrumHit> hits;
    drumdoctor::ScoreMap score;
    bool loaded = false;
    {
        const juce::ScopedLock lock (clipLock);
        hits = clipHits;
        score = clipScore;
        loaded = clipLoaded;
    }

    drumdoctor::AnalysisSettings settings;
    settings.clusterWindowSeconds = clusterWindowSeconds();
    settings.kitMap = currentKitMap();
    auto result = drumdoctor::analyzeHits (hits, score, settings);
    result.loaded = loaded;

    const juce::ScopedLock lock (clipLock);
    analysis = std::make_shared<const drumdoctor::AnalysisResult> (std::move (result));
    ++generation;
}

bool DrumDoctorProcessor::ingestMidi (const juce::MemoryBlock& bytes)
{
    auto loaded = drumdoctor::loadDrumClip (bytes);
    if (! loaded)
        return false;

    {
        const juce::ScopedLock lock (clipLock);
        clipHits = std::move (loaded->hits);
        clipScore = std::move (loaded->score);
        clipBytes = bytes;
        clipLoaded = true;
        if (! clipScore.tempos.empty())
            playbackBpm.store (clipScore.tempos.front().bpm);
    }

    reanalyze();
    rewindStandalone();
    return true;
}

bool DrumDoctorProcessor::loadMidiFile (const juce::File& file)
{
    juce::MemoryBlock bytes;
    if (! file.loadFileAsData (bytes))
        return false;

    capturing.store (false);
    captureHits.clear();
    return ingestMidi (bytes);
}

void DrumDoctorProcessor::startCapture()
{
    capturing.store (false);
    drainIncoming();
    captureHits.clear();
    liveHits.clear();

    {
        const juce::ScopedLock lock (clipLock);
        clipHits.clear();
        clipBytes.reset();
        clipLoaded = false;
        clipScore = {};
        analysis = std::make_shared<const drumdoctor::AnalysisResult>();
        ++generation;
    }

    standalonePlaying.store (false);
    standalonePpq.store (0.0);
    standaloneSeconds.store (0.0);
    capturing.store (true);
}

bool DrumDoctorProcessor::stopCapture()
{
    capturing.store (false);
    drainIncoming();

    if (captureHits.empty())
        return false;

    const bool useHost = wrapperType != juce::AudioProcessor::wrapperType_Standalone && hostValid.load();
    const double bpm = std::max (1.0, useHost ? hostBpm.load() : playbackBpm.load());
    const auto bytes = drumdoctor::midiFromHits (captureHits, bpm);
    captureHits.clear();
    ingestMidi (bytes);
    return true;
}

void DrumDoctorProcessor::setStandalonePlaying (bool shouldPlay)
{
    standalonePlaying.store (shouldPlay);
}

void DrumDoctorProcessor::rewindStandalone()
{
    double start = 0.0;
    {
        const juce::ScopedLock lock (clipLock);
        if (analysis != nullptr)
            start = analysis->startPpq;
    }

    const double bpm = std::max (1.0, playbackBpm.load());
    standalonePpq.store (start);
    standaloneSeconds.store (start * 60.0 / bpm);
}

bool DrumDoctorProcessor::isTransportPlaying() const
{
    if (wrapperType != juce::AudioProcessor::wrapperType_Standalone && hostValid.load())
        return hostIsPlaying.load();
    return standalonePlaying.load();
}

double DrumDoctorProcessor::getFilePlayheadPpq() const
{
    double offset = 0.0;
    if (auto* value = apvts.getRawParameterValue ("ppqOffset"))
        offset = static_cast<double> (value->load());

    double ppq = 0.0;
    double bpm = playbackBpm.load();
    bool moving = false;

    if (wrapperType != juce::AudioProcessor::wrapperType_Standalone && hostValid.load())
    {
        ppq = hostPpq.load();
        bpm = hostBpm.load();
        moving = hostIsPlaying.load();
    }
    else if (wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        ppq = standalonePpq.load();
        moving = standalonePlaying.load();
    }

    if (moving)
    {
        const double elapsedMs = juce::Time::getMillisecondCounterHiRes() - positionStampMs.load();
        if (elapsedMs > 0.0 && elapsedMs < 90.0)
            ppq += elapsedMs * 0.001 * std::max (1.0, bpm) / 60.0;
    }

    return ppq - offset;
}

bool DrumDoctorProcessor::hasClip() const
{
    const juce::ScopedLock lock (clipLock);
    return clipLoaded;
}

std::shared_ptr<const drumdoctor::AnalysisResult> DrumDoctorProcessor::getAnalysis() const
{
    const juce::ScopedLock lock (clipLock);
    return analysis;
}

uint32_t DrumDoctorProcessor::getAnalysisGeneration() const
{
    const juce::ScopedLock lock (clipLock);
    return generation;
}

drumdoctor::DifficultyPoint DrumDoctorProcessor::hostedLiveMeter()
{
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    while (! liveHits.empty() && (liveHits.front().receivedMs <= 0.0 || nowMs - liveHits.front().receivedMs > 2000.0))
        liveHits.pop_front();

    if (liveHits.empty())
        return {};

    double bpm = playbackBpm.load();
    if (hostValid.load())
        bpm = hostBpm.load();
    bpm = std::max (1.0, bpm);

    constexpr double anchor = 16.0;
    std::vector<drumdoctor::DrumHit> hits;
    hits.reserve (liveHits.size());
    for (const auto& hit : liveHits)
    {
        auto timed = hit;
        const double ageSeconds = std::max (0.0, (nowMs - hit.receivedMs) * 0.001);
        timed.timeSeconds = anchor * 60.0 / bpm - ageSeconds;
        timed.ppq = anchor - ageSeconds * bpm / 60.0;
        hits.push_back (timed);
    }

    drumdoctor::ScoreMap score;
    score.tempos = { { 0.0, bpm } };
    score.meters = { { 0.0, 4, 4 } };
    drumdoctor::AnalysisSettings settings;
    settings.clusterWindowSeconds = clusterWindowSeconds();
    settings.kitMap = currentKitMap();
    settings.curveEndPpq = anchor;
    const auto live = drumdoctor::analyzeHits (hits, score, settings);
    return drumdoctor::sampleCurve (live.curve, anchor);
}

drumdoctor::DifficultyPoint DrumDoctorProcessor::meterPoint()
{
    if (wrapperType != juce::AudioProcessor::wrapperType_Standalone)
        return hostedLiveMeter();

    bool loaded = false;
    std::shared_ptr<const drumdoctor::AnalysisResult> snapshot;
    {
        const juce::ScopedLock lock (clipLock);
        loaded = clipLoaded;
        snapshot = analysis;
    }

    if (loaded && snapshot != nullptr)
        return drumdoctor::sampleCurve (snapshot->curve, getFilePlayheadPpq());

    if (liveHits.empty())
        return {};

    double bpm = playbackBpm.load();
    if (wrapperType != juce::AudioProcessor::wrapperType_Standalone && hostValid.load())
        bpm = hostBpm.load();

    drumdoctor::ScoreMap score;
    score.tempos = { { 0.0, std::max (1.0, bpm) } };
    score.meters = { { 0.0, 4, 4 } };
    drumdoctor::AnalysisSettings settings;
    settings.clusterWindowSeconds = clusterWindowSeconds();
    settings.kitMap = currentKitMap();
    const auto live = drumdoctor::analyzeHits (std::vector<drumdoctor::DrumHit> (liveHits.begin(), liveHits.end()),
                                               score,
                                               settings);
    return drumdoctor::sampleCurve (live.curve, liveHits.back().ppq);
}

void DrumDoctorProcessor::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID == "clusterMs" || parameterID == "kitMap")
        triggerAsyncUpdate();
}

void DrumDoctorProcessor::handleAsyncUpdate()
{
    reanalyze();
}

void DrumDoctorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("DrumDoctor");
    state.addChild (apvts.copyState(), -1, nullptr);

    juce::MemoryBlock bytes;
    {
        const juce::ScopedLock lock (clipLock);
        bytes = clipBytes;
    }

    if (bytes.getSize() > 0)
        state.setProperty ("midi", bytes.toBase64Encoding(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DrumDoctorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    const auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || ! state.hasType ("DrumDoctor"))
        return;

    if (const auto params = state.getChildWithName ("PARAMS"); params.isValid())
        apvts.replaceState (params);

    if (state.hasProperty ("midi"))
    {
        juce::MemoryBlock bytes;
        if (bytes.fromBase64Encoding (state.getProperty ("midi").toString()))
            ingestMidi (bytes);
    }
}

juce::AudioProcessorEditor* DrumDoctorProcessor::createEditor()
{
    return new DrumDoctorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumDoctorProcessor();
}
