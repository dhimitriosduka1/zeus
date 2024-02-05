#include <lightwave.hpp>

#include "pcg32.h"
#include "prime_generator.hpp"

namespace lightwave {
    class Halton : public Sampler {
        uint64_t m_seed;
        pcg32 m_pcg;
        PrimeGenerator m_primeGenerator;

        int m_base;
        int m_index;
        float m_offset;

    public:
        Halton(const Properties &properties): Sampler(properties) {
            m_seed = properties.get<int>("seed", 1337);
        }

        void seed(int sampleIndex) override {}
    
        void seed(const Point2i &pixel, int sampleIndex) override {    
            m_primeGenerator.resetIdx();
            m_base = m_primeGenerator.getNextPrime();

            const uint64_t a = (uint64_t(pixel.x()) << 32) ^ pixel.y();
            m_pcg.seed(m_seed, a);
            m_offset = m_pcg.nextFloat();

            m_index = sampleIndex;
        }

        float next() override {
            float result = halton(m_base, m_index) + m_offset;
            if (result >= 1.0f) result -= 1.0f;
            m_base = m_primeGenerator.getNextPrime();
            return result; 
        }

        float halton(int base, int index) {
            float invBase = (float)1 / (float)base, invBaseM = 1;
            uint64_t reversedDigits = 0;

            while (index) {
                uint64_t next = index / base;
                uint64_t digit = index - next * base;
                reversedDigits = reversedDigits * base + digit;
                invBaseM *= invBase;
                index = next;
            }
            return min(reversedDigits * invBaseM, 1 - Epsilon);
        }

        ref<Sampler> clone() const override {
            return std::make_shared<Halton>(*this);
        }

        std::string toString() const override {
            return tfm::format(
                "Halton[\n"
                "  count = %d\n"
                "]",
                m_samplesPerPixel
            );
        }
    };
}

REGISTER_SAMPLER(Halton, "halton")
