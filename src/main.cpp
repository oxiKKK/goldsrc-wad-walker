#include <iostream>
#include <filesystem>
#include <fstream>

#include <windows.h>

#include "wad.h"
#include "argparser.h"

void display_help()
{
	printf( "\n" );
	printf( "--- Help ---\n" );
	printf( "Name usage description\n" );
	printf( "---------------------------\n" );

	for (int32_t i = 0; i < ArgCount; i++)
	{
		const auto arg = &g_ArgumentList[i];
		printf( "%-20s %-30s %s\n", arg->m_name.c_str(), arg->m_usage.c_str(), arg->m_description.c_str() );
	}

	printf( "\n" );
}

void hang()
{
	printf( "Press any key to continue..." );
	std::cin.get();
}

int main( int argc, char** argv )
{
	CArgumentParser parser( argc, argv );

	if (!parser.parse())
	{
		printf( "Error: Couldn't parse arguments.\n" );
		display_help();
		hang();
		return 1;
	}

	if (!g_ArgumentList[ArgFile].m_exists)
	{
		printf("Error: No input file.\n");
		display_help();
		hang();
		return 1;
	}

	auto path = std::filesystem::path( g_ArgumentList[ArgFile].m_value );
	auto basepath = std::filesystem::path( argv[0] ).parent_path();

	if (std::filesystem::is_directory( path ))
	{
		printf( "Error: Can't process a directory. Input a valid file with an valid WAD extension.\n" );
		hang();
		return 1;
	}

	printf( "Processing file:\n" );
	wprintf( L"%s\n", path.wstring().c_str() );
	printf( "\n" );

	if (!std::filesystem::exists( path ))
	{
		//	Search within executable directory
		path = basepath.string() + "\\" + path.filename().string();
		if (!std::filesystem::exists( path ))
		{
			printf( "Error: File don't exist.\n" );
			wprintf( L"%s\n", path.c_str() );
			return 1;
		}
	}

	CWadFile wad( path );

	if (!wad.process())
	{
		printf( "Error: Failed to process WAD file.\n" );
		hang();
		return 0;
	}

	if (g_ArgumentList[ArgDump].m_exists)
		wad.dump_wad_full();

	if (g_ArgumentList[ArgExport].m_exists)
	{
		auto szval = g_ArgumentList[ArgExport].m_value;
		uint32_t miplevel = 1;

		if (szval.size() && std::isdigit( *szval.begin() ))
			miplevel = *szval.begin();

		if (!miplevel)
			miplevel = 1;

		const auto export_path = basepath.string() + "\\images\\";

		if (!std::filesystem::exists( export_path ))
			std::filesystem::create_directory( export_path );

		wad.export_images_from_wad( export_path, miplevel );
	}

	printf( "Success\n" );
	hang();
	return 1;
}
