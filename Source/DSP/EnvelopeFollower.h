#pragma once

class EnvelopeFollower
{
public:
    void prepare(double sampleRate);
    void setAttackMs(float ms);
    void setReleaseMs(float ms);
    float process(float input);
    void reset();

private:
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
    double sr = 44100.0;
};
