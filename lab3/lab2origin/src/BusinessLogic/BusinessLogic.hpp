#include <xutility>
#include <vector>
#include <map>

#include <View/Detail.hpp>

using draw_func_t = void(*)(tgl::win::HDC, int, int, int);

enum class DrawSelector
{
	Internal,
	External,
	Plugins
};

class BusinessLogic
{
public:
	BusinessLogic(std::pair<uint16_t,uint16_t> _Size, std::wstring _FullExeName) noexcept;
	~BusinessLogic();

	void paint(tgl::win::HDC _DC) noexcept;
	void size(uint16_t _Width, uint16_t _Height) noexcept;

	void lb_press(int64_t, uint16_t _X, uint16_t _Y) noexcept;
	void lp_release(int64_t, uint16_t _X, uint16_t _Y) noexcept;

	void mb_press(int64_t, uint16_t _X, uint16_t _Y) noexcept;
	void mb_release(int64_t, uint16_t _X, uint16_t _Y) noexcept;

	void rb_press(int64_t, uint16_t _X, uint16_t _Y) noexcept;
	void rb_release(int64_t, uint16_t _X, uint16_t _Y) noexcept;

	void wheel(uint16_t, int16_t _RotateDist, int32_t _X, int32_t _Y) noexcept;

private:

	void free_modules() noexcept;

	std::vector<std::pair<int32_t, int32_t>> mDrawerPoss;

	bool mLBPress, mRBPress, mMBPress;
	std::pair<uint16_t, uint16_t> mSize;
	int mRadius;
	
	std::map<tgl::win::HMODULE, draw_func_t> mPlugins;
	std::map<tgl::win::HMODULE, draw_func_t>::iterator mCurrentPlugin;

	tgl::win::HBRUSH mBrushHandle;
	fnw::function<void(tgl::win::HDC _DC, int _X, int _Y, int _Radius)> mDrawer;
	DrawSelector mDrawerMethod;
	bool mEditTitle;
	std::wstring mPluginsDir;
};