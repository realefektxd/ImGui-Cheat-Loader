#pragma once
#include <unordered_map>

class c_globals {
public:
	bool active = true;
	char user_name[255];
	char pass_word[255];
};

inline c_globals globals;