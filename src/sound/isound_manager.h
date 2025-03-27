#ifndef ISOUND_MANAGER_H
#define ISOUND_MANAGER_H

#include <string>

class ISoundManager {
public:
    virtual ~ISoundManager() = default;
    virtual void loadSounds() = 0;
    virtual void playSound(const std::string& key) = 0;
};

#endif // ISOUND_MANAGER_H