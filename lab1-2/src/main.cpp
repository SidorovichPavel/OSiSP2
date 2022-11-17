#include <tinyGL.hpp>
#include <View/KeyBoard.hpp>

#include <Timer/Timer.hpp>

#include <math/Vector.hpp>

#include <memory>
#include <vector>
#include <iostream>
#include <limits>
#include <functional>
#include <thread>
#include <mutex>

#pragma comment(lib, "Msimg32.lib")

#define LAB1

using vec2f = ta::Vector<float, 2>;
using vec2i = ta::Vector<int, 2>;

namespace tgl::win
{
#include <wingdi.h>
}


class Image
{
	tgl::win::HANDLE mHandle;
	tgl::win::BITMAP mBitmap;

	bool mImage;
	bool mMoveByMouse;
	bool mRButton;

	float mScale;
	float mScrolVal;

	vec2i mCanvasSize;
	vec2f mPosition;
	vec2f mSpeed;
	std::vector<vec2f> mForces;
	vec2i mMousePos;

public:
	Image(int _CanvasWidth, int _CanvasHeight)
		:
		mScale(0.5f),
		mCanvasSize{ _CanvasWidth,_CanvasHeight },
		mImage(true),
		mScrolVal(0),
		mMoveByMouse(false),
		mRButton(false)
	{
		mHandle = tgl::win::LoadImage(NULL, L"kotik.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		if (mHandle)
			tgl::win::GetObject(mHandle, sizeof(mBitmap), &mBitmap);
		else
			throw std::runtime_error("lab1::image load failed");

		mPosition.x() = (_CanvasWidth - mBitmap.bmWidth * mScale) / 2.f;
		mPosition.y() = (_CanvasHeight - mBitmap.bmHeight * mScale) / 2.f;
	}

	~Image()
	{
		tgl::win::DeleteObject(mHandle);
	}

	void on_size(uint16_t _X, uint16_t _Y)
	{
		mCanvasSize = { _X, _Y };
	}

	void draw(tgl::win::HDC _DC)
	{
		tgl::win::RECT rect = { 0,0,mCanvasSize.x(),mCanvasSize.y() };
		tgl::win::LOGBRUSH br;
		br.lbStyle = BS_SOLID;
		br.lbColor = 0xEECCCC;
		tgl::win::HBRUSH brush;
		brush = CreateBrushIndirect(&br);
		FillRect(_DC, &rect, brush);
		DeleteObject(brush);

		if (mImage)
		{
			tgl::win::HDC hmdc = tgl::win::CreateCompatibleDC(_DC);
			tgl::win::SelectObject(hmdc, mHandle);

			tgl::win::TransparentBlt(
				_DC, static_cast<int>(mPosition.x()), static_cast<int>(mPosition.y()), static_cast<int>(mBitmap.bmWidth * mScale), static_cast<int>(mBitmap.bmHeight * mScale),
				hmdc, 0, 0, mBitmap.bmWidth, mBitmap.bmHeight, 0x00ffffff);

			DeleteDC(hmdc);
		}
		else
		{
			vec2i p;
			p.x() = static_cast<int>(mPosition.x());
			p.y() = static_cast<int>(mPosition.y());

			vec2i p2;
			p2 = p + vec2i{ static_cast<int>(mBitmap.bmWidth * mScale), static_cast<int>(mBitmap.bmHeight * mScale) };

			tgl::win::RECT rect = { p.x(), p.y(), p2.x(), p2.y() };
			tgl::win::LOGBRUSH br;
			br.lbStyle = BS_SOLID;
			br.lbColor = 0xEE55aa;
			tgl::win::HBRUSH brush;
			brush = CreateBrushIndirect(&br);
			FillRect(_DC, &rect, brush);
			DeleteObject(brush);
		}
	}

	void update_scale(uint16_t, int16_t _WheelRotateDist, int32_t, int32_t)
	{
		if (_WheelRotateDist > 0)
			mScrolVal += 0.5f;
		else
			mScrolVal -= 0.5f;
	}

	void add_force(const ta::Vector<float, 2>& _Force)
	{
		mForces.push_back(_Force);
	}

	void swap_obj()
	{
		mImage = !mImage;
	}

private:
	vec2f get_main_force()
	{
		vec2f result;
		for (auto& force : mForces)
			result += force;

		mForces.clear();
		return result;
	}

public:
	void update(float t)
	{
		auto ppos = mPosition;

		mPosition += mSpeed * t + get_main_force() * t * t * mBitmap.bmWidth * mBitmap.bmHeight * mScale;
		mSpeed = (mPosition - ppos) / t;

		auto neg = [](auto& e) {e = -e; };

		if (mPosition.x() < 0.f)
		{
			mPosition.x() = 0.f;
			neg(mSpeed.x());
		}
		if (mPosition.x() + mBitmap.bmWidth * mScale >= mCanvasSize.x())
		{
			mPosition.x() = mCanvasSize.x() - mBitmap.bmWidth * mScale;
			neg(mSpeed.x());
		}

		if (mPosition.y() < 0.f)
		{
			mPosition.y() = 0.f;
			neg(mSpeed.y());
		}
		if (mPosition.y() + mBitmap.bmHeight * mScale >= mCanvasSize.y())
		{
			mPosition.y() = mCanvasSize.y() - mBitmap.bmHeight * mScale;
			neg(mSpeed.y());
		}

	}

	vec2f get_speed() noexcept
	{
		return mSpeed;
	}

private:
	void clear_speed() noexcept
	{
		mSpeed = { 0.f,0.f };
	}

public:
	float get_scrol_v()
	{
		return abs(mScrolVal) < std::numeric_limits<float>::epsilon() ? 0.f : mScrolVal;
	}

	void clear_scrol_v()
	{
		mScrolVal = 0.f;
	}

	void on_lb_press(int64_t, int32_t, int32_t)
	{
		mMoveByMouse = true;
	}

	void on_lb_release(int64_t, int32_t, int32_t)
	{
		mMoveByMouse = false;
	}

	bool is_move_by_mouse()
	{
		return mMoveByMouse;
	}

	void on_mouse_shift(int32_t x, int32_t y)
	{
		if (mMoveByMouse)
			mPosition += vec2f{ static_cast<float>(x),static_cast<float>(y) };
	}

	void on_rb_press(int64_t, int32_t, int32_t)
	{
		mRButton = true;
	}

	void on_rb_release(int64_t, int32_t, int32_t)
	{
		if (mRButton)
			clear_speed();

		mRButton = false;
	}
};

#ifndef LAB1

std::atomic<bool> STOP = false;
struct Data
{
	Image* img;
	float& t;
};

void CALLBACK wtProc(tgl::win::LPVOID lpArg, tgl::win::DWORD dwTimerLowValue, tgl::win::DWORD dwTimerHighValue)
{
	auto data = reinterpret_cast<Data*>(lpArg);

	data->img->update(data->t);
}

void thread(Image* _Img, float& _Time)
{
	using namespace tgl::win;

	HANDLE hTimer = NULL;

	hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	if (hTimer == NULL)
	{
		std::cout << "INVALID_HANDLE_VALUE on Timer create" << std::endl;
		return;
	}

	LARGE_INTEGER dueTime;
	dueTime.QuadPart = -10000000ll;
	Data d(_Img, _Time);
	if (!SetWaitableTimer(hTimer, &dueTime, 16, wtProc, &d, 0))
	{
		std::cout << "Set Waitable timer failed" << std::endl;
		return;
	}

	for (; !STOP;)
	{
		SleepEx(0, TRUE);
		WaitForSingleObject(hTimer, INFINITE);
	}
}

#endif

void processing(tgl::View& _View, tgl::KeyBoard& _Keyboard, Image& _Img, float _Duration)
{
	float coef = 0.25f * _Duration;

	using tgl::KeyCode;
	if (_Keyboard[KeyCode::Escape])
		_View.destroy();

	if (!_Img.is_move_by_mouse())
	{
		if (_Keyboard[KeyCode::W] || _Keyboard[KeyCode::Up])
			_Img.add_force(vec2f{ 0, -1 * coef });
		if (_Keyboard[KeyCode::S] || _Keyboard[KeyCode::Down])
			_Img.add_force(vec2f{ 0, 1 * coef });
		if (_Keyboard[KeyCode::D] || _Keyboard[KeyCode::Right])
			_Img.add_force(vec2f{ 1 * coef, 0 });
		if (_Keyboard[KeyCode::A] || _Keyboard[KeyCode::Left])
			_Img.add_force(vec2f{ -1 * coef, 0 });
	}

	auto v = _Img.get_scrol_v();
	_Img.clear_scrol_v();
	if (_Keyboard[KeyCode::Shift])
		_Img.add_force(vec2f{ 0,v * coef });
	else
		_Img.add_force(vec2f{ v * coef,0 });

	if (_Keyboard.get_key_count(VK_RETURN) > 0)
	{
		for (auto i = _Keyboard.get_key_count(VK_RETURN); i > 0; i--)
			_Img.swap_obj();

		_Keyboard.clear_key_count(VK_RETURN);
	}

	auto speed = _Img.get_speed();
	_Img.add_force(speed * -0.000002f);
}

int main(int argc, char* args[])
{
	auto constexpr start_width = 1920, start_height = 1000;
	auto style = new tgl::Style(L"lab1", 0, 0, start_width, start_height);
	*style << tgl::StyleModifier::OverlappedWindow;

	auto window = std::make_unique<tgl::View>(style);

	auto keyboard = std::make_unique<tgl::KeyBoard>();

	std::unique_ptr<Image> image;
	try
	{
		image = std::make_unique<Image>(start_width, start_height);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return -1;
	}

	decltype(auto) events = window->get_events();
	events.size.attach(image.get(), &Image::on_size);
	events.paint.attach(image.get(), &Image::draw);
	events.mouse_wheel.attach(image.get(), &Image::update_scale);
	events.key_down.attach(keyboard.get(), &tgl::KeyBoard::key_down);
	events.key_up.attach(keyboard.get(), &tgl::KeyBoard::key_up);
	events.mouse_rbutton_down.attach(image.get(), &Image::on_rb_press);
	events.mouse_rbutton_up.attach(image.get(), &Image::on_rb_release);
	events.mouse_lbutton_down.attach(image.get(), &Image::on_lb_press);
	events.mouse_lbutton_up.attach(image.get(), &Image::on_lb_release);
	window->mouse_raw_input(true);
	events.mouse_shift.attach(image.get(), &Image::on_mouse_shift);

#ifdef LAB1
	for (; window->is_open();)
	{
		auto [update, msg] = tgl::event_pool(60);
		processing(*window, *keyboard, *image, tgl::FrameTimeInfo.s());

		image->update(tgl::FrameTimeInfo.s());

		window->redraw();
	}
#elif
	tgl::Timer timer(window->get_handle(), 16);
	timer.booom.attach(window.get(), &tgl::View::redraw);

	float f = tgl::FrameTimeInfo.s();
	std::thread th(thread, image.get(), std::ref(f));

	for (; window->is_open();)
	{
		auto [update, msg] = tgl::event_pool(60);
		f = tgl::FrameTimeInfo.s();
		processing(*window, *keyboard, *image, tgl::FrameTimeInfo.s());

		//image->update(tgl::FrameTimeInfo.s());
	}

	STOP = true;
	th.join();
#endif
	

	return 0;
}