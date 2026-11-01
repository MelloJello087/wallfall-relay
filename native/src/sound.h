#pragma once
#include <string>
#include <vector>

namespace wf {

/* ---------- Sound: the browser's oscillator and tone(), rendered to samples ---------- */
enum class Wave { Sine, Square, Sawtooth, Triangle };
void addTone(std::vector<float> &out, int rate, double f, double dur, Wave type = Wave::Square, double vol = .05, double to = 0, double delay = 0);
std::vector<float> renderSfx(const std::string &name, int rate, double seconds);
const std::vector<std::string> &sfxNames();

}
