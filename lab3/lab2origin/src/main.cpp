#include <memory>
#include <iostream>

#include <tinyGL.hpp>

#include "BusinessLogic/BusinessLogic.hpp"

//x86: ?draw_dll@@YAXPAUHDC__@win@tgl@@HHH@Z = ?draw_dll@@YAXPAUHDC__@win@tgl@@HHH@Z (void __cdecl draw_dll(struct tgl::win::HDC__ *,int,int,int))
//x64: ?draw_dll@@YAXPEAUHDC__@win@tgl@@HHH@Z = ?draw_dll@@YAXPEAUHDC__@win@tgl@@HHH@Z (void __cdecl draw_dll(struct tgl::win::HDC__ *,int,int,int))

int wmain(int argc, const wchar_t* args[])
{
	auto style = new tgl::Style(args[0], 640, 480);
	*style << tgl::StyleModifier::OverlappedWindow;
	*style >> tgl::StyleModifier::ThickFrame >> tgl::StyleModifier::MaximazeBox;

	auto window = std::make_unique<tgl::View>(style);

	BusinessLogic bsl(window->get_size(), args[0]);//!!!

	decltype(auto) events = window->get_events();
	events.paint.attach(&bsl, &BusinessLogic::paint);
	events.size.attach(&bsl, &BusinessLogic::size);
	events.mouse_lbutton_down.attach(&bsl, &BusinessLogic::lb_press);
	events.mouse_lbutton_up.attach(&bsl, &BusinessLogic::lp_release);
	events.mouse_mbutton_down.attach(&bsl, &BusinessLogic::mb_press);
	events.mouse_mbutton_up.attach(&bsl, &BusinessLogic::mb_release);
	events.mouse_rbutton_down.attach(&bsl, &BusinessLogic::rb_press);
	events.mouse_rbutton_up.attach(&bsl, &BusinessLogic::rb_release);
	events.mouse_wheel.attach(&bsl, &BusinessLogic::wheel);

	for (; window->is_open();)
	{
		auto [update, msg] = tgl::event_pool(60);

		if (!window->is_open())
			continue;

		window->redraw();
	}

	return 0;
}