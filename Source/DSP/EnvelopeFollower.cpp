#include "EnvelopeFollower.h"

#include <cmath>

void EnvelopeFollower::prepare(double sampleRate)
{
    sr = sampleRate;
    envelope = 0.0f;
    setAttackMs(5.0f);
    setReleaseMs(15.0f);
}

void EnvelopeFollower::setAttackMs(float ms)
{
    attackCoeff = std::exp(-1.0f / (float(sr) * ms * 0.001f));
}

void EnvelopeFollower::setReleaseMs(float ms)
{
    releaseCoeff = std::exp(-1.0f / (float(sr) * ms * 0.001f));
}

float EnvelopeFollower::process(float input)
{
    float absInput = std::fabs(input);
    if (absInput > envelope)
        envelope = attackCoeff * envelope + (1.0f - attackCoeff) * absInput;
    else
        envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * absInput;
    return envelope;
}

void EnvelopeFollower::reset()
{
    envelope = 0.0f;
}
