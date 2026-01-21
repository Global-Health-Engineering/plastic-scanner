#ifndef TLC59208_H
#define TLC59208_H

class TLC59208 {
public:
    bool begin(); //was void in beginning
    void on(int output);
    void off(int output);
};


#endif  // TLC59208_H
