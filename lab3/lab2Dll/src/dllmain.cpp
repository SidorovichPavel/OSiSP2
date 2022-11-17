// dllmain.cpp : Определяет точку входа для приложения DLL.

#include "declaration.hpp"

#include <random>

tgl::win::BOOL APIENTRY DllMain(tgl::win::HMODULE hModule, tgl::win::DWORD  ul_reason_for_call, tgl::win::LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

constexpr size_t Number = 20;
//constexpr size_t Number = __COUNTER__ % 9 + 3;

void draw_dll(tgl::win::HDC _DC, int _X, int _Y, int _Radius)
{
	tgl::win::LOGBRUSH log_brush;
	log_brush.lbStyle = BS_SOLID;
	log_brush.lbColor = 0xFFFFFF;
	tgl::win::HBRUSH brush_handle = CreateBrushIndirect(&log_brush);

	auto old_handle = SelectObject(_DC, brush_handle);

	std::vector<tgl::win::POINT> points;

	float pi = 3.1415927f;
	float da = 2 * pi / Number;
	for (size_t i = 0; i < Number; i++)
	{
		auto angle = da * i;

		float x = _X + _Radius * std::cos(angle - pi / 2);
		float y = _Y + _Radius * std::sin(angle - pi / 2);
		points.emplace_back(static_cast<int>(x), static_cast<int>(y));
	}

	tgl::win::Polygon(_DC, points.data(), static_cast<int>(points.size()));
	SelectObject(_DC, old_handle);

	DeleteObject(brush_handle);
}

