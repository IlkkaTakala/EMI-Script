#pragma once

class ModuleWrapper
{
	bool Valid = false;
};

class ModuleLoader
{
public:

	static ModuleWrapper LoadModule(const char* path);
};

