#pragma once

#include <View/Detail.hpp>

extern "C"
{
	__declspec(dllexport)
		void draw_dll(tgl::win::HDC _DC, int _X, int _Y, int _Radius);
}