#ifndef _ERIL_INC_GUARD_
#define _ERIL_INC_GUARD_
#pragma once

#ifdef SCRIPTEXPORT
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport) 
#endif

#define _C extern "C"


#endif // !_ERIL_INC_GUARD_