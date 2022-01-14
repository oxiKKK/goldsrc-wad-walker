#include <iostream>
#include <deque>

#include "argparser.h"

Argument_t g_ArgumentList[ArgCount] =
{
	{ Argument_t::Double, "-f", "<\"path to the file\">", "Specifies the input file" },
	{ Argument_t::Single, "-help", "", "Displayes all arguments" },
	{ Argument_t::Single, "-d", "", "Dumps out the WAD file information" },
	{ Argument_t::Single, "-e", "<miplevel 1-4>", "Exports all textures from the WAD file" },
};

bool CArgumentParser::parse()
{
	if (!validate_args())
	{
		printf("Error: Couldn't parse arguments.\n");
		return false;
	}

	for (int32_t i = 0; i < m_argc; i++)
	{
		for (uint32_t k = 0; k < ArgCount; k++)
		{
			auto arg = &g_ArgumentList[k];

			if (arg->m_name == m_argv[i])
			{
				switch (arg->m_type)
				{
					case Argument_t::Single:
						arg->m_exists = true;
						m_args.push_back( arg );
						break;

					case Argument_t::Double:
						if (m_argv[i + 1])
						{
							arg->m_exists = true;
							arg->m_value = m_argv[i + 1];
							m_args.push_back( arg );
						}
						break;
					case Argument_t::Triple:
						if (m_argv[i + 1] && m_argv[i + 2])
						{
							arg->m_exists = true;
							arg->m_value = m_argv[i + 1];
							arg->m_value1 = m_argv[i + 2];
							m_args.push_back( arg );
						}
						break;
					default:
						break;
				}

				arg->m_exists = true;
			}
		}
	}
}

bool CArgumentParser::validate_args()
{
	//	The first argument is program path
	if (m_argc == 1)
	{
		printf( "Error: No arguments specified.\n" );
		return false;
	}

	if (m_argc > ArgCount + 1)
	{
		printf( "Error: Too much arguments specified.\n" );
		return false;
	}

	return true;
}
