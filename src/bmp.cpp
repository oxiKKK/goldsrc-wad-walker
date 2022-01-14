#include "bmp.h"

#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

EBMPResult CBitMap::Write( const char* szFile, uint32_t width, uint32_t height, uint8_t* pbBits, uint8_t* pbPalette )
{
	// Bogus parameter check
	if (!pbPalette || !pbBits)
	{
		printf( "Error: Invalid parameter passed: %#x %#x\n", pbPalette, pbBits );
		return EBMPResult::InvalidParameter;
	}

	// File exists?
	const auto pfile = fopen( szFile, "wb" );
	if (!pfile)
	{
		printf( "Error: Invalid filehandle (%s)\n", szFile );
		return EBMPResult::InvalidFilehandle;
	}

	ULONG biTrueWidth = ((width + 3) & ~3);
	ULONG cbBmpBits = biTrueWidth * height;
	ULONG cbPalBytes = kColorDepth * sizeof( RGBQUAD );

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	// Bogus file header check
	bmfh.bfType = kFileHeaderType;
	bmfh.bfSize = sizeof( bmfh ) + sizeof( bmih ) + cbBmpBits + cbPalBytes;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof( bmfh ) + sizeof( bmih ) + cbPalBytes;

	// Write file header
	if (fwrite( &bmfh, sizeof( bmfh ), sizeof( uint8_t ), pfile ) != sizeof( uint8_t ))
	{
		fclose( pfile );
		printf( "Error: Failed to write file header\n" );
		return EBMPResult::FailFileHeader;
	}

	// Size of structure
	bmih.biSize = sizeof( bmih );
	// Width
	bmih.biWidth = biTrueWidth;
	// Height
	bmih.biHeight = height;
	// Only 1 plane 
	bmih.biPlanes = kNumPlanes;
	// Only 8-bit supported.
	bmih.biBitCount = kBitDepth;
	// Only non-compressed supported.
	bmih.biCompression = kBitCompression;
	bmih.biSizeImage = 0;

	// huh?
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;

	// Always full palette
	bmih.biClrUsed = kColorDepth;
	bmih.biClrImportant = 0;

	// Write info header
	if (fwrite( &bmih, sizeof( bmih ), sizeof( uint8_t ), pfile ) != sizeof( uint8_t ))
	{
		fclose( pfile );
		printf( "Error: Failed to write info header\n" );
		return EBMPResult::FailInfoHeader;
	}

	// convert to expanded palette
	uint8_t* pb = pbPalette;

	// Copy over used entries
	RGBQUAD rgrgbPalette[kColorDepth];
	for (int32_t i = 0; i < (int32_t)bmih.biClrUsed; i++)
	{
		rgrgbPalette[i].rgbRed = *pb++;
		rgrgbPalette[i].rgbGreen = *pb++;
		rgrgbPalette[i].rgbBlue = *pb++;
		rgrgbPalette[i].rgbReserved = 0;
	}

	// Write palette (bmih.biClrUsed entries)
	cbPalBytes = bmih.biClrUsed * sizeof( RGBQUAD );
	if (fwrite( rgrgbPalette, cbPalBytes, sizeof( uint8_t ), pfile ) != sizeof( uint8_t ))
	{
		fclose( pfile );
		printf( "Error: Failed to write palette (bmih.biClrUsed entries)\n" );
		return EBMPResult::FailPalette;
	}

	uint8_t* pbBmpBits = (uint8_t*)malloc( cbBmpBits );

	if (!pbBmpBits)
	{
		fclose( pfile );
		printf( "Error: Failed to allocate memory\n" );
		return EBMPResult::FailMalloc;
	}

	pb = pbBits;
	// reverse the order of the data.
	pb += (height - 1) * width;
	for (int32_t i = 0; i < bmih.biHeight; i++)
	{
		memmove( &pbBmpBits[biTrueWidth * i], pb, width );
		pb -= width;
	}

	// Write bitmap bits (remainder of file)
	if (fwrite( pbBmpBits, cbBmpBits, sizeof( uint8_t ), pfile ) != sizeof( uint8_t ))
	{
		printf( "Error: Failed to write bitmap bits (remainder of file)\n" );
		free( pbBmpBits );
		fclose( pfile );
		return EBMPResult::FailBitmapBits;
	}

	free( pbBmpBits );
	fclose( pfile );

	return EBMPResult::Success;
}

EBMPResult CBitMap::Read( const char* szFile, uint8_t** ppbBits, uint8_t** ppbPalette )
{
	// Bogus parameter check
	if (!ppbPalette || !ppbBits)
	{
		printf( "Error: Invalid parameter passed: %#x %#x\n", ppbPalette, ppbBits );
		return EBMPResult::InvalidParameter;
	}

	// File exists?
	const auto pfile = fopen( szFile, "rb" );
	if (!pfile)
	{
		printf( "Error: Invalid filehandle (%s)\n", szFile );
		return EBMPResult::InvalidFilehandle;
	}

	// Read file header
	BITMAPFILEHEADER bmfh;
	if (fread( &bmfh, sizeof( bmfh ), sizeof( uint8_t ), pfile ) != sizeof( uint8_t ))
	{
		fclose( pfile );
		printf( "Error: Failed to read file header\n" );
		return EBMPResult::FailFileHeader;
	}

	// Bogus file header check
	if (!(bmfh.bfReserved1 == 0 && bmfh.bfReserved2 == 0))
	{
		fclose( pfile );
		printf( "Error: Failed to read file header\n" );
		return EBMPResult::FailFileHeader;
	}

	// Read info header
	BITMAPINFOHEADER bmih;
	if (fread( &bmih, sizeof( bmih ), sizeof( uint8_t ), pfile ) != sizeof( uint8_t ))
	{
		fclose( pfile );
		printf( "Error: Failed to read info header\n" );
		return EBMPResult::FailInfoHeader;
	}

	// Bogus info header check
	if (!(bmih.biSize == sizeof( bmih ) && bmih.biPlanes == 1))
	{
		fclose( pfile );
		printf( "Error: Failed to read info header\n" );
		return EBMPResult::FailInfoHeader;
	}

	// Bogus bit depth? Only 8-bit supported.
	if (bmih.biBitCount != kBitDepth)
	{
		fclose( pfile );
		printf( "Error: Invalid bit depth: %d\n", bmih.biBitCount );
		return EBMPResult::InvalidBitDepth;
	}

	// Bogus compression? Only non-compressed supported.
	if (bmih.biCompression != kBitCompression)
	{
		fclose( pfile );
		printf( "Error: Invalid bit compression: %d\n", bmih.biCompression );
		return EBMPResult::InvalidBitCompression;
	}

	// Figure out how many entires are actually in the table
	ULONG cbPalBytes = bmih.biClrUsed * sizeof( RGBQUAD );
	if (bmih.biClrUsed == 0)
	{
		bmih.biClrUsed = kColorDepth;
		cbPalBytes = (1 << bmih.biBitCount) * sizeof( RGBQUAD );
	}

	// Read palette (bmih.biClrUsed entries)
	RGBQUAD rgrgbPalette[kColorDepth];
	if (fread( rgrgbPalette, cbPalBytes, 1/*count*/, pfile ) != 1)
	{
		fclose( pfile );
		printf( "Error: Failed to read palette (bmih.biClrUsed entries)\n" );
		return EBMPResult::FailPalette;
	}

	// convert to a packed 768 uint8_t palette
	uint8_t* pbPal = (uint8_t*)malloc( kPaletteSize );
	if (!pbPal)
	{
		fclose( pfile );
		printf( "Error: Failed to allocate memory\n" );
		return EBMPResult::FailMalloc;
	}

	// Copy over used entries
	uint8_t* pb = pbPal;
	for (int32_t i = 0; i < (int32_t)bmih.biClrUsed; i++)
	{
		*pb++ = rgrgbPalette[i].rgbRed;
		*pb++ = rgrgbPalette[i].rgbGreen;
		*pb++ = rgrgbPalette[i].rgbBlue;
	}

	// Fill in unused entires will 0,0,0
	for (int32_t i = bmih.biClrUsed; i < kColorDepth; i++)
	{
		*pb++ = 0;
		*pb++ = 0;
		*pb++ = 0;
	}

	// Read bitmap bits (remainder of file)
	ULONG cbBmpBits = bmfh.bfSize - ftell( pfile );
	pb = (uint8_t*)malloc( cbBmpBits );
	if (!pb)
	{
		printf( "Error: Failed to allocate memory\n" );
		fclose( pfile );
		free( pbPal );
		free( pb );
		return EBMPResult::FailMalloc;
	}

	// Read bitmap bits (remainder of file)
	if (fread( pb, cbBmpBits, sizeof( uint8_t ), pfile ) != sizeof( uint8_t ))
	{
		printf( "Error: Failed to read bitmap bits (remainder of file)\n" );
		fclose( pfile );
		free( pbPal );
		free( pb );
		return EBMPResult::FailBitmapBits;
	}

	uint8_t* pbBmpBits = (uint8_t*)malloc( cbBmpBits );
	if (!pbBmpBits)
	{
		printf( "Error: Failed to allocate memory\n" );
		fclose( pfile );
		free( pbPal );
		free( pb );
		free( pbBmpBits );
		return EBMPResult::FailMalloc;
	}

	// data is actually stored with the width being rounded up to a multiple of 4
	int32_t biTrueWidth = (bmih.biWidth + 3) & ~3;

	// reverse the order of the data.
	pb += (bmih.biHeight - 1) * biTrueWidth;

	for (int32_t i = 0; i < bmih.biHeight; i++, pb -= biTrueWidth)
		memmove( &pbBmpBits[biTrueWidth * i], pb, biTrueWidth );

	pb += biTrueWidth;

	// Set output parameters
	*ppbPalette = pbPal;
	*ppbBits = pbBmpBits;

	fclose( pfile );
	free( pb );

	return EBMPResult::Success;
}