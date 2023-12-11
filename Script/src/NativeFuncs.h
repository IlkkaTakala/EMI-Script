#pragma once
#include "Defines.h"
#include <string>

inline std::string IntToStr(int i) { return std::to_string(i); }
inline std::string FloatToStr(float i) { return std::to_string(i); }
inline std::string BoolToStr(bool i) { return i ? "true" : "false"; }

inline int StrToInt(const char* i) { return std::atoi(i); }
inline int FloatToInt(float i) { return int(i); }
inline int BoolToInt(bool i) { return i; }

inline float StrToFloat(const char* i) { return (float)std::atof(i); }
inline float IntToFloat(int i) { return (float)i; }
inline float BoolToFloat(bool i) { return (float)i; }

inline bool StrToBool(const char* i) { return false; }
inline bool FloatToBool(float i) { return bool(i); }
inline bool IntToBool(int i) { return i; }

// Ops

class VM;

void IntAdd(const uint8*& ptr, const uint8* end);