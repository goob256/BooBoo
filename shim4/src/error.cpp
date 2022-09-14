#include "shim4/error.h"

using namespace noo;

namespace noo {

namespace util {

Error::Error()
{
}

Error::Error(std::string error_message) : error_message(error_message)
{
}

Error::~Error()
{
}

MemoryError::MemoryError(std::string error_message)
{
	this->error_message = "Memory error: " + error_message;
}

MemoryError::~MemoryError()
{
}

LoadError::LoadError(std::string error_message)
{
	this->error_message = "Load error: " + error_message;
}

LoadError::~LoadError()
{
}

FileNotFoundError::FileNotFoundError(std::string error_message)
{
	this->error_message = "File not found: " + error_message;
}

FileNotFoundError::~FileNotFoundError()
{
}

GLError::GLError(std::string error_message)
{
	this->error_message = "OpenGL error: " + error_message;
}

GLError::~GLError()
{
}

} // End namespace util

} // End namespace noo
