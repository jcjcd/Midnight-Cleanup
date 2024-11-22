//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#pragma once


#define INLINE __forceinline

namespace Math
{
	template <typename T> __forceinline T AlignUpWithMask(T value, size_t mask)
	{
		return (T)(((size_t)value + mask) & ~mask);
	}

	template <typename T> __forceinline T AlignDownWithMask(T value, size_t mask)
	{
		return (T)((size_t)value & ~mask);
	}

	template <typename T> __forceinline T AlignUp(T value, size_t alignment)
	{
		return AlignUpWithMask(value, alignment - 1);
	}

	template <typename T> __forceinline T AlignDown(T value, size_t alignment)
	{
		return AlignDownWithMask(value, alignment - 1);
	}

	template <typename T> __forceinline bool IsAligned(T value, size_t alignment)
	{
		return 0 == ((size_t)value & (alignment - 1));
	}

	template <typename T> __forceinline T DivideByMultiple(T value, size_t alignment)
	{
		return (T)((value + alignment - 1) / alignment);
	}

	template <typename T> __forceinline bool IsPowerOfTwo(T value)
	{
		return 0 == (value & (value - 1));
	}

	template <typename T> __forceinline bool IsDivisible(T value, T divisor)
	{
		return (value / divisor) * divisor == value;
	}

	__forceinline uint8_t Log2(uint64_t value)
	{
		unsigned long mssb; // most significant set bit
		unsigned long lssb; // least significant set bit

		// If perfect power of two (only one set bit), return index of bit.  Otherwise round up
		// fractional log by adding 1 to most signicant set bit's index.
		if (_BitScanReverse64(&mssb, value) > 0 && _BitScanForward64(&lssb, value) > 0)
			return uint8_t(mssb + (mssb == lssb ? 0 : 1));
		else
			return 0;
	}

	template <typename T> __forceinline T AlignPowerOfTwo(T value)
	{
		return value == 0 ? 0 : 1 << Log2(value);
	}

	using namespace DirectX;

	INLINE XMVECTOR SplatZero()
	{
		return XMVectorZero();
	}

#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)

	INLINE XMVECTOR SplatOne(XMVECTOR zero = SplatZero())
	{
		__m128i AllBits = _mm_castps_si128(_mm_cmpeq_ps(zero, zero));
		return _mm_castsi128_ps(_mm_slli_epi32(_mm_srli_epi32(AllBits, 25), 23));	// return 0x3F800000
		//return _mm_cvtepi32_ps(_mm_srli_epi32(SetAllBits(zero), 31));				// return (float)1;  (alternate method)
	}

#if defined(_XM_SSE4_INTRINSICS_)
	INLINE XMVECTOR CreateXUnitVector(XMVECTOR one = SplatOne())
	{
		return _mm_insert_ps(one, one, 0x0E);
	}
	INLINE XMVECTOR CreateYUnitVector(XMVECTOR one = SplatOne())
	{
		return _mm_insert_ps(one, one, 0x0D);
	}
	INLINE XMVECTOR CreateZUnitVector(XMVECTOR one = SplatOne())
	{
		return _mm_insert_ps(one, one, 0x0B);
	}
	INLINE XMVECTOR CreateWUnitVector(XMVECTOR one = SplatOne())
	{
		return _mm_insert_ps(one, one, 0x07);
	}
	INLINE XMVECTOR SetWToZero(FXMVECTOR vec)
	{
		return _mm_insert_ps(vec, vec, 0x08);
	}
	INLINE XMVECTOR SetWToOne(FXMVECTOR vec)
	{
		return _mm_blend_ps(vec, SplatOne(), 0x8);
	}
#else
	INLINE XMVECTOR CreateXUnitVector(XMVECTOR one = SplatOne())
	{
		return _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(one), 12));
	}
	INLINE XMVECTOR CreateYUnitVector(XMVECTOR one = SplatOne())
	{
		XMVECTOR unitx = CreateXUnitVector(one);
		return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(unitx), 4));
	}
	INLINE XMVECTOR CreateZUnitVector(XMVECTOR one = SplatOne())
	{
		XMVECTOR unitx = CreateXUnitVector(one);
		return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(unitx), 8));
	}
	INLINE XMVECTOR CreateWUnitVector(XMVECTOR one = SplatOne())
	{
		return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(one), 12));
	}
	INLINE XMVECTOR SetWToZero(FXMVECTOR vec)
	{
		__m128i MaskOffW = _mm_srli_si128(_mm_castps_si128(_mm_cmpeq_ps(vec, vec)), 4);
		return _mm_and_ps(vec, _mm_castsi128_ps(MaskOffW));
	}
	INLINE XMVECTOR SetWToOne(FXMVECTOR vec)
	{
		return _mm_movelh_ps(vec, _mm_unpackhi_ps(vec, SplatOne()));
	}
#endif

#else // !_XM_SSE_INTRINSICS_

	INLINE XMVECTOR SplatOne() { return XMVectorSplatOne(); }
	INLINE XMVECTOR CreateXUnitVector() { return g_XMIdentityR0; }
	INLINE XMVECTOR CreateYUnitVector() { return g_XMIdentityR1; }
	INLINE XMVECTOR CreateZUnitVector() { return g_XMIdentityR2; }
	INLINE XMVECTOR CreateWUnitVector() { return g_XMIdentityR3; }
	INLINE XMVECTOR SetWToZero(FXMVECTOR vec) { return XMVectorAndInt(vec, g_XMMask3); }
	INLINE XMVECTOR SetWToOne(FXMVECTOR vec) { return XMVectorSelect(g_XMIdentityR3, vec, g_XMMask3); }

#endif

	enum EZeroTag { kZero, kOrigin };
	enum EIdentityTag { kOne, kIdentity };
	enum EXUnitVector { kXUnitVector };
	enum EYUnitVector { kYUnitVector };
	enum EZUnitVector { kZUnitVector };
	enum EWUnitVector { kWUnitVector };

}


namespace Utility
{
#ifdef _CONSOLE
    inline void Print( const char* msg ) { printf("%s", msg); }
    inline void Print( const wchar_t* msg ) { wprintf(L"%ws", msg); }
#else
    inline void Print( const char* msg ) { OutputDebugStringA(msg); }
    inline void Print( const wchar_t* msg ) { OutputDebugString(msg); }
#endif

    inline void Printf( const char* format, ... )
    {
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        va_end(ap);
        Print(buffer);
    }

    inline void Printf( const wchar_t* format, ... )
    {
        wchar_t buffer[256];
        va_list ap;
        va_start(ap, format);
        vswprintf(buffer, 256, format, ap);
        va_end(ap);
        Print(buffer);
    }

#ifndef RELEASE
    inline void PrintSubMessage( const char* format, ... )
    {
        Print("--> ");
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        va_end(ap);
        Print(buffer);
        Print("\n");
    }
    inline void PrintSubMessage( const wchar_t* format, ... )
    {
        Print("--> ");
        wchar_t buffer[256];
        va_list ap;
        va_start(ap, format);
        vswprintf(buffer, 256, format, ap);
        va_end(ap);
        Print(buffer);
        Print("\n");
    }
    inline void PrintSubMessage( void )
    {
    }
#endif

    std::wstring UTF8ToWideString( const std::string& str );
    std::string WideStringToUTF8( const std::wstring& wstr );
    std::string ToLower(const std::string& str);
    std::wstring ToLower(const std::wstring& str);
    std::string GetBasePath(const std::string& str);
    std::wstring GetBasePath(const std::wstring& str);
    std::string RemoveBasePath(const std::string& str);
    std::wstring RemoveBasePath(const std::wstring& str);
    std::string GetFileExtension(const std::string& str);
    std::wstring GetFileExtension(const std::wstring& str);
    std::string RemoveExtension(const std::string& str);
    std::wstring RemoveExtension(const std::wstring& str);


} // namespace Utility

#ifdef ERROR
#undef ERROR
#endif
#ifdef ASSERT
#undef ASSERT
#endif
#ifdef HALT
#undef HALT
#endif

#define HALT( ... ) ERROR( __VA_ARGS__ ) __debugbreak();

#ifdef RELEASE

    #define ASSERT( isTrue, ... ) (void)(isTrue)
    #define WARN_ONCE_IF( isTrue, ... ) (void)(isTrue)
    #define WARN_ONCE_IF_NOT( isTrue, ... ) (void)(isTrue)
    #define ERROR( msg, ... )
    #define DEBUGPRINT( msg, ... ) do {} while(0)
    #define ASSERT_SUCCEEDED( hr, ... ) (void)(hr)

#else	// !RELEASE

    #define STRINGIFY(x) #x
    #define STRINGIFY_BUILTIN(x) STRINGIFY(x)
    #define ASSERT( isFalse, ... ) \
        if (!(bool)(isFalse)) { \
            Utility::Print("\nAssertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            Utility::PrintSubMessage("\'" #isFalse "\' is false"); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
            __debugbreak(); \
        }

    #define ASSERT_SUCCEEDED( hr, ... ) \
        if (FAILED(hr)) { \
            Utility::Print("\nHRESULT failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            Utility::PrintSubMessage("hr = 0x%08X", hr); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
            __debugbreak(); \
        }


    #define WARN_ONCE_IF( isTrue, ... ) \
    { \
        static bool s_TriggeredWarning = false; \
        if ((bool)(isTrue) && !s_TriggeredWarning) { \
            s_TriggeredWarning = true; \
            Utility::Print("\nWarning issued in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            Utility::PrintSubMessage("\'" #isTrue "\' is true"); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
        } \
    }

    #define WARN_ONCE_IF_NOT( isTrue, ... ) WARN_ONCE_IF(!(isTrue), __VA_ARGS__)

    #define ERROR( ... ) \
        Utility::Print("\nError reported in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
        Utility::PrintSubMessage(__VA_ARGS__); \
        Utility::Print("\n");

    #define DEBUGPRINT( msg, ... ) \
    Utility::Printf( msg "\n", ##__VA_ARGS__ );

#endif

#define BreakIfFailed( hr ) if (FAILED(hr)) __debugbreak()

void SIMDMemCopy( void* __restrict Dest, const void* __restrict Source, size_t NumQuadwords );
void SIMDMemFill( void* __restrict Dest, __m128 FillVector, size_t NumQuadwords );
