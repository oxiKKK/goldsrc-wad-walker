#ifndef _C_ARGUMENT_PARSER_
#define _C_ARGUMENT_PARSER_

#pragma once

class Argument_t
{
public:
	enum Type
	{
		Single,
		Double,
		Triple,
	};

public:
	Argument_t( Type type, const std::string& name, const std::string& usage, const std::string& description ) :
		m_type( type ),
		m_name( name ),
		m_usage( usage ),
		m_description( description )
	{}

public:
	Type m_type;

	std::string m_name;
	std::string m_value;
	std::string m_value1;

	std::string m_usage;
	std::string m_description;

	bool m_exists;
};

enum Arguments
{
	ArgFile,
	ArgHelp,
	ArgDump,
	ArgExport,

	ArgCount
};

extern Argument_t g_ArgumentList[ArgCount];

class CArgumentParser
{
private:
	bool validate_args();

public:
	bool parse();

	CArgumentParser( int32_t argc, char** argv ) :
		m_argc(argc), 
		m_argv(argv)
	{}

	int32_t m_argc;
	char** m_argv;

	std::deque<Argument_t*> m_args{ ArgCount };
};

#endif