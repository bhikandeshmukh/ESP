#ifndef ANTIDETECT_H
#define ANTIDETECT_H

#include <random>
#include <chrono>
#include <thread>
#include <string>
#include <vector>

/**
 * Anti-Detection Module
 * - Random delays between memory reads
 * - Memory access pattern randomization
 * - Process name spoofing helpers
 */
class AntiDetect {
private:
    std::mt19937 m_rng;
    int m_minDelay;
    int m_maxDelay;
    bool m_enabled;
    
    // Read counter for pattern analysis prevention
    int m_readCount;
    int m_burstLimit;
    
    // Timing
    std::chrono::steady_clock::time_point m_lastRead;
    
public:
    AntiDetect();
    
    void setEnabled(bool enabled) { m_enabled = enabled; }
    void setDelayRange(int minMs, int maxMs);
    void setBurstLimit(int limit) { m_burstLimit = limit; }
    
    // Call before each memory read
    void preRead();
    
    // Call after memory read
    void postRead();
    
    // Random delay
    void randomDelay();
    
    // Randomize read order
    template<typename T>
    void shuffleVector(std::vector<T>& vec) {
        if (vec.size() <= 1) return;
        for (size_t i = vec.size() - 1; i > 0; i--) {
            std::uniform_int_distribution<size_t> dist(0, i);
            size_t j = dist(m_rng);
            std::swap(vec[i], vec[j]);
        }
    }
    
    // Get random int in range
    int randomInt(int min, int max);
    
    // Get random float in range
    float randomFloat(float min, float max);
    
    // Check if should skip this frame (random frame skip)
    bool shouldSkipFrame(int skipChance = 5);  // 5% chance
    
    // Obfuscate memory address (for logging)
    std::string obfuscateAddress(unsigned long addr);
};

// Global instance
extern AntiDetect g_antiDetect;

// Macro for easy use
#define ANTI_DETECT_PRE() if(g_config.enableAntiDetect) g_antiDetect.preRead()
#define ANTI_DETECT_POST() if(g_config.enableAntiDetect) g_antiDetect.postRead()

#endif // ANTIDETECT_H
