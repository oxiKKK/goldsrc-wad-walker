#ifndef WAD_H
#define WAD_H

#pragma once

#include <deque>
#include <chrono>
#include <climits>
#include <array>
#include <filesystem>

//	Windows.h stupidity.
#ifdef max
#	undef max
#endif

#define	MIPLEVELS			4

#define MAXLUMP				(640 * 480 * 85 / 64)

//	I've defined these myself. There is no public definition of which is which.
//	Or atleast I didn't found any.
#define LUMP_TYPE_TEXTURE	'C' // 0x43
#define LUMP_TYPE_DECAL		'B' // 0x42
#define LUMP_TYPE_FONT		'F' // 0x46
#define LUMP_TYPE_CACHE		'@' // 0x40

//	Wad file is made out of lumps. Each lump contains the 
//	file position offset where the specific information 
//	belonging to the lump is located.
struct LumpInfo_t
{
	uint32_t filepos;

	int32_t disksize;
	int32_t size; // uncompressed

	char type;
	char compression;
	char pad1;
	char pad2;

	char name[16]; // must be null terminated
};

struct MipTexture_t
{
	char	 name[16];
	uint32_t width, height;

	//	Contains relative offset from the base of this
	//	structure. The pixel data for the first mip 
	//	should lay on -> baseofmip + sizeof(MipTexture_t).
	uint32_t offsets[MIPLEVELS]; 
};

//	Palette contains of 256-color data
struct ColorData_t
{
	uint8_t Red, Green, Blue;
};

//	Texture data we can obtain from the MipTexture_t
struct TextureData_t
{
	std::string name;
	uint32_t width, height;

	//	Pixel data for all mip levels
	std::array<std::vector<uint8_t>, MIPLEVELS> pixel_data;

	//	After the pixel data for last mip is a word which specifies how many colors
	//	there are inside the palette and right after that there's the palette data.
	//	
	//	The palette consists of three 1-byte color data - RGB.
	//	There are usually 256 colors in each entry, because the byte is 8-bits in length, 
	//	that means one byte can hold max up to 256 values:
	//	[0 - 256) values -> 2 ^ sizeof(byte) == 256
	uint16_t m_palette_colors;
	std::vector<ColorData_t> m_palette_data;
};

//	This is the information about the wad file that is 
//	in the beggining of the buffer.
struct WadHeader_t
{
	char	 identification[4]; // WAD2/WAD3

	uint32_t numlumps;
	uint32_t infotableofs;
};

class CWadFile
{
public:
	CWadFile( const std::filesystem::path& path ) :
		m_path(path)
	{}

	CWadFile() = delete;

	bool process();

	//	Dumping
	void dump_wad_full();
	void dump_wad_header();
	void dump_wad_lumps();
	void dump_wad_texture_data();

	bool export_images_from_wad( const std::filesystem::path& to, uint32_t miplevel );

	//	WAD id check
	static bool check_wad_id( const std::string& id );

	//	Lumps
	static bool is_lump_valid( LumpInfo_t* lump );
	static bool check_lump_size( LumpInfo_t* lump );
	static std::string str_for_lump_type( char type );

	//	Texture data
	bool is_texture_valid( MipTexture_t* miptex );

private:
	uint8_t* allocate_buf( uint32_t size );
	void deallocate_buf();
	uint8_t* get_buffer_ptr( uint32_t filesize );

public:
	std::filesystem::path m_path;
	uint8_t* m_buffer;

	std::string m_wad_id; // A null-terminated wad id
	WadHeader_t* m_wadheader;

	uint8_t* m_lumps_base;
	std::deque<LumpInfo_t*> m_lumps;

	std::deque<MipTexture_t*> m_miptextures;
	std::deque<TextureData_t> m_texturedata;

	std::chrono::high_resolution_clock::time_point m_start_timestamp;

	//	This is set to true if some error occured and process has to stop.
	bool m_failed = false;
};

#endif