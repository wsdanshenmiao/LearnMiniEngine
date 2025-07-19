#pragma once
#ifndef __RANDOM_H__
#define __RANDOM_H__

#include "MathCommon.h"
#include "../Utilities/Singleton.h"
#include <random>

namespace DSM {
    
    class RandomNumberGenerater : public Singleton<RandomNumberGenerater> 
    {
    public:
        RandomNumberGenerater(std::uint32_t seed = 0)
            :m_Generator(m_RandomDevice()){
            if (seed != 0) {
                m_Generator.seed(seed);
            }
        }

        int NextInt(int min = std::numeric_limits<int>::lowest(), int max = (std::numeric_limits<int>::max)())
        {
            return std::uniform_int_distribution{min, max}(m_Generator);
        }

        float NextFloat(float min = 0.0f, float max = 1.0f)
        {
            return std::uniform_real_distribution{min, max}(m_Generator);
        }

        void SetSeed(std::uint32_t seed)
        {
            m_Generator.seed(seed);
        }

    private:
        std::random_device m_RandomDevice{};
        std::mt19937_64 m_Generator{};
    };

    #define g_RandomGenerator (RandomNumberGenerater::GetInstance())
}

#endif
