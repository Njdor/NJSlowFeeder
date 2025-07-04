// Speaker.h
#ifndef SPEAKER_H
#define SPEAKER_H

class Speaker {
public:
    virtual void makeSound(int frequency, int duration) = 0; // Pure virtual function
    virtual ~Speaker() {}         
};

#endif