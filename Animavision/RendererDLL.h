#pragma once

#ifdef ANIMAVISION_EXPORTS
#define ANIMAVISION_DLL __declspec(dllexport)
#else
#define ANIMAVISION_DLL __declspec(dllimport)
#endif

#pragma warning(disable : 4251) 
