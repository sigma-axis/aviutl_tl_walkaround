/*
The MIT License (MIT)

Copyright (c) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cmath>
#include <algorithm>
#include <optional>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

using namespace AviUtl;
using namespace ExEdit;

////////////////////////////////
// 主要情報源の変数アドレス．
////////////////////////////////
inline constinit struct ExEdit092 {
	FilterPlugin* fp;
	constexpr static const char* info_exedit092 = "拡張編集(exedit) version 0.92 by ＫＥＮくん";
	bool init(FilterPlugin* this_fp)
	{
		if (fp != nullptr) return true;
		SysInfo si; this_fp->exfunc->get_sys_info(nullptr, &si);
		for (int i = 0; i < si.filter_n; i++) {
			auto that_fp = this_fp->exfunc->get_filterp(i);
			if (that_fp->information != nullptr &&
				0 == std::strcmp(that_fp->information, info_exedit092)) {
				fp = that_fp;
				init_pointers();
				return true;
			}
		}
		return false;
	}

	int32_t* SortedObjectLayerBeginIndex;	// 0x149670
	int32_t* SortedObjectLayerEndIndex;		// 0x135ac8
	Object** SortedObject;					// 0x168fa8
	Object** ObjectArray_ptr;				// 0x1e0fa4
	int32_t* NextObjectIdxArray;			// 0x1592d8
	int32_t* SettingDialogObjectIndex;		// 0x177a10
	LayerSetting* LayerSettings;			// 0x188498
	int32_t* current_scene;					// 0x1a5310
	int32_t* curr_timeline_scale_len;		// 0x0a3fc8
	int32_t* scroll_follows_cursor;			// 0x1790d0
	HWND*	 timeline_h_scroll_bar;			// 0x179108
	int32_t* timeline_h_scroll_pos;			// 0x1a52f0
	int32_t* timeline_width_in_frames;		// 0x1a52f8
	HWND*	 timeline_v_scroll_bar;			// 0x158d34
	int32_t* timeline_v_scroll_pos;			// 0x1a5308
	int32_t* timeline_height_in_layers;		// 0x0a3fbc

private:
	void init_pointers()
	{
		auto pick_addr = [this,exedit_base = reinterpret_cast<uintptr_t>(fp->dll_hinst)]
			<class T>(T& target, ptrdiff_t offset) { target = reinterpret_cast<T>(exedit_base + offset); };
		pick_addr(SortedObjectLayerBeginIndex,	0x149670);
		pick_addr(SortedObjectLayerEndIndex,	0x135ac8);
		pick_addr(SortedObject,					0x168fa8);
		pick_addr(ObjectArray_ptr,				0x1e0fa4);
		pick_addr(NextObjectIdxArray,			0x1592d8);
		pick_addr(SettingDialogObjectIndex,		0x177a10);
		pick_addr(LayerSettings,				0x188498);
		pick_addr(current_scene,				0x1a5310);
		pick_addr(curr_timeline_scale_len,		0x0a3fc8);
		pick_addr(scroll_follows_cursor,		0x1790d0);
		pick_addr(timeline_h_scroll_bar,		0x179108);
		pick_addr(timeline_h_scroll_pos,		0x1a52f0);
		pick_addr(timeline_width_in_frames,		0x1a52f8);
		pick_addr(timeline_v_scroll_bar,		0x158d34);
		pick_addr(timeline_v_scroll_pos,		0x1a5308);
		pick_addr(timeline_height_in_layers,	0x0a3fbc);
	}
} exedit;


////////////////////////////////
// 探索関数コア部．
////////////////////////////////
struct Timeline {
	static bool is_active_direct(const Object* obj) {
		return has_flag(obj->filter_status[0], Object::FilterStatus::Active);
	}
	static bool is_active(const Object* obj) {
		auto i = obj->index_midpt_leader;
		return is_active_direct(i < 0 ? obj : &(*exedit.ObjectArray_ptr)[i]);
	}
	static bool is_hidden(const LayerSetting& setting) {
		return has_flag(setting.flag, LayerSetting::Flag::UnDisp);
	}
	static int chain_begin(const Object* obj)
	{
		if (auto i = obj->index_midpt_leader; i >= 0) obj = &(*exedit.ObjectArray_ptr)[i];
		return obj->frame_begin;
	}
	static int chain_end(const Object* obj)
	{
		if (auto i = obj->index_midpt_leader; i >= 0) {
			int j;
			while (j = exedit.NextObjectIdxArray[i], j >= 0) i = j;
			obj = &(*exedit.ObjectArray_ptr)[i];
		}
		return obj->frame_end;
	}

#define to_skip(obj)		(skip_inactives && !is_active(obj))
#define chain_begin(obj)	(skip_midpoints ? chain_begin(obj) : (obj)->frame_begin)
#define chain_end(obj)		(skip_midpoints ? chain_end(obj) : (obj)->frame_end)
	static int find_adjacent_left(int pos, int layer, bool skip_midpoints, bool skip_inactives)
	{
		// 指定された位置より左にある最も近い境界を探す．
		int idx_L = exedit.SortedObjectLayerBeginIndex[layer], i0 = idx_L,
			idx_R = exedit.SortedObjectLayerEndIndex[layer];
		if (idx_L > idx_R) return 0; // 空レイヤーなので最左端を return.

		// まず左端のオブジェクトを見て，指定位置とわかりやすい位置関係にあるなら return.
		if (auto obj = exedit.SortedObject[idx_L]; pos <= obj->frame_end + 1)
			return pos <= obj->frame_begin || to_skip(obj) ? 0 : obj->frame_begin;

		// 右端も見てわかりやすい位置関係なら，後続のループを飛ばすように仕掛ける．
		if (exedit.SortedObject[idx_R]->frame_begin < pos) idx_L = idx_R;

		// 2分法でインデックスの差が1以下になるまで検索，
		// idx_L のオブジェクトが pos と重なるか，最も近い左側に位置するように探す．
		// 途中またわかりやすいものがあったらそこで return... してたけど恩恵を受ける場面は少なそう．
		while (idx_L + 1 < idx_R) {
			auto idx = (idx_L + idx_R) >> 1;
			if (exedit.SortedObject[idx]->frame_begin < pos) idx_L = idx; else idx_R = idx;
		}

		// 目的オブジェクトが見つかったので return, スキップ対象のときはそこを起点に線形探索．
		if (auto obj = exedit.SortedObject[idx_L]; !to_skip(obj))
			// obj->frame_end + 1 < pos の場合，pos の位置には何もないので obj_chain_end() は不要．
			return pos <= obj->frame_end + 1 ? chain_begin(obj) : obj->frame_end + 1;

		// 無効でない最も近いオブジェクトを探すのは線形探索しか方法がない．
		while (i0 <= --idx_L) {
			// 初期 idx_L が無効オブジェクトでスキップされたという前提があるため，
			// 最初に見つかった有効オブジェクトの終了点は中間点でなくオブジェクト境界になる．
			// なので skip_midpoints の確認や obj_chain_end() 呼び出しは必要ない．
			if (auto obj = exedit.SortedObject[idx_L]; is_active(obj))
				return obj->frame_end + 1;
		}
		return 0;
	}

	// the return value of -1 stands for the end of the scene.
	static int find_adjacent_right(int pos, int layer, bool skip_midpoints, bool skip_inactives)
	{
		// 上の関数の左右入れ替えただけ．
		int idx_L = exedit.SortedObjectLayerBeginIndex[layer],
			idx_R = exedit.SortedObjectLayerEndIndex[layer], i1 = idx_R;
		if (idx_L > idx_R) return -1;

		if (auto obj = exedit.SortedObject[idx_R]; obj->frame_begin <= pos)
			return pos < obj->frame_end + 1 && !to_skip(obj) ? obj->frame_end + 1 : -1;

		if (pos < exedit.SortedObject[idx_L]->frame_end + 1) idx_R = idx_L;

		while (idx_L + 1 < idx_R) {
			auto idx = (idx_L + idx_R) >> 1;
			if (exedit.SortedObject[idx]->frame_end + 1 <= pos) idx_L = idx; else idx_R = idx;
		}

		if (auto obj = exedit.SortedObject[idx_R]; !to_skip(obj))
			return pos < obj->frame_begin ? obj->frame_begin : chain_end(obj) + 1;

		while (++idx_R <= i1) {
			if (auto obj = exedit.SortedObject[idx_R]; is_active(obj))
				return obj->frame_begin;
		}
		return -1;
	}
#undef to_skip
#undef chain_begin
#undef chain_end
	// 「-1 で最右端」規格がめんどくさいのでラップ．
	static int find_adjacent_right(int pos, int layer,
		bool skip_midpoints, bool skip_inactives, int len) {
		pos = find_adjacent_right(pos, layer, skip_midpoints, skip_inactives);
		return 0 <= pos && pos < len ? pos : len - 1;
	}
};


////////////////////////////////
// Windows API 利用の補助関数．
////////////////////////////////

template<int N>
bool check_window_class(HWND hwnd, const wchar_t (&classname)[N])
{
	wchar_t buf[N + 1];
	return ::GetClassNameW(hwnd, buf, N + 1) + 1 == N && std::wcscmp(buf, classname) == 0;
}

// キー入力認識をちょろまかす補助クラス．
class ForceKeyState {
	static auto force_key_state(short vkey, uint8_t state)
	{
		uint8_t states[256]; std::ignore = ::GetKeyboardState(states);
		std::swap(state, states[vkey]); ::SetKeyboardState(states);
		return state;
	}

	static auto state(bool pressed) { return pressed ? key_pressed : key_released; }

	const short vkey;
	const uint8_t prev;

public:
	constexpr static uint8_t key_released = 0, key_pressed = 0x80;
	constexpr static int vkey_invalid = -1;

	ForceKeyState(short vkey, bool pressed) : ForceKeyState(vkey, state(pressed)) {}
	ForceKeyState(short vkey, uint8_t state) :
		vkey{ 0 <= vkey && vkey < 256 ? vkey : vkey_invalid },
		prev{ this->vkey != vkey_invalid ? force_key_state(this->vkey, state) : uint8_t{} } {}
	~ForceKeyState() { if (vkey != vkey_invalid) force_key_state(vkey, prev); }

	ForceKeyState(const ForceKeyState&) = delete;
	ForceKeyState& operator=(const ForceKeyState&) = delete;
};

// タイムラインのスクロールバー操作．
class TimelineScrollBar {
	HWND* const& phwnd;
	const uint32_t hv_message;

public:
	constexpr TimelineScrollBar(bool horizontal)
		: phwnd{ horizontal ? exedit.timeline_h_scroll_bar : exedit.timeline_v_scroll_bar }
		, hv_message{ static_cast<uint32_t>(horizontal ? WM_HSCROLL : WM_VSCROLL) } {}

	void set_pos(int pos, EditHandle* editp) const
	{
		auto hwnd = *phwnd;
		if (hwnd == nullptr) return;

		const SCROLLINFO si{ .cbSize = sizeof(si), .fMask = SIF_POS, .nPos = pos };
		::SetScrollInfo(hwnd, SB_CTL, &si, true);
		exedit.fp->func_WndProc(exedit.fp->hwnd, hv_message,
			(pos << 16) | SB_THUMBTRACK, reinterpret_cast<LPARAM>(hwnd),
			editp, exedit.fp);
	}
};
inline constexpr struct : TimelineScrollBar {
	constexpr static int
		scroll_raw = 1'000'000, // スクロール量の基準となる内部的な数値．
		margin_raw = 960'000; // 最終フレームより右側に表示される余白幅を決定する内部的な数値．
	static int get_margin() {
		if (auto coeff = *exedit.curr_timeline_scale_len; coeff > 0)
			return margin_raw / coeff;
		return -1;
	}

	static int get_pos() { return *exedit.timeline_h_scroll_pos; }
	static int get_page_size() { return *exedit.timeline_width_in_frames; }
} tl_scroll_h { true };
inline constexpr struct : TimelineScrollBar {
	static int get_pos() { return *exedit.timeline_v_scroll_pos; }
	static int get_page_size() { return *exedit.timeline_height_in_layers; }
} tl_scroll_v { false };


////////////////////////////////
// 設定項目．
////////////////////////////////
inline constinit struct Settings {
	constexpr static int config_rate_prec = 1000; // 設定でスクロール量などの比率を指定する最小単位の逆数．

	bool skip_inactive_objects = false;
	bool skip_hidden_layers = false;

	bool suppress_shift = true;

	int layer_count = 1;
	int seek_amount = tl_scroll_h.scroll_raw;
	int scroll_amount = tl_scroll_h.scroll_raw;

	void load(const char* ini_file)
	{
		auto read_raw = [&](auto def, const char* section, const char* key) {
			return static_cast<decltype(def)>(
				::GetPrivateProfileIntA(section, key, static_cast<int32_t>(def), ini_file));
		};
#define load_gen(tgt, section, read, write)	tgt = read(read_raw(write(tgt), section, #tgt))
#define load_int(tgt, section)		load_gen(tgt, section, /*id*/, /*id*/)
#define load_ratio(tgt, section)	load_gen(tgt, section, \
			[](auto y) { return y * (tl_scroll_h.scroll_raw / config_rate_prec); }, \
			[](auto x) { return x / (tl_scroll_h.scroll_raw / config_rate_prec); })
#define load_bool(tgt, section)		load_gen(tgt, section, \
			0 != , [](auto x) { return x ? 1 : 0; })

		load_bool(skip_inactive_objects, "skips");
		load_bool(skip_hidden_layers, "skips");

		load_ratio(seek_amount, "scroll");
		load_ratio(scroll_amount, "scroll");
		load_int(layer_count, "scroll");

		load_bool(suppress_shift, "keyboard");

#undef load_bool
#undef load_ratio
#undef load_int
#undef load_gen

		constexpr int max_factor = 16;
		seek_amount = std::clamp(seek_amount, tl_scroll_h.scroll_raw / max_factor, tl_scroll_h.scroll_raw * max_factor);
		scroll_amount = std::clamp(scroll_amount, tl_scroll_h.scroll_raw / max_factor, tl_scroll_h.scroll_raw * max_factor);

		constexpr int num_layers = 100;
		layer_count = std::clamp(layer_count, 1, num_layers - 1);
	}
} settings;


////////////////////////////////
// 設定ロード．
////////////////////////////////

// replacing a file extension when it's known.
template<class T, size_t len_max, size_t len_old, size_t len_new>
void replace_tail(T(&dst)[len_max], size_t len, const T(&tail_old)[len_old], const T(&tail_new)[len_new])
{
	if (len < len_old || len - len_old + len_new > len_max) return;
	std::memcpy(dst + len - len_old, tail_new, len_new * sizeof(T));
}
inline void load_settings(HMODULE h_dll)
{
	char path[MAX_PATH];
	replace_tail(path, ::GetModuleFileNameA(h_dll, path, std::size(path)) + 1, "auf", "ini");

	settings.load(path);
}


////////////////////////////////
// コマンドの定義．
////////////////////////////////

struct Menu {
	enum class ID : unsigned int {
		StepObjLeft = 0,
		StepObjLeftAll,
		StepObjRight,
		StepObjRightAll,

		StepMidptLeft,
		StepMidptLeftAll,
		StepMidptRight,
		StepMidptRightAll,

		StepLenLeft,
		StepLenRight,
		StepPageLeft,
		StepPageRight,

		StepIntoSelObj,
		StepIntoSelMidpt,
		StepIntoView,

		ScrollLeft,
		ScrollRight,
		ScrollPageLeft,
		ScrollPageRight,
		ScrollToCurrent,
		ScrollLeftMost,
		ScrollRightMost,
		ScrollUp,
		ScrollDown,

		SelectUpperObj,
		SelectLowerObj,

		Invalid,
	};
	using enum ID;
	static constexpr bool is_invalid(ID id) { return id >= Invalid; }
	static constexpr bool is_seek(ID id) { return id <= StepIntoView; }
	static constexpr bool is_scroll(ID id) { return id <= ScrollDown; }
	static constexpr bool is_select(ID id) { return id <= SelectLowerObj; }

	constexpr static struct { ID id; const char* name; } Items[] = {
		{ StepMidptLeft,	"左の中間点(レイヤー)" },
		{ StepMidptRight,	"右の中間点(レイヤー)" },
		{ StepObjLeft,		"左のオブジェクト境界(レイヤー)" },
		{ StepObjRight,		"右のオブジェクト境界(レイヤー)" },
		{ StepMidptLeftAll,	"左の中間点(シーン)" },
		{ StepMidptRightAll,"右の中間点(シーン)" },
		{ StepObjLeftAll,	"左のオブジェクト境界(シーン)" },
		{ StepObjRightAll,	"右のオブジェクト境界(シーン)" },
		{ StepIntoSelMidpt,	"現在位置を選択中間点区間に移動" },
		{ StepIntoSelObj,	"現在位置を選択オブジェクトに移動" },
		{ StepIntoView,		"現在位置をTL表示範囲内に移動" },
		{ StepLenLeft,		"左へ一定量移動" },
		{ StepLenRight,		"右へ一定量移動" },
		{ StepPageLeft,		"左へ1ページ分移動" },
		{ StepPageRight,	"右へ1ページ分移動" },
		{ ScrollLeft,		"TLスクロール(左)" },
		{ ScrollRight,		"TLスクロール(右)" },
		{ ScrollPageLeft,	"TLスクロール(左ページ)" },
		{ ScrollPageRight,	"TLスクロール(右ページ)" },
		{ ScrollToCurrent,	"TLスクロール(現在位置まで)" },
		{ ScrollLeftMost,	"TLスクロール(開始位置まで)" },
		{ ScrollRightMost,	"TLスクロール(終了位置まで)" },
		{ ScrollUp,			"TLスクロール(上)" },
		{ ScrollDown,		"TLスクロール(下)" },
		{ SelectUpperObj,	"現在フレームのオブジェクトへ移動(上)" },
		{ SelectLowerObj,	"現在フレームのオブジェクトへ移動(下)" },
	};

	// 拡張編集の「現在のオブジェクトを選択」コマンドの ID.
	constexpr static int exedit_command_select_obj = 1050;
};

////////////////////////////////
// 各種コマンド実行の実装．
////////////////////////////////

// タイムラインスクロール系の操作．
inline bool menu_scroll_handler(Menu::ID id, EditHandle* editp, FilterPlugin* fp)
{
	switch (id) {
		int dir;

		// horizontal scroll.
	case Menu::ScrollLeft: dir = -1; goto scroll_LR;
	case Menu::ScrollRight: dir = 1; goto scroll_LR;
	scroll_LR:
		if (int coeff = *exedit.curr_timeline_scale_len; coeff > 0) {
			dir *= settings.scroll_amount / coeff;
			tl_scroll_h.set_pos(tl_scroll_h.get_pos() + dir, editp);
		}
		break;

	case Menu::ScrollPageLeft: dir = -1; goto scroll_page_LR;
	case Menu::ScrollPageRight: dir = 1; goto scroll_page_LR;
	scroll_page_LR:
		dir *= tl_scroll_h.get_page_size();
		tl_scroll_h.set_pos(tl_scroll_h.get_pos() + dir, editp);
		break;

	case Menu::ScrollLeftMost: dir = -1; goto scroll_end_LR;
	case Menu::ScrollRightMost: dir = 1; goto scroll_end_LR;
	scroll_end_LR:
		tl_scroll_h.set_pos(dir < 0 ? 0 : fp->exfunc->get_frame_n(editp), editp);
		break;

	case Menu::ScrollToCurrent:
	{
		int curr_scr_pos = tl_scroll_h.get_pos();
		int page_size = tl_scroll_h.get_page_size();
		int margin = tl_scroll_h.get_margin();
		int pos = fp->exfunc->get_frame(editp);

		int moveto = curr_scr_pos;
		if (pos < curr_scr_pos + margin) moveto = pos - margin;
		else if (curr_scr_pos + page_size - margin < pos)
			moveto = pos - page_size + margin;
		if (moveto < 0) moveto = 0;
		else if (int max = fp->exfunc->get_frame_n(editp) - page_size + margin;
			moveto > max) moveto = max;

		if (moveto != curr_scr_pos)
			tl_scroll_h.set_pos(moveto, editp);
	}
		break;

		// vertical (layerwise) scroll.
	case Menu::ScrollUp: dir = -1; goto scroll_UD;
	case Menu::ScrollDown: dir = 1; goto scroll_UD;
	scroll_UD:
		dir *= settings.layer_count;
		tl_scroll_v.set_pos(tl_scroll_v.get_pos() + dir, editp);
		break;
	}

	// 出力画面更新の必要はないので false を返す．
	return false;
}

// 現在フレーム移動操作．
inline bool menu_seek_obj_handler(Menu::ID id, EditHandle* editp, FilterPlugin* fp)
{
	constexpr int num_layers = 100;
	const auto pos = fp->exfunc->get_frame(editp);
	const auto len = fp->exfunc->get_frame_n(editp);

	int moveto = pos;
	switch (id) {
		{
		bool skip_midpoints;
	case Menu::StepObjLeft: skip_midpoints = true; goto step_left;
	case Menu::StepMidptLeft: skip_midpoints = false; goto step_left;
	step_left:
		// 選択中のオブジェクトのあるレイヤー内の左の編集点へ移動．
		if (int sel_idx = *exedit.SettingDialogObjectIndex; sel_idx >= 0) {
			moveto = Timeline::find_adjacent_left(pos, (*exedit.ObjectArray_ptr)[sel_idx].layer_set,
				skip_midpoints, settings.skip_inactive_objects);
		}
		else goto step_left_all;
		break;

	case Menu::StepObjLeftAll: skip_midpoints = true; goto step_left_all;
	case Menu::StepMidptLeftAll: skip_midpoints = false; goto step_left_all;
	step_left_all:
	{
		// 全レイヤーを探して左の編集点へ移動．選択オブジェクトがない場合のフォールバックも兼任．
		moveto = 0;
		auto scene_layer_settings = exedit.LayerSettings + num_layers * (*exedit.current_scene);
		for (int j = 0; j < num_layers; j++) {
			if (settings.skip_hidden_layers && Timeline::is_hidden(scene_layer_settings[j])) continue;
			moveto = std::max(moveto,
				Timeline::find_adjacent_left(pos, j, skip_midpoints, settings.skip_inactive_objects));
		}
		break;
	}

		// 以上 2 つの左右入れ替え．
	case Menu::StepObjRight: skip_midpoints = true; goto step_right;
	case Menu::StepMidptRight: skip_midpoints = false; goto step_right;
	step_right:
		if (int sel_idx = *exedit.SettingDialogObjectIndex; sel_idx >= 0) {
			moveto = Timeline::find_adjacent_right(pos, (*exedit.ObjectArray_ptr)[sel_idx].layer_set,
				skip_midpoints, settings.skip_inactive_objects, len);
		}
		else goto step_right_all;
		break;

	case Menu::StepObjRightAll: skip_midpoints = true; goto step_right_all;
	case Menu::StepMidptRightAll: skip_midpoints = false; goto step_right_all;
	step_right_all:
	{
		moveto = len - 1;
		auto scene_layer_settings = exedit.LayerSettings + num_layers * (*exedit.current_scene);
		for (int j = 0; j < num_layers; j++) {
			if (settings.skip_hidden_layers && Timeline::is_hidden(scene_layer_settings[j])) continue;
			moveto = std::min(moveto,
				Timeline::find_adjacent_right(pos, j, skip_midpoints, settings.skip_inactive_objects, len));
		}
		break;
	}

#define chain_begin(obj)	(skip_midpoints ? Timeline::chain_begin(obj) : (obj)->frame_begin)
#define chain_end(obj)		(skip_midpoints ? Timeline::chain_end(obj) : (obj)->frame_end)
		// 現在選択中のオブジェクト or 中間点区間まで現在フレームを移動．
	case Menu::StepIntoSelObj: skip_midpoints = true; goto step_into_selected;
	case Menu::StepIntoSelMidpt: skip_midpoints = false; goto step_into_selected;
	step_into_selected:
		if (auto sel_idx = *exedit.SettingDialogObjectIndex; sel_idx >= 0) {
			const auto& sel_obj = (*exedit.ObjectArray_ptr)[sel_idx];
			if (auto begin = chain_begin(&sel_obj); pos < begin) moveto = begin;
			else if (auto end = chain_end(&sel_obj); end < pos) moveto = end;
		}
		break;
#undef chain_begin
#undef chain_end
		}

		{
		int dir;
		// タイムラインの拡大率に応じたフレーム数で，画面上一定長さだけ移動．
	case Menu::StepLenLeft: dir = -1; goto step_len_LR;
	case Menu::StepLenRight: dir = 1; goto step_len_LR;
	step_len_LR:
		if (int coeff = *exedit.curr_timeline_scale_len; coeff > 0) {
			dir *= settings.seek_amount / coeff;
			moveto = std::clamp(pos + dir, 0, len - 1);
		}
		break;

		// タイムラインの表示範囲分だけ移動．
	case Menu::StepPageLeft: dir = -1; goto step_page_LR;
	case Menu::StepPageRight: dir = 1; goto step_page_LR;
	step_page_LR:
		dir *= tl_scroll_h.get_page_size() - tl_scroll_h.get_margin();
		moveto = std::clamp(pos + dir, 0, len - 1);
		break;
		}

		// タイムラインの表示範囲内まで現在フレームを移動．
	case Menu::StepIntoView:
	{
		int curr_scr_pos = tl_scroll_h.get_pos();
		int page_size = tl_scroll_h.get_page_size();
		int margin = tl_scroll_h.get_margin();

		if (curr_scr_pos == 0 && pos <= margin); // do nothing.
		else if (2 * margin >= page_size)
			moveto = curr_scr_pos + page_size / 2;
		else if (pos < curr_scr_pos + margin)
			moveto = curr_scr_pos + margin;
		else if (pos > curr_scr_pos + page_size - margin)
			moveto = curr_scr_pos + page_size - margin;

		if (moveto < 0) moveto = 0;
		if (moveto >= len) moveto = len - 1;
		break;
	}
	}

	// 位置を特定できたので必要なら移動．
	if (pos == moveto) return false;

	// disable SHIFT key
	ForceKeyState shift(settings.suppress_shift ?
		VK_SHIFT : ForceKeyState::vkey_invalid, false);
	if (pos == fp->exfunc->set_frame(editp, moveto)) return false; // 実質移動なし．

	// 拡張編集のバグで，「カーソル移動時に自動でスクロール」を設定していても，
	// 「選択オブジェクトの追従」が有効でかつ移動先で新たなオブジェクトが選択された場合，
	// スクロールが追従しないため手動でスクロール．
	if (*exedit.scroll_follows_cursor != 0)
		menu_scroll_handler(Menu::ScrollToCurrent, editp, fp);

	// true を返して出力画面を更新させる．
	return true;
}

// オブジェクト選択．
inline bool menu_select_obj_handler(Menu::ID id, EditHandle* editp)
{
	bool shift;
	switch (id) {
	case Menu::SelectUpperObj: shift = true; break;
	case Menu::SelectLowerObj: shift = false; break;
	default: return false;
	}

	// fake shift key state.
	ForceKeyState k(VK_SHIFT, shift);

	// 拡張編集にコマンドメッセージ送信．
	return exedit.fp->func_WndProc(exedit.fp->hwnd, FilterPlugin::WindowMessage::Command,
		Menu::exedit_command_select_obj, 0, editp, exedit.fp) != FALSE;
}


////////////////////////////////
// AviUtlに渡す関数の定義．
////////////////////////////////
BOOL func_init(FilterPlugin* fp)
{
	// 情報源確保．
	if (!exedit.init(fp)) {
		::MessageBoxA(nullptr, "拡張編集0.92が見つかりませんでした．",
			fp->name, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	// message-only window を作成，登録．これで NoConfig でも AviUtl からメッセージを受け取れる.
	fp->hwnd = ::CreateWindowExW(0, L"AviUtl", L"", 0, 0, 0, 0, 0,
		HWND_MESSAGE, nullptr, fp->hinst_parent, nullptr);

	// メニュー登録．
	for (auto [id, name] : Menu::Items)
		fp->exfunc->add_menu_item(fp, name, fp->hwnd, static_cast<int32_t>(id), 0, ExFunc::AddMenuItemFlag::None);

	// 設定ロード．
	load_settings(fp->dll_hinst);

	return TRUE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, EditHandle* editp, FilterPlugin* fp)
{
	using Message = FilterPlugin::WindowMessage;
	switch (message) {
	case Message::Exit:
		// message-only window を削除．必要ないかもしれないけど．
		fp->hwnd = nullptr; ::DestroyWindow(hwnd);
		break;

		// コマンドハンドラ．
	case Message::Command:
		if (auto id = static_cast<Menu::ID>(wparam); !Menu::is_invalid(id) &&
			fp->exfunc->is_editing(editp) && !fp->exfunc->is_saving(editp)) {

			// 系統によって処理の毛色が違うので分岐．
			return (
				Menu::is_seek(id)   ? menu_seek_obj_handler(id, editp, fp) :
				Menu::is_scroll(id) ? menu_scroll_handler(id, editp, fp) :
				Menu::is_select(id) ? menu_select_obj_handler(id, editp) :
				false) ? TRUE : FALSE;
		}
		break;
	}
	return FALSE;
}


////////////////////////////////
// 看板．
////////////////////////////////
#define PLUGIN_NAME		"TLショトカ移動"
#define PLUGIN_VERSION	"v1.01-beta2"
#define PLUGIN_AUTHOR	"sigma-axis"
#define PLUGIN_INFO_FMT(name, ver, author)	(name##" "##ver##" by "##author)
#define PLUGIN_INFO		PLUGIN_INFO_FMT(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR)

extern "C" __declspec(dllexport) FilterPluginDLL* __stdcall GetFilterTable(void)
{
	// （フィルタとは名ばかりの）看板．
	using Flag = FilterPlugin::Flag;
	static constinit FilterPluginDLL filter {
		.flag = Flag::NoConfig | Flag::AlwaysActive | Flag::ExInformation,
		.name = PLUGIN_NAME,

		.func_init = func_init,
		.func_WndProc = func_WndProc,
		.information = PLUGIN_INFO,
	};
	return &filter;
}
