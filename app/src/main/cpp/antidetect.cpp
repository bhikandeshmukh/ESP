#include "antidetect.h"
#include "features.h"
#include <sstream>
#include <iomanip>

extern ESPConfig g_config;
AntiDetect g_antiDetect;

AntiDetect::AntiDetect() : m_minDelay(1), m_maxDelay(5), m_enabled(true),
    m_readCount(0), m_burstLimit(50) {
    
    // Seed RNG with high-resolution time
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    m_rng.seed(static_cast<unsigned int>(seed));
    
    m_lastRead = std::chrono::steady_clock::now();
}

void AntiDetect::setDelayRange(int minMs, int maxMs) {
    m_minDelay = minMs;
    m_maxDelay = maxMs;
}

void AntiDetect::preRead() {
    if (!m_enabled) return;
    
    m_readCount++;
    
    // Burst protection - add delay after many consecutive reads
    if (m_readCount >= m_burstLimit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(randomInt(5, 15)));
        m_readCount = 0;
    }
    
    // Random micro-delay
    if (g_config.randomizeReads.load()) {
        int delay = randomInt(m_minDelay, m_maxDelay);
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(delay * 100));
        }
    }
}

void AntiDetect::postRead() {
    if (!m_enabled) return;
    
    m_lastRead = std::chrono::steady_clock::now();
}

void AntiDetect::randomDelay() {
    int delay = randomInt(m_minDelay, m_maxDelay);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
}

int AntiDetect::randomInt(int min, int max) {
    if (min >= max) return min;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(m_rng);
}

float AntiDetect::randomFloat(float min, float max) {
    if (min >= max) return min;
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_rng);
}

bool AntiDetect::shouldSkipFrame(int skipChance) {
    return randomInt(1, 100) <= skipChance;
}

std::string AntiDetect::obfuscateAddress(unsigned long addr) {
    // XOR with random key for logging
    unsigned long key = 0xDEADBEEF;
    unsigned long obfuscated = addr ^ key;
    
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << obfuscated;
    return ss.str();
}
