
#pragma once 

#if defined(E2_BUILD)

#define E2_API __declspec(dllexport)

#else 

#define E2_API __declspec(dllimport)

#endif