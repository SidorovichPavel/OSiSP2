#include "BusinessLogic.hpp"

#include <limits>
#include <string>
#include <iostream>

constexpr size_t Number = 7;

extern "C"
{
	__declspec(dllimport)
		void draw_dll(tgl::win::HDC _DC, int _X, int _Y, int _Radius);
}

void draw(tgl::win::HDC _DC, int _X, int _Y, int _Radius)
{
	tgl::win::LOGBRUSH log_brush;
	log_brush.lbStyle = BS_SOLID;
	log_brush.lbColor = 0x2299FF;
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

BusinessLogic::BusinessLogic(std::pair<uint16_t, uint16_t> _Size, std::wstring _FullExeName) noexcept
	:
	mLBPress(false),
	mRBPress(false),
	mMBPress(false),
	mBrushHandle(NULL),
	mSize(_Size),
	mRadius(20),
	mDrawer(draw_dll),
	mDrawerMethod(DrawSelector::External),
	mEditTitle(true)
{
	tgl::win::LOGBRUSH log_brush;
	log_brush.lbStyle = BS_SOLID;
	log_brush.lbColor = 0xEECCCC;

	mBrushHandle = CreateBrushIndirect(&log_brush);

	auto pos = _FullExeName.rfind('\\');
	mPluginsDir = std::wstring(_FullExeName.begin(), _FullExeName.begin() + pos);
	mPluginsDir += L"\\plugins_x64\\*";
}

BusinessLogic::~BusinessLogic()
{
	DeleteObject(mBrushHandle);

	free_modules();
}

void BusinessLogic::paint(tgl::win::HDC _DC) noexcept
{
	tgl::win::RECT rect = { 0,0,mSize.first,mSize.second };

	FillRect(_DC, &rect, mBrushHandle);

	for (auto& pos : mDrawerPoss)
		mDrawer(_DC, pos.first, pos.second, mRadius);
}

void BusinessLogic::size(uint16_t _Width, uint16_t _Height) noexcept
{
	mSize = { _Width,_Height };
}

void BusinessLogic::lb_press(int64_t, uint16_t _X, uint16_t _Y) noexcept
{
	mLBPress = true;
}

void BusinessLogic::lp_release(int64_t, uint16_t _X, uint16_t _Y) noexcept
{
	if (!mLBPress)
		return;

	mLBPress = false;

	mDrawerPoss.emplace_back(_X, _Y);
}

void BusinessLogic::mb_press(int64_t, uint16_t _X, uint16_t _Y) noexcept
{
	mMBPress = true;
}

void BusinessLogic::mb_release(int64_t, uint16_t _X, uint16_t _Y) noexcept
{
	if (!mMBPress)
		return;

	mMBPress = false;
	free_modules();

	using namespace tgl::win;
	WIN32_FIND_DATAW wfd;

	HANDLE const hFind = FindFirstFileW(mPluginsDir.c_str(), &wfd);

	if (INVALID_HANDLE_VALUE != hFind)
	{
		do
		{
			std::wstring file_name = std::wstring(mPluginsDir.begin(), mPluginsDir.end() - 1);
			file_name += wfd.cFileName;

			HMODULE lib = LoadLibraryW(file_name.c_str());
			if (!lib)
				continue;

			auto fn = reinterpret_cast<draw_func_t>(GetProcAddress(lib, "draw_dll"));
			if (fn)
				mPlugins[lib] = fn;
		} while (NULL != FindNextFileW(hFind, &wfd));

		FindClose(hFind);
	}

	mCurrentPlugin = mPlugins.begin();
}

void BusinessLogic::rb_press(int64_t, uint16_t _X, uint16_t _Y) noexcept
{
	mRBPress = true;
}

void BusinessLogic::rb_release(int64_t, uint16_t _X, uint16_t _Y) noexcept
{
	if (!mRBPress)
		return;

	switch (mDrawerMethod)
	{
	case DrawSelector::Internal:
		mDrawer = draw_dll;
		mDrawerMethod = DrawSelector::External;
		break;
	case DrawSelector::External:
		if (mPlugins.empty())
		{
			mDrawer = draw;
			mDrawerMethod = DrawSelector::Internal;
		}
		else
		{
			mDrawer = mCurrentPlugin->second;
			mDrawerMethod = DrawSelector::Plugins;
		}
		break;
	case DrawSelector::Plugins:
		mDrawer = draw;
		mDrawerMethod = DrawSelector::Internal;
		break;
	}
}

void BusinessLogic::wheel(uint16_t, int16_t _RotateDist, int32_t _X, int32_t _Y) noexcept
{
	if (mPlugins.empty() || (mDrawerMethod != DrawSelector::Plugins))
		return;

	auto count = std::abs(_RotateDist) / 120;
	if (_RotateDist > 0)
		for (auto i = 0; i < count; i++)
		{
			if (++mCurrentPlugin == mPlugins.end())
				mCurrentPlugin = mPlugins.begin();
		}
	else
		for (auto i = 0; i < count; i++)
			if (mCurrentPlugin == mPlugins.begin())
			{
				mCurrentPlugin = mPlugins.end();
				--mCurrentPlugin;
			}
			else
				--mCurrentPlugin;

	mDrawer = mCurrentPlugin->second;
}

void BusinessLogic::free_modules() noexcept
{
	for (auto& p : mPlugins)
		tgl::win::FreeLibrary(p.first);

	mPlugins.clear();
	mCurrentPlugin = mPlugins.end();
}
