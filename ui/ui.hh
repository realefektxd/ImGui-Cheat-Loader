#pragma once
#include <d3d9.h>
#include "../imgui/imgui.h"
//#include "../api/auth.hpp"
//#include "../api/json.hpp"
//#include "../api/utils.hpp"
//#include "../api/XorStr.hpp"

namespace ui {
	void init(LPDIRECT3DDEVICE9);
	void render();
	inline bool show_loader = true;
	inline float loader_timer = 0.f;
	inline float login_alpha = 0.f;
}

namespace ui {
	inline LPDIRECT3DDEVICE9 dev;
	inline const char* window_title = "Frost.CC | LUA MENU";
	inline ImFont* title_font = nullptr;
	inline ImFont* bold_font = nullptr;

	struct FontScope {
		FontScope(ImFont* f);
		FontScope(const FontScope&) = delete;
		FontScope& operator=(const FontScope&) = delete;
		FontScope(FontScope&& other) noexcept;
		FontScope& operator=(FontScope&& other) noexcept;
		~FontScope();
		void end();
	private:
		ImFont* font;
		bool active;
	};

	FontScope Regular(float size = 0.0f);
	FontScope Bold(float size = 0.0f);
}

namespace ui {
	inline ImVec2 screen_res{ 000, 000 };
	inline ImVec2 window_pos{ 0, 0 };
	inline ImVec2 window_size{ 620, 380 };
	inline DWORD  window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
}