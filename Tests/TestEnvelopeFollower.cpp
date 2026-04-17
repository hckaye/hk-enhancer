#include "DSP/EnvelopeFollower.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("EnvelopeFollower initializes to zero", "[envelope]")
{
    EnvelopeFollower env;
    env.prepare(44100.0);

    REQUIRE_THAT(env.process(0.0f), WithinAbs(0.0, 1e-6));
}

TEST_CASE("EnvelopeFollower tracks amplitude", "[envelope]")
{
    EnvelopeFollower env;
    env.prepare(44100.0);
    env.setAttackMs(0.1f); // very fast attack
    env.setReleaseMs(0.1f);

    // Feed a constant signal, envelope should rise toward it
    float result = 0.0f;
    for (int i = 0; i < 1000; ++i)
        result = env.process(1.0f);

    REQUIRE(result > 0.9f);
}

TEST_CASE("EnvelopeFollower decays on silence", "[envelope]")
{
    EnvelopeFollower env;
    env.prepare(44100.0);
    env.setAttackMs(0.1f);
    env.setReleaseMs(1.0f);

    // Build up envelope
    for (int i = 0; i < 1000; ++i)
        env.process(1.0f);

    // Feed silence — envelope should decay
    float peakAfterSilence = 0.0f;
    for (int i = 0; i < 500; ++i)
        peakAfterSilence = env.process(0.0f);

    REQUIRE(peakAfterSilence < 0.5f);
}

TEST_CASE("EnvelopeFollower reset clears state", "[envelope]")
{
    EnvelopeFollower env;
    env.prepare(44100.0);

    for (int i = 0; i < 100; ++i)
        env.process(1.0f);

    env.reset();
    REQUIRE_THAT(env.process(0.0f), WithinAbs(0.0, 1e-6));
}

TEST_CASE("EnvelopeFollower handles negative input", "[envelope]")
{
    EnvelopeFollower env;
    env.prepare(44100.0);
    env.setAttackMs(0.1f);

    float result = 0.0f;
    for (int i = 0; i < 1000; ++i)
        result = env.process(-0.8f);

    // Should track absolute value
    REQUIRE(result > 0.7f);
}
