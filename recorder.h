#ifndef RECORDER_H
#define RECORDER_G

#include <string>
#include <cstdint>
#include <vector>

void StartRecording(std::string filename, uint16_t res_x, uint16_t res_y, uint8_t fps);
void NextFrame(uint8_t* rgba);
void StopRecording();

#endif //RECORDER_H