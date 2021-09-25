#pragma once

#include <imgui.h>

bool operator==(const ImVec2& a, const ImVec2& b) {
	return a.x == b.x && a.y == b.y;
}

bool operator!=(const ImVec2& a, const ImVec2& b) {
	return a.x != b.x || a.y != b.y;
}

ImVec2 operator+(const ImVec2& a, const ImVec2& b) {
	return ImVec2(a.x + b.x, a.y + b.y);
}

ImVec2 operator-(const ImVec2& a, const ImVec2& b) {
	return ImVec2(a.x - b.x, a.y - b.y);
}
