#pragma once
#include "Defines.h"
#include <string>

inline std::string IntToStr(int i) { return std::to_string(i); }
inline std::string FloatToStr(double i) { return std::to_string(i); }
inline std::string BoolToStr(bool i) { return i ? "true" : "false"; }

inline int StrToInt(const char* i) { return std::atoi(i); }
inline int FloatToInt(double i) { return int(i); }
inline int BoolToInt(bool i) { return i; }

inline double StrToFloat(const char* i) { return (double)std::atof(i); }
inline double IntToFloat(int i) { return (double)i; }
inline double BoolToFloat(bool i) { return (double)i; }

inline bool StrToBool(const char*) { return false; }
inline bool FloatToBool(double i) { return bool(i); }
inline bool IntToBool(int i) { return i; }

// Ops

class VM;

void IntAdd(const uint8*& ptr, const uint8* end);