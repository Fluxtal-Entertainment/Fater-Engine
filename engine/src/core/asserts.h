#pragma once
#include "defines.h"

//Disable assertions by commenting out the below line.
#define FASSERTIONS_ENABLED

#ifdef FASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

FAPI void report_assertion_faliure(const char* expression, const char* message, const char* file, i32 line);

#define FASSERT(expr)   {  if (expr) {} else { report_assertion_faliure(#expr, "", __FILE__, __LINE__); debugBreak(); }}       

#define FASSERT_MSG(expr, message)  {   if (expr) {} else { report_assertion_faliure(#expr, message, __FILE__, __LINE__); debugBreak(); }}  

#ifdef _DEBUG
#define FASSERT_DEBUG(expr){    if (expr) {} else { report_assertion_faliure(#expr, "", __FILE__, __LINE__); debugBreak();}}
  
#else
#define FASSERT_DEBUG(expr) //Does nothing at all
#endif

#else
#define FASSERT(expr)   //Does nothing at all
#define FASSERT_MSG(expr, message)  //Does nothing at all
#define FASSERT_DEBUG(espr) //Does nothing at all
#endif