#include <iostream>
#include <fstream>
#include <windows.h>

#include "wad.h"
#include "bmp.h"

#define ADDR "0x%08X"

bool CWadFile::process()
{
	printf( "WAD file process begin\n" );
	printf( "--------------------------------------\n" );

	m_start_timestamp = std::chrono::high_resolution_clock::now();

	const uint32_t filesize = std::filesystem::file_size( m_path );

	if (!(m_buffer = get_buffer_ptr( filesize )))
	{
		printf( "Error: Couldn't get buffer pointer.\n" );
		return false;
	}

	m_wadheader = reinterpret_cast<WadHeader_t*>(m_buffer);

	//	Create a null-terminated wad ID.
	m_wad_id.reserve( 4 );
	for (uint32_t i = 0; i < 4; i++)
		m_wad_id.push_back( m_wadheader->identification[i] );
	
	if (!check_wad_id( m_wad_id ))
	{
		printf( "Error: Invalid WAD id. (%s)\n", m_wad_id.c_str() );
		return false;
	}

	m_wad_id.push_back( '\0' );

	//	Base address of the lump information located inside the wadfile.
	m_lumps_base = m_buffer + m_wadheader->infotableofs;

	printf( "Base of lumps located at " ADDR "\n", (uint32_t)m_lumps_base );

	for (uint32_t i = 0; i < m_wadheader->numlumps; i++)
	{
		printf( "\rProcessing lump #%d", i );

		const auto lumpptr = reinterpret_cast<LumpInfo_t*>(m_lumps_base + i * sizeof( LumpInfo_t ));

		if ((uint8_t*)lumpptr > m_buffer + filesize)
		{
			printf( "\nError: The lump data pointer is out of the range of the WAD file.\n" );
			m_failed = true;
			break;
		}

		//	Something went wrong. Some wad files are fucked up, and we have to check for
		//	them stupidly like this.
		if (!is_lump_valid( lumpptr ))
		{
			printf( "\nError: This WAD file constains corrupted information.\n" );
			m_failed = true;
			break;
		}

		if (!check_lump_size( lumpptr ))
		{
			printf( "\nError: Lump #%d don't fit into max size. (%d bytes) %d bytes exceeded.\n", i, MAXLUMP, lumpptr->size - MAXLUMP );
			m_failed = true;
			break;
		}

		m_lumps.emplace_back( lumpptr );

		const auto miptexptr = reinterpret_cast<MipTexture_t*>(m_buffer + lumpptr->filepos);

		if ((uint8_t*)miptexptr > m_buffer + filesize)
		{
			printf( "\nError: The texture data pointer is out of the range of the WAD file.\n" );
			m_failed = true;
			break;
		}

		if (!is_texture_valid( miptexptr ))
		{
			printf( "\nError: This WAD file constains corrupted information.\n" );
			m_failed = true;
			break;
		}

		TextureData_t tex;
		tex.name = miptexptr->name;
		tex.width = miptexptr->width;
		tex.height = miptexptr->height;

		for (uint32_t m = 0; m < MIPLEVELS; m++)
		{
			//	Each mip is smaller by a half. That means that the n'th mip will be
			//	1 / (2 ^ n) pixels in size from the biggest mip.
			const float scale = 1.f / (1 << m);

			uint32_t width = tex.width * scale;
			uint32_t height = tex.height * scale;

			tex.pixel_data[m].resize( width * height );

			uint8_t* pixeldata_base = (uint8_t*)miptexptr + miptexptr->offsets[m];

			//	Copy the pixel data row by row
			for (uint32_t h = 0; h < height; h++)
				memcpy( tex.pixel_data[m].data() + h * width, pixeldata_base + h * width, width );
		}

		const uint32_t last_mip_pixel_data_size = tex.pixel_data[MIPLEVELS - 1].size();
		uint8_t* palette_base = (uint8_t*)miptexptr + miptexptr->offsets[MIPLEVELS - 1] + last_mip_pixel_data_size;
		
		//	There's a word after the pixel data specifying how many colors 
		//	are inside the palette.
		uint16_t* palette_colors = (uint16_t*)palette_base;
		tex.m_palette_colors = *palette_colors;

		//	The palette data is right after that word.
		const uint32_t word_padding = sizeof(uint16_t);
		uint8_t* pcolordata = palette_base + word_padding;

		tex.m_palette_data.resize( tex.m_palette_colors );

		//	The palette is located after the pixel data of last mip, and after a 2-byte word.
		for (uint32_t entry = 0; entry < tex.m_palette_colors; entry++)
		{
			tex.m_palette_data[entry].Red = *pcolordata++;
			tex.m_palette_data[entry].Green = *pcolordata++;
			tex.m_palette_data[entry].Blue = *pcolordata++;
		}

		m_texturedata.push_back( tex );
	}

	if (m_failed)
	{
		printf( "--------------------------------------\n" );
		return false;
	}

	printf(" ... OK\n");
	printf( "Finished!\n" );

	double duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
		std::chrono::high_resolution_clock::now() - m_start_timestamp).count();

	if (duration > 1000)
		printf( "Took %0.4f seconds to process!\nWOAH that's a big WAD file!\n", duration / 1000.0 );
	else
		printf( "Took %0.4f milliseconds to process!\n", duration );

	//deallocate_buf();

	printf( "--------------------------------------\n" );

	return true;
}

void CWadFile::dump_wad_full()
{
	if (m_failed)
		return;

	dump_wad_header();
	dump_wad_lumps();
	dump_wad_texture_data();
}

void CWadFile::dump_wad_header()
{
	printf( "\n" );
	printf( " Wad information:\n" );
	printf( "\n" );

	printf( "        Identification: %s\n", m_wad_id.c_str() );
	printf( "       Number of lumps: %d\n", m_wadheader->numlumps );
	printf( "   Offset to infotable: " ADDR "\n", m_wadheader->infotableofs );
}

void CWadFile::dump_wad_lumps()
{
	printf( "\n" );
	printf( " Lump information:\n" );
	printf( "\n" );

	printf( "Base of lumps located at " ADDR "\n", (uint32_t)m_lumps_base );
	printf( "\n" );
	printf( "ID   Offset to data   Disk size (KiB)  Uncompressed size (KiB)   Type       Compression   Name\n" );

	uint32_t lump_disk_size_sum = 0, n = 0;

	for (const auto lumpptr : m_lumps)
	{
		const char compression_str[2] = { lumpptr->compression, '\0' };

		printf( "%-4d " ADDR "       %-7.3f          %-7.3f                   %-7s    %3s           ",
				++n,
				lumpptr->filepos,
				lumpptr->disksize / 1024.f, lumpptr->size / 1024.f,
				str_for_lump_type( lumpptr->type ).c_str(), lumpptr->compression ? compression_str : "n/a" );

		lump_disk_size_sum += lumpptr->disksize;

		for (uint32_t i = 0; i < sizeof( lumpptr->name ); i++)
		{
			const char c = lumpptr->name[i];

			if (c >= ' ' && c <= '~')
				putchar( c );
			else if (c == '\0')
				break;
			else putchar( '?' );
		}

		printf( "\n" );
	}

	printf( "\n" );
	printf( "Total size of data inside lumps: %0.3f KiB\n", lump_disk_size_sum / 1024.f );
}

void CWadFile::dump_wad_texture_data()
{
	printf( "\n" );
	printf( " Texture data:\n" );
	printf( "\n" );

	printf( "ID    Resolution  Name\n" );

	uint32_t n = 0;
	for (const auto& tex : m_texturedata)
		printf( "%-4d %3dx%-3d      %s.bmp\n", ++n, tex.width, tex.height, tex.name.c_str() );

	printf( "\n" );
}

bool CWadFile::export_images_from_wad(const std::filesystem::path& to, uint32_t miplevel)
{
	if (miplevel > MIPLEVELS)
	{
		printf("Error: Invalid mip level specified (%d).Maximum is %d", miplevel, MIPLEVELS );
		return false;
	}

	auto start_timestamp = std::chrono::high_resolution_clock::now();

	uint32_t n = 0;
	float percentage = 0;
	for (auto& tex : m_texturedata)
	{
		for (uint32_t m = 0; m < miplevel; m++)
		{
			std::string filename = to.string() + tex.name;

			switch (m)
			{
				case 0: break;
				case 1: filename += "_medium"; break;
				case 2: filename += "_small"; break;
				case 3: filename += "_smallest"; break;
			}

			filename += ".bmp";

			uint8_t* pixel_data = tex.pixel_data[m].data();
			uint8_t* palette_data = (uint8_t*)tex.m_palette_data.data();

			const float scale = 1.f / (1 << m);

			uint32_t width = tex.width * scale;
			uint32_t height = tex.height * scale;

			if (CBitMap::Write(
				filename.c_str(), 
				width, height, 
				pixel_data, 
				palette_data
			) != EBMPResult::Success)
			{
				printf("Error: Couldn't export texture #%d:\n", n);
				printf("%s\n", filename.c_str() );
				return false;
			}

			percentage = float(float(n + m) / float(m_texturedata.size() * miplevel)) * 100.f;

			if (n == m_texturedata.size() - 1)
				percentage = 100.f;

			const auto path = std::filesystem::path( filename ).filename();
			printf("\r                                                                               ");
			printf("\rExporting texture... (%0.1f%%) %s", percentage, path.string().c_str());
		}
		n++;
	}

	double duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
		std::chrono::high_resolution_clock::now() - m_start_timestamp).count();

	printf( "\n" );
	printf( "\n" );

	if (duration > 1000)
		printf( "Took %0.4f seconds to export %d images!\n", duration / 1000.0, n );
	else
		printf( "Took %0.4f milliseconds to export %d images!\n", duration, n );

	printf( "\nDONE!\n" );

	return true;
}

bool CWadFile::check_wad_id( const std::string& id )
{
	return id == "WAD3" || id == "WAD2";
}

bool CWadFile::is_lump_valid( LumpInfo_t* lump )
{
	if (!lump->filepos || !lump->disksize || !lump->size)
		return false;

	return true;
}

bool CWadFile::check_lump_size( LumpInfo_t* lump )
{
	return lump->size < MAXLUMP;
}

std::string CWadFile::str_for_lump_type( char type )
{
	switch (type)
	{
		case LUMP_TYPE_TEXTURE:
			return "texture";
		case LUMP_TYPE_DECAL:
			return "decal";
		case LUMP_TYPE_FONT:
			return "font";
		case LUMP_TYPE_CACHE:
			return "cache";
	}

	return "n/a";
}

bool CWadFile::is_texture_valid( MipTexture_t* miptex )
{
	if (!miptex->width || !miptex->height || !miptex->offsets[0])
		return false;

	return true;
}

uint8_t* CWadFile::allocate_buf( uint32_t size )
{
	return new uint8_t[size];
}

void CWadFile::deallocate_buf()
{
	if (m_buffer)
	{
		delete[] m_buffer;
		m_buffer = nullptr;
	}
}

uint8_t* CWadFile::get_buffer_ptr( uint32_t filesize )
{
	std::ifstream ifs( m_path, std::ios_base::in | std::ios_base::binary );

	if (!ifs.good())
	{
		printf( "Error: Couldn't open input file for reading.\n" );
		return nullptr;
	}

	if (!filesize)
	{
		printf( "Error: Invalid filesize.\n" );
		return nullptr;
	}

	uint8_t* buf = nullptr;
	if (!(buf = allocate_buf( filesize )))
	{
		printf( "Error: Couldn't allocate buffer.\n" );
		return nullptr;
	}

	ifs.read( (char*)buf, filesize );

	printf( "Allocated buffer at 0x%08X with size %d\n", (uint32_t)buf, filesize );

	ifs.close();

	return buf;
}
