#ifndef BMP_H
#define BMP_H

#include <iostream>
#include <Windows.h>

enum class EBMPResult : uint32_t
{
	Success,

	InvalidParameter,
	InvalidFilehandle,
	InvalidBitCompression,
	InvalidBitDepth,

	FailFileHeader,
	FailInfoHeader,
	FailPalette,
	FailBitmapBits,
	FailMalloc,
};

class CBitMap
{
public:
	inline static constexpr uint16_t kFileHeaderType = MAKEWORD( 'B', 'M' );
	inline static constexpr uint32_t kColorDepth = ((uint8_t)-1) + 1;
	inline static constexpr uint32_t kNumPlanes = 1;
	inline static constexpr uint32_t kBitDepth = 8;
	inline static constexpr uint32_t kBitCompression = BI_RGB;
	inline static constexpr uint32_t kPaletteSize = 768;

	static EBMPResult Write( const char* szFile, uint32_t width, uint32_t height, uint8_t* pbBits, uint8_t* pbPalette );
	static EBMPResult Read( const char* szFile, uint8_t** ppbBits, uint8_t** ppbPalette );
};

#endif