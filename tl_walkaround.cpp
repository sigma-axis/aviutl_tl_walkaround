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

using byte = uint8_t;
#include <exedit.hpp>

using namespace AviUtl;
using namespace ExEdit;

#include "key_states.hpp"
using namespace sigma_lib::W32::UI;

////////////////////////////////
// 主要情報源の変数アドレス．
////////////////////////////////
inline constinit struct ExEdit092 {
	FilterPlugin* fp;
	decltype(fp->func_WndProc) func_wndproc;
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
				func_wndproc = fp->func_WndProc;
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
	int32_t* curr_timeline_layer_height;	// 0x0a3e20
	int32_t* scroll_follows_cursor;			// 0x1790d0
	HWND*	 timeline_h_scroll_bar;			// 0x179108
	int32_t* timeline_h_scroll_pos;			// 0x1a52f0
	int32_t* timeline_width_in_frames;		// 0x1a52f8
	HWND*	 timeline_v_scroll_bar;			// 0x158d34
	int32_t* timeline_v_scroll_pos;			// 0x1a5308
	int32_t* timeline_height_in_layers;		// 0x0a3fbc

	int32_t* timeline_drag_kind;			// 0x177a24
		// 0: ドラッグなし．
		// 1: オブジェクト移動ドラッグ．
		// 2: オブジェクトの左端をつまんで移動．
		// 3: 中間点や右端をつまんで移動．
		// 4: 現在フレームを動かすドラッグ．
		// 5: ? 不明．
		// 6: Ctrl での複数選択ドラッグ．
		// 7: Alt でのスクロールドラッグ．
	int32_t* timeline_drag_edit_flag;		// 0x177a08, 0: clean, 1: dirty.
		// makes sense only for drag kinds 1, 2, 3 above.

	int32_t* timeline_BPM_tempo;			// 0x159190, multiplied by 10'000
	int32_t* timeline_BPM_frame_origin;		// 0x158d28, 1-based
	int32_t* timeline_BPM_num_beats;		// 0x178e30
	int32_t* timeline_BPM_show_grid;		// 0x196760, 0: hidden, 1: visible

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
		pick_addr(curr_timeline_layer_height,	0x0a3e20);
		pick_addr(scroll_follows_cursor,		0x1790d0);
		pick_addr(timeline_h_scroll_bar,		0x179108);
		pick_addr(timeline_h_scroll_pos,		0x1a52f0);
		pick_addr(timeline_width_in_frames,		0x1a52f8);
		pick_addr(timeline_v_scroll_bar,		0x158d34);
		pick_addr(timeline_v_scroll_pos,		0x1a5308);
		pick_addr(timeline_height_in_layers,	0x0a3fbc);

		pick_addr(timeline_drag_kind,			0x177a24);
		pick_addr(timeline_drag_edit_flag,		0x177a08);

		pick_addr(timeline_BPM_tempo,			0x159190);
		pick_addr(timeline_BPM_frame_origin,	0x158d28);
		pick_addr(timeline_BPM_num_beats,		0x178e30);
		pick_addr(timeline_BPM_show_grid,		0x196760);
	}
} exedit;


////////////////////////////////
// 探索関数コア部．
////////////////////////////////
class Timeline {
	// returns the index of the right-most object whose beginning point is at `pos` or less,
	// `idx_L-1` if couldn't find such.
	static int find_nearest_index(int pos, int idx_L, int idx_R)
	{
		if (idx_L > idx_R) return idx_L - 1; // 空レイヤー．

		// まず両端のオブジェクトを見る．
		if (exedit.SortedObject[idx_L]->frame_begin > pos) return idx_L - 1;
		if (exedit.SortedObject[idx_R]->frame_begin <= pos) idx_L = idx_R;

		// あとは２分法．
		while (idx_L + 1 < idx_R) {
			auto idx = (idx_L + idx_R) >> 1;
			if (exedit.SortedObject[idx]->frame_begin <= pos) idx_L = idx; else idx_R = idx;
		}
		return idx_L;
	}

public:
	static int find_adjacent_left(int pos, int layer, bool skip_midpoints, bool skip_inactives)
	{
		// 指定された位置より左にある最も近い境界を探す．
		int idx_L = exedit.SortedObjectLayerBeginIndex[layer],
			idx_R = exedit.SortedObjectLayerEndIndex[layer];

		// pos より左側に開始点のあるオブジェクトの index を取得．
		int idx = find_nearest_index(pos - 1, idx_L, idx_R);
		if (idx < idx_L) return 0; // 空レイヤー or 左側に何もない．

		// そこを起点に線形探索してスキップ対象を除外．
		return find_adjacent_left_core(idx, idx_L, pos - 1, skip_midpoints, skip_inactives);
	}

	// the return value of -1 stands for the end of the scene.
	static int find_adjacent_right(int pos, int layer, bool skip_midpoints, bool skip_inactives)
	{
		// 上の関数の左右逆版．
		int idx_L = exedit.SortedObjectLayerBeginIndex[layer],
			idx_R = exedit.SortedObjectLayerEndIndex[layer];

		// pos の位置かそれより左側に開始点のあるオブジェクトを取得．
		int idx = find_nearest_index(pos, idx_L, idx_R);

		// pos より右側に終了点のあるオブジェクトの index に修正．
		if (idx < idx_L) idx = idx_L;
		else if (exedit.SortedObject[idx]->frame_end + 1 <= pos) idx++;
		if (idx > idx_R) return -1; // 最右端．

		// そこを起点に線形探索してスキップ対象を除外．
		return find_adjacent_right_core(idx, idx_R, pos + 1, skip_midpoints, skip_inactives);
	}
	// 「-1 で最右端」規格がめんどくさいのでラップ．
	static int find_adjacent_right(int pos, int layer,
		bool skip_midpoints, bool skip_inactives, int len) {
		pos = find_adjacent_right(pos, layer, skip_midpoints, skip_inactives);
		return 0 <= pos && pos < len ? pos : len - 1;
	}

	static std::tuple<int, int> find_interval(int pos, int layer, bool skip_midpoints, bool skip_inactives)
	{
		int idx_L = exedit.SortedObjectLayerBeginIndex[layer],
			idx_R = exedit.SortedObjectLayerEndIndex[layer];

		// pos の位置かそれより左側に開始点のあるオブジェクトを取得．
		int idx = find_nearest_index(pos, idx_L, idx_R);

		// まずは左端．
		int pos_l = idx < idx_L ? 0 :
			find_adjacent_left_core(idx, idx_L, pos, skip_midpoints, skip_inactives);

		// そして右端．
		// pos より右側に終了点のあるオブジェクトの index に修正．
		if (idx < idx_L) idx = idx_L;
		else if (exedit.SortedObject[idx]->frame_end + 1 <= pos) idx++;
		int pos_r = idx > idx_R ? -1 :
			find_adjacent_right_core(idx, idx_R, pos + 1, skip_midpoints, skip_inactives);

		return { pos_l, pos_r };
	}

private:
#define ToSkip(obj)		(skip_inactives && !is_active(obj))
#define ChainBegin(obj)	(skip_midpoints ? chain_begin(obj) : (obj)->frame_begin)
#define ChainEnd(obj)	(skip_midpoints ? chain_end(obj) : (obj)->frame_end)
	// idx 以下のオブジェクトの境界や中間点で，条件に当てはまる pos 以下の点を線形に検索．
	static int find_adjacent_left_core(int idx, int idx_L, int pos, bool skip_midpoints, bool skip_inactives)
	{
		if (auto obj = exedit.SortedObject[idx]; !ToSkip(obj))
			return pos < obj->frame_end + 1 ? ChainBegin(obj) : ChainEnd(obj) + 1;

		// 無効でない最も近いオブジェクトを探すのは線形探索しか方法がない．
		while (idx_L <= --idx) {
			// 初期 idx が無効オブジェクトでスキップされたという前提があるため，
			// 最初に見つかった有効オブジェクトの終了点は中間点でなくオブジェクト境界になる．
			// なので skip_midpoints の確認や ChainEnd() 呼び出しは必要ない．
			if (auto obj = exedit.SortedObject[idx]; is_active(obj))
				return obj->frame_end + 1;
		}
		return 0;
	}
	// idx 以上のオブジェクトの境界や中間点で，条件に当てはまる pos 以上の点を線形に検索．
	static int find_adjacent_right_core(int idx, int idx_R, int pos, bool skip_midpoints, bool skip_inactives)
	{
		// 上の関数の左右逆．
		if (auto obj = exedit.SortedObject[idx]; !ToSkip(obj))
			return pos <= obj->frame_begin ? obj->frame_begin : ChainEnd(obj) + 1;

		while (++idx <= idx_R) {
			if (auto obj = exedit.SortedObject[idx]; is_active(obj))
				return obj->frame_begin;
		}
		return -1;
	}
#undef ToSkip
#undef ChainBegin
#undef ChainEnd

public:
	static bool is_active(const Object* obj) {
		if (auto i = obj->index_midpt_leader; i >= 0) obj = &(*exedit.ObjectArray_ptr)[i];
		return has_flag_or(obj->filter_status[0], Object::FilterStatus::Active);
	}
	static bool is_hidden(const LayerSetting& setting) {
		return has_flag_or(setting.flag, LayerSetting::Flag::UnDisp);
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

	// 座標変換．
private:
	// division that rounds toward negative infinity.
	// `divisor` is assumed to be positive.
	static constexpr auto floor_div(auto dividend, auto const divisor) {
		if (dividend < 0) dividend -= divisor - 1;
		return dividend / divisor;
	}

	// タイムラインズームサイズの分母．
	constexpr static int scale_denom = 10'000;
public:
	// タイムラインの最上段レイヤーの左上座標．
	constexpr static int x_leftmost_client = 64, y_topmost_client = 42;
	static inline int PointToFrame(int x)
	{
		x -= x_leftmost_client;
		x *= scale_denom;
		x = floor_div(x, *exedit.curr_timeline_scale_len);
		x += *exedit.timeline_h_scroll_pos;
		if (x < 0) x = 0;

		return x;
	}
	static inline int FrameToPoint(int f)
	{
		f -= *exedit.timeline_h_scroll_pos;
		{
			// possibly overflow; curr_timeline_scale_len is at most 100'000 (> 2^16).
			int64_t F = f;
			F *= *exedit.curr_timeline_scale_len;
			F = floor_div(F, scale_denom);
			f = static_cast<int32_t>(F);
		}
		f += x_leftmost_client;

		return f;
	}
	// シーンによらず最上段レイヤーは 0 扱い．
	static int PointToLayer(int y)
	{
		y -= y_topmost_client;
		y = floor_div(y, *exedit.curr_timeline_layer_height);
		y += *exedit.timeline_v_scroll_pos;
		y = std::clamp(y, 0, 99);

		return y;
	}
};

// BPM グリッドの丸め計算．
struct BPM_Grid {
	BPM_Grid(int beats_numer, int beats_denom, EditHandle* editp)
		: BPM_Grid(*exedit.timeline_BPM_frame_origin - 1, beats_numer, beats_denom, editp) {}
	BPM_Grid(int origin, int beats_numer, int beats_denom, EditHandle* editp)
		: origin{ origin } {
		// find the frame rate.
		FileInfo fi; // the frame rate is calculated as: fi.video_rate / fi.video_scale.
		exedit.fp->exfunc->get_file_info(editp, &fi);

		// find the tempo in BPM.
		auto tempo = *exedit.timeline_BPM_tempo;

		// calculate "frames per beat".
		if (fi.video_rate > 0 && fi.video_scale > 0 && tempo > 0) {
			fpb_n = static_cast<int64_t>(fi.video_rate) * (600'000 * beats_numer);
			fpb_d = static_cast<int64_t>(fi.video_scale) * (tempo * beats_denom);
		}
		else fpb_n = fpb_d = 0;
	}

	// check if the retrieved parameters are valid.
	constexpr operator bool() const { return fpb_n > 0 && fpb_d > 0; }

	// フレーム位置から拍数を計算．
	// フレーム位置 pos 上かそれより左にある拍数線のうち，最も大きい拍数を返す．
	// (1 フレームに複数の拍数線が重なっている状態でも，そのうち最も大きい拍数が対象．)
	constexpr auto beat_from_pos(int32_t pos) const {
		// frame -> beat is by rounding toward zero.
		pos -= origin;
		// handle negative positions differently.
		auto beats = (pos < 0 ? -1 - pos : pos) * fpb_d / fpb_n;
		return pos < 0 ? -1 - beats : beats;
	}

	// 拍数からフレーム位置を計算．
	// 拍数 beat で描画される拍数線のフレーム位置を返す．
	/*constexpr */auto pos_from_beat(int64_t beat) const {
		// beat -> frame is by rounding away from zero.
		auto d = std::div(beat * fpb_n, fpb_d); // std::div() isn't constexpr at this time.
		if (d.rem > 0) d.quot++; else if (d.rem < 0) d.quot--;
		return static_cast<int32_t>(d.quot) + origin;
	}

	// the origin of the BPM grid.
	int32_t origin;

private:
	// "frames per beat" represented as a rational number.
	int64_t fpb_n, fpb_d;
};


////////////////////////////////
// Windows API 利用の補助関数．
////////////////////////////////

// タイムラインのスクロールバー操作．
class TimelineScrollBar {
	HWND* const& phwnd;
	int32_t* const& pos;
	int32_t* const& page;
	const uint32_t hv_message;

public:
	consteval TimelineScrollBar(bool horizontal)
		: phwnd{ horizontal ? exedit.timeline_h_scroll_bar : exedit.timeline_v_scroll_bar }
		, pos{ horizontal ? exedit.timeline_h_scroll_pos : exedit.timeline_v_scroll_pos }
		, page{ horizontal ? exedit.timeline_width_in_frames : exedit.timeline_height_in_layers }
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
	int get_pos() const { return *pos; }
	int get_page_size() const { return *page; }
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
} tl_scroll_h { true };
inline constexpr TimelineScrollBar tl_scroll_v { false };


////////////////////////////////
// 設定項目．
////////////////////////////////
inline constinit struct Settings {
	constexpr static int config_rate_prec = 1000; // 設定でスクロール量などの比率を指定する最小単位の逆数．

	struct Skips {
		bool skip_inactive_objects = false;
		bool skip_hidden_layers = false;
	} skips;

	struct Keyboard {
		bool suppress_shift = true;
	} keyboard;

	struct BPMGrid {
		uint8_t nth_beat = 3;
		constexpr static uint8_t nth_beat_min = 2, nth_beat_max = 128;
	} bpm_grid;

	struct Scroll {
		int layer_count = 1;
		int seek_amount = tl_scroll_h.scroll_raw;
		int scroll_amount = tl_scroll_h.scroll_raw;

	private:
		constexpr static int seek_factor = 16, scroll_factor = 16;
	public:
		constexpr static int
			layer_count_min = 1, layer_count_max = 99,
			seek_amount_min = tl_scroll_h.scroll_raw / seek_factor,
			seek_amount_max = tl_scroll_h.scroll_raw * seek_factor,
			scroll_amount_min = tl_scroll_h.scroll_raw / scroll_factor,
			scroll_amount_max = tl_scroll_h.scroll_raw * scroll_factor;
	} scroll;

	enum class DragButton : uint8_t {
		none		= 0,
		// left can't be assigned.
		shift_right	= 1,
		wheel		= 2,
		x1			= 3,
		x2			= 4,
		left_right	= 5,
		left_wheel	= 6,
		left_x1		= 7,
		left_x2		= 8,
	};
	enum class ModKey : uint8_t {
		off,
		on,
		ctrl,
		inv_ctrl,
		shift,
		inv_shift,
		// no alt; may conflict with InputPipePlugin.
	};
	struct Mouse {
		DragButton obj_button = DragButton::none;
		ModKey obj_condition = ModKey::on,
			obj_skip_midpoints = ModKey::ctrl,
			obj_skip_inactives = ModKey::off;

		DragButton bpm_button = DragButton::none;
		ModKey bpm_condition = ModKey::on,
			bpm_stop_beat = ModKey::inv_ctrl,
			bpm_stop_4th_beat = ModKey::shift,
			bpm_stop_nth_beat = ModKey::off;

		uint8_t snap_range = 20;
		constexpr static uint8_t snap_range_min = 2, snap_range_max = 128;
		bool suppress_shift = true;

		DragButton scr_button = DragButton::none;
		ModKey scr_condition = ModKey::on;
	} mouse;

	void load(const char* ini_file)
	{
		auto read_raw = [&](auto def, const char* section, const char* key) {
			return static_cast<decltype(def)>(
				::GetPrivateProfileIntA(section, key, static_cast<int32_t>(def), ini_file));
		};
		auto read_modkey = [&](const char* section, const char* key) {
			char buf[32];
			::GetPrivateProfileStringA(section, key, "0", buf, std::size(buf), ini_file);

			// doesn't check the word entirely. if the first character matches, it's enough.
			using enum ModKey;
			bool inv = false;
			const char* p = &buf[0];
			switch (*p) {
			case '0': return off;
			case '1': return on;
			case '!': inv = true; p++; break;
			}
			switch (*p) {
			case 's': case 'S': return inv ? inv_shift : shift;
			case 'c': case 'C': return inv ? inv_ctrl : ctrl;
			default: return off;
			}
		};
	#define load_gen(tgt, section, read, write)	section.tgt = read(read_raw(write(section.tgt), #section, #tgt))
	#define load_int(tgt, section)		load_gen(tgt, section,\
		[&](auto y) { return std::clamp(y, section.tgt##_min, section.tgt##_max); }, /*id*/)
	#define load_enum(tgt, section)		load_gen(tgt, section, /*id*/, /*id*/)
	#define load_mkey(tgt, section)		section.tgt = read_modkey(#section, #tgt)
	#define load_ratio(tgt, section)	load_gen(tgt, section, \
			[](auto y) { return y * (tl_scroll_h.scroll_raw / config_rate_prec); }, \
			[](auto x) { return x / (tl_scroll_h.scroll_raw / config_rate_prec); })
	#define load_bool(tgt, section)		load_gen(tgt, section, \
			0 != , [](auto x) { return x ? 1 : 0; })

		load_bool	(skip_inactive_objects,	skips);
		load_bool	(skip_hidden_layers,	skips);

		load_ratio	(seek_amount,			scroll);
		load_ratio	(scroll_amount,			scroll);
		load_int	(layer_count,			scroll);

		load_bool	(suppress_shift,		keyboard);

		load_int	(nth_beat,				bpm_grid);

		load_enum	(obj_button,			mouse);
		load_mkey	(obj_condition,			mouse);
		load_mkey	(obj_skip_midpoints,	mouse);
		load_mkey	(obj_skip_inactives,	mouse);

		load_enum	(bpm_button,			mouse);
		load_mkey	(bpm_condition,			mouse);
		load_mkey	(bpm_stop_beat,			mouse);
		load_mkey	(bpm_stop_4th_beat,		mouse);
		load_mkey	(bpm_stop_nth_beat,		mouse);

		load_enum	(scr_button,			mouse);
		load_mkey	(scr_condition,			mouse);

		load_int	(snap_range,			mouse);
		load_bool	(suppress_shift,		mouse);

	#undef load_bool
	#undef load_ratio
	#undef load_int
	#undef load_gen
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
// マウス操作の追加．
////////////////////////////////
class Drag {
	// WM_*BUTTONDOWN.
	static inline constinit UINT obj_mes_down = WM_NULL, bpm_mes_down = WM_NULL, scr_mes_down = WM_NULL;
	// flags in wparam that are either required or rejected.
	static inline constinit uint32_t obj_msk_flags = 0, bpm_msk_flags = 0, scr_msk_flags = 0;
	// flags in wparam required to be one.
	static inline constinit uint32_t obj_req_flags = ~0, bpm_req_flags = ~0, scr_req_flags = ~0;
	static inline uint_fast8_t drag_state = 0; // 0: normal, 1: obj-wise drag, 2: bpm-wise drag, 3: scroll drag.

	constexpr static bool key2flag(Settings::ModKey k, bool key_ctrl, bool key_shift) {
		switch (k) {
			using enum Settings::ModKey;
		case on:		return true;
		case shift:		return key_shift;
		case inv_shift:	return !key_shift;
		case ctrl:		return key_ctrl;
		case inv_ctrl:	return !key_ctrl;
		case off: default: return false;
		}
	}
	static bool ObjSeek(int mouse_x, int mouse_y, bool ctrl, bool shift,
		EditHandle* editp, FilterPlugin* fp)
	{
		const auto pos = fp->exfunc->get_frame(editp);
		const auto len = fp->exfunc->get_frame_n(editp);

		// calculate the frame number and layer at the mouse position.
		auto frame_mouse = Timeline::PointToFrame(mouse_x);
		auto layer_mouse = Timeline::PointToLayer(mouse_y);

		// scroll if the mouse is outside the window.
		scroll_vertically_on_drag(layer_mouse, editp);

		// limit the target layer to a visible one.
		{
			auto layer_top = tl_scroll_v.get_pos();
			layer_mouse = std::clamp(layer_mouse, layer_top, layer_top + tl_scroll_v.get_page_size() - 1);
		}

		// calculate the smallest interval containing `frame_mouse`.
		auto [l, r] = Timeline::find_interval(frame_mouse, layer_mouse,
			key2flag(settings.mouse.obj_skip_midpoints, ctrl, shift),
			key2flag(settings.mouse.obj_skip_inactives, ctrl, shift));
		if (r < 0) r = len - 1;

		// then seek to either left or right if it's near enough to the mouse.
		return SeekLeftRight(l, r, mouse_x, pos, len, editp, fp);
	}
	// scrolls the timeline vertically when dragging over the visible area.
	// there are at least 100 ms intervals between scrolls.
	static void scroll_vertically_on_drag(int layer_mouse, EditHandle* editp) {
		constexpr uint32_t interval_min = 100;

		constexpr auto check_tick = [] {
		#pragma warning(suppress : 28159) // 32 bit is enough.
			uint32_t curr = ::GetTickCount();

			static constinit uint32_t prev = 0;
			if (curr - prev >= interval_min) {
				prev = curr;
				return true;
			}
			return false;
		};

		int dir = 0;
		if (auto rel_pos = layer_mouse - tl_scroll_v.get_pos();
			rel_pos < 0) dir = -1;
		else if (rel_pos >= tl_scroll_v.get_page_size()) dir = +1;
		else return;
		if (!check_tick()) return;

		// then scroll.
		tl_scroll_v.set_pos(tl_scroll_v.get_pos() + dir, editp);
	}
	static bool BPMSeek(int mouse_x, int mouse_y, bool ctrl, bool shift,
		EditHandle* editp, FilterPlugin* fp)
	{
		const auto pos = fp->exfunc->get_frame(editp);
		const auto len = fp->exfunc->get_frame_n(editp);

		auto frame_mouse = Timeline::PointToFrame(mouse_x);

		// find which `strength` of beat to stop.
		// priority: nth > 4th > one beat > per measure.
		int numer, denom;
		if (key2flag(settings.mouse.bpm_stop_nth_beat, ctrl, shift))
			numer = 1, denom = settings.bpm_grid.nth_beat;
		else if (key2flag(settings.mouse.bpm_stop_4th_beat, ctrl, shift))
			numer = 1, denom = 4;
		else if (key2flag(settings.mouse.bpm_stop_beat, ctrl, shift))
			numer = 1, denom = 1;
		else numer = *exedit.timeline_BPM_num_beats, denom = 1;

		// prepare BPM_Grid struct.
		BPM_Grid grid{ numer, denom, editp };

		// calculate the nearest left beat.
		auto beat = grid.beat_from_pos(frame_mouse);

		// seek to either `beat` or `beat + 1`.
		return SeekLeftRight(grid.pos_from_beat(beat), grid.pos_from_beat(beat + 1),
			mouse_x, pos, len, editp, fp);
	}
	static bool SeekLeftRight(int l, int r, int mouse_x, int pos, int len,
		EditHandle* editp, FilterPlugin* fp)
	{
		l = std::clamp(l, 0, len - 1); r = std::clamp(r, 0, len - 1);

		// choose the nearer frame in the screen coordinate.
		int moveto, moveto_x;
		auto l_x = Timeline::FrameToPoint(l), r_x = Timeline::FrameToPoint(r);
		if (2 * mouse_x <= l_x + r_x) moveto = l, moveto_x = l_x;
		else moveto = r, moveto_x = r_x;

		if (moveto == pos) return false; // didn't move.
		// if too far from mouse compared to the snap_range, do nothing.
		if ((settings.mouse.snap_range < settings.mouse.snap_range_max &&
			std::abs(moveto_x - mouse_x) > settings.mouse.snap_range)) return false;

		// Then, move to the desired destination, forcing Shift key state if necessary.
		ForceKeyState shift{ settings.mouse.suppress_shift ? VK_SHIFT : 0, false };
		fp->exfunc->set_frame(editp, moveto);
		return true;
	}
	static bool TLScroll(int mouse_x, int mouse_y, bool ctrl, bool shift,
		EditHandle* editp, FilterPlugin* fp)
	{
		return exedit.func_wndproc(exedit.fp->hwnd, WM_MOUSEMOVE,
			MK_LBUTTON, (mouse_y << 16) | (mouse_x & 0xffff), editp, fp) != FALSE;
	}
	static void exit_drag() {
		if (drag_state == 3) *exedit.timeline_drag_kind = 0;
		drag_state = 0;
		if (::GetCapture() == exedit.fp->hwnd)
			::ReleaseCapture();
	}

	static BOOL func_wndproc_detour(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
		EditHandle* editp, FilterPlugin* fp)
	{
		auto do_seek = [&](int mouse_x, int mouse_y) {
			return (
				drag_state == 1 ? ObjSeek :
				drag_state == 2 ? BPMSeek : TLScroll)(
				mouse_x, mouse_y, (wparam & MK_CONTROL) != 0, (wparam & MK_SHIFT) != 0, editp, fp);
		};
		if (drag_state != 0) {
			if (!fp->exfunc->is_editing(editp) || fp->exfunc->is_saving(editp)) {
				exit_drag();
				goto default_handler;
			}

			switch (message) {
			case WM_MOUSEMOVE:
				return do_seek(static_cast<int16_t>(lparam), static_cast<int16_t>(lparam >> 16)) ? TRUE : FALSE;

			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_XBUTTONUP:
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_XBUTTONDOWN:
				// any mouse button would stop this drag.
			case WM_CAPTURECHANGED: // drag aborted.
			end_this_drag:
				exit_drag();
				return FALSE;

			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				if (wparam == VK_ESCAPE)
					// ESC key should stop dragging too.
					goto end_this_drag;
				[[fallthrough]];
			case WM_KEYUP:
			case WM_SYSKEYUP:
				// ignore these two keys, which prevents the mouse cursor from changing.
				if (wparam == VK_CONTROL || wparam == VK_SHIFT) return FALSE;

				[[fallthrough]];
			default:
				// otherwise let it be handled normally.
				goto default_handler;
			}
		}
		else {
			decltype(drag_state) state_cand;

			// check for the message condition to initiate drag operation.
			if (message == obj_mes_down && (wparam & obj_msk_flags) == obj_req_flags) state_cand = 1;
			else if (message == bpm_mes_down && (wparam & bpm_msk_flags) == bpm_req_flags) state_cand = 2;
			else if (message == scr_mes_down && (wparam & scr_msk_flags) == scr_req_flags) state_cand = 3;
			else goto default_handler;

			// one more thing that user must be clicking inside the timeline region.
			int mouse_x = static_cast<int16_t>(lparam),
				mouse_y = static_cast<int16_t>(lparam >> 16);
			if (Timeline::x_leftmost_client > mouse_x || Timeline::y_topmost_client > mouse_y)
				goto default_handler;

			// exedit must not be in a dragging state, or at least yet-to-edit.
			auto drag_kind = *exedit.timeline_drag_kind;
			switch (drag_kind) {
			case 1: case 2: case 3:
				if (*exedit.timeline_drag_edit_flag != 0) goto default_handler;
				break;
			}

			// additionally check for the current state of AviUtl.
			if (!fp->exfunc->is_editing(editp) || fp->exfunc->is_saving(editp) ||
				::GetCapture() != (drag_kind == 0 ? nullptr : hwnd)) goto default_handler;

			// exit the existing drag before starting new one.
			if (drag_kind == 6) {
				// releasing ctrl key should deselect.
				ForceKeyState ctrl{ VK_CONTROL, false };
				exedit.func_wndproc(hwnd, WM_KEYUP, VK_CONTROL, 0xc000'0001, editp, fp);
			}
			else if (drag_kind != 0) exedit.func_wndproc(hwnd, WM_LBUTTONUP,
				wparam & ~(MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2 | ((XBUTTON1 | XBUTTON2) << 16) | MK_CONTROL),
				lparam, editp, fp);

			// turn into the dragging state, overriding mouse behavior.
			drag_state = state_cand;
			if (state_cand == 3) {
				// scroll drag uses built-in behavior of exedit.
				*exedit.timeline_drag_kind = 0;
				ForceKeyState alt{ VK_MENU, true };
				return exedit.func_wndproc(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lparam, editp, fp);
			}
			::SetCapture(hwnd);
			::SetCursor(::LoadCursorW(nullptr, reinterpret_cast<PCWSTR>(IDC_ARROW)));
			return do_seek(mouse_x, mouse_y) ? TRUE : FALSE;
		}

	default_handler:
		return exedit.func_wndproc(hwnd, message, wparam, lparam, editp, fp);
	}

public:
	static void init()
	{
		// pre-calculate the fast check condition to initiate drag operations.
		constexpr auto mes_flags = [](Settings::DragButton button, Settings::ModKey key) -> std::tuple<UINT, uint32_t, uint32_t> {
			constexpr auto disabled = std::make_tuple(WM_NULL, 0, ~0);

			// flags for modifier keys.
			uint32_t req = 0, rej = 0;
			switch (key) {
				using enum Settings::ModKey;
			case off:		return disabled;
			case ctrl:		req = MK_CONTROL;	break;
			case inv_ctrl:	rej = MK_CONTROL;	break;
			case shift:		req = MK_SHIFT;		break;
			case inv_shift:	rej = MK_SHIFT;		break;
			case on: default: break;
			}

			// flags for mouse buttons (and perhaps shift).
			UINT mes; uint32_t btn;
			switch (button) {
				using enum Settings::DragButton;
			case shift_right:	mes = WM_RBUTTONDOWN; btn = MK_RBUTTON | MK_SHIFT;							break;
			case wheel:			mes = WM_MBUTTONDOWN; btn = MK_MBUTTON;										break;
			case x1:			mes = WM_XBUTTONDOWN; btn = MK_XBUTTON1 | (XBUTTON1 << 16);					break;
			case x2:			mes = WM_XBUTTONDOWN; btn = MK_XBUTTON2 | (XBUTTON2 << 16);					break;
			case left_right:	mes = WM_RBUTTONDOWN; btn = MK_LBUTTON | MK_RBUTTON;						break;
			case left_wheel:	mes = WM_MBUTTONDOWN; btn = MK_LBUTTON | MK_MBUTTON;						break;
			case left_x1:		mes = WM_XBUTTONDOWN; btn = MK_LBUTTON | MK_XBUTTON1 | (XBUTTON1 << 16);	break;
			case left_x2:		mes = WM_XBUTTONDOWN; btn = MK_LBUTTON | MK_XBUTTON2 | (XBUTTON2 << 16);	break;
			case none: default: return disabled;
			}

			// combine.
			req |= btn;
			rej |= ~(MK_CONTROL | MK_SHIFT | btn);

			// check overlaps.
			if ((req & rej) != 0) return disabled;
			return { mes, req | rej, req};
		};

		std::tie(obj_mes_down, obj_msk_flags, obj_req_flags) = mes_flags(settings.mouse.obj_button, settings.mouse.obj_condition);
		std::tie(bpm_mes_down, bpm_msk_flags, bpm_req_flags) = mes_flags(settings.mouse.bpm_button, settings.mouse.bpm_condition);
		std::tie(scr_mes_down, scr_msk_flags, scr_req_flags) = mes_flags(settings.mouse.scr_button, settings.mouse.scr_condition);

		if (obj_mes_down != WM_NULL || bpm_mes_down != WM_NULL || scr_mes_down != WM_NULL)
			// as this feature is enabled, replace the callback function.
			exedit.fp->func_WndProc = &func_wndproc_detour;
	}

	static void force_cancel() {
		if (drag_state == 0) return;
		exit_drag();
	}
};


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

		StepMeasureBPMLeft,
		StepMeasureBPMRight,
		StepBeatBPMLeft,
		StepBeatBPMRight,
		StepQuarterBPMLeft,
		StepQuarterBPMRight,
		StepNthBPMLeft,
		StepNthBPMRight,

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

		BPMMoveOriginLeft,
		BPMMoveOriginRight,
		BPMMoveOriginCurrent,
		BPMFitBarToCurrent,

		Invalid,
	};
	using enum ID;
	static constexpr bool is_invalid(ID id) { return id >= Invalid; }
	static constexpr bool is_seek(ID id) { return id <= StepIntoView; }
	static constexpr bool is_scroll(ID id) { return id <= ScrollDown; }
	static constexpr bool is_select(ID id) { return id <= SelectLowerObj; }
	static constexpr bool is_misc(ID id) { return id <= BPMFitBarToCurrent; }

	constexpr static struct { ID id; const char* name; } Items[] = {
		{ StepMidptLeft,		"左の中間点(レイヤー)" },
		{ StepMidptRight,		"右の中間点(レイヤー)" },
		{ StepObjLeft,			"左のオブジェクト境界(レイヤー)" },
		{ StepObjRight,			"右のオブジェクト境界(レイヤー)" },
		{ StepMidptLeftAll,		"左の中間点(シーン)" },
		{ StepMidptRightAll,	"右の中間点(シーン)" },
		{ StepObjLeftAll,		"左のオブジェクト境界(シーン)" },
		{ StepObjRightAll,		"右のオブジェクト境界(シーン)" },
		{ StepIntoSelMidpt,		"現在位置を選択中間点区間に移動" },
		{ StepIntoSelObj,		"現在位置を選択オブジェクトに移動" },
		{ StepIntoView,			"現在位置をTL表示範囲内に移動" },
		{ StepLenLeft,			"左へ一定量移動" },
		{ StepLenRight,			"右へ一定量移動" },
		{ StepPageLeft,			"左へ1ページ分移動" },
		{ StepPageRight,		"右へ1ページ分移動" },
		{ StepMeasureBPMLeft,	"左の小節線に移動(BPM)" },
		{ StepMeasureBPMRight,	"右の小節線に移動(BPM)" },
		{ StepBeatBPMLeft,		"左の拍数位置に移動(BPM)" },
		{ StepBeatBPMRight,		"右の拍数位置に移動(BPM)" },
		{ StepQuarterBPMLeft,	"左の1/4拍位置に移動(BPM)" },
		{ StepQuarterBPMRight,	"右の1/4拍位置に移動(BPM)" },
		{ StepNthBPMLeft,		"左の1/N拍位置に移動(BPM)" },
		{ StepNthBPMRight,		"右の1/N拍位置に移動(BPM)" },
		{ BPMMoveOriginLeft,	"グリッドを左に移動(BPM)" },
		{ BPMMoveOriginRight,	"グリッドを右に移動(BPM)" },
		{ BPMFitBarToCurrent,	"最寄りの小節線を現在位置に(BPM)" },
		{ BPMMoveOriginCurrent,	"基準フレームを現在位置に(BPM)" },
		{ ScrollLeft,			"TLスクロール(左)" },
		{ ScrollRight,			"TLスクロール(右)" },
		{ ScrollPageLeft,		"TLスクロール(左ページ)" },
		{ ScrollPageRight,		"TLスクロール(右ページ)" },
		{ ScrollToCurrent,		"TLスクロール(現在位置まで)" },
		{ ScrollLeftMost,		"TLスクロール(開始位置まで)" },
		{ ScrollRightMost,		"TLスクロール(終了位置まで)" },
		{ ScrollUp,				"TLスクロール(上)" },
		{ ScrollDown,			"TLスクロール(下)" },
		{ SelectUpperObj,		"現在フレームのオブジェクトへ移動(上)" },
		{ SelectLowerObj,		"現在フレームのオブジェクトへ移動(下)" },
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
			dir *= settings.scroll.scroll_amount / coeff;
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

		int moveto = page_size < 2 * margin ? pos - (page_size / 2) :
			std::clamp(curr_scr_pos, pos - page_size + margin, pos - margin);

		moveto = std::clamp(moveto, 0, fp->exfunc->get_frame_n(editp) - page_size + margin);
		if (moveto != curr_scr_pos)
			tl_scroll_h.set_pos(moveto, editp);
		break;
	}

		// vertical (layerwise) scroll.
	case Menu::ScrollUp: dir = -1; goto scroll_UD;
	case Menu::ScrollDown: dir = 1; goto scroll_UD;
	scroll_UD:
		dir *= settings.scroll.layer_count;
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
				skip_midpoints, settings.skips.skip_inactive_objects);
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
			if (settings.skips.skip_hidden_layers && Timeline::is_hidden(scene_layer_settings[j])) continue;
			moveto = std::max(moveto,
				Timeline::find_adjacent_left(pos, j, skip_midpoints, settings.skips.skip_inactive_objects));
		}
		break;
	}

	// 以上 2 つの左右入れ替え．
	case Menu::StepObjRight: skip_midpoints = true; goto step_right;
	case Menu::StepMidptRight: skip_midpoints = false; goto step_right;
	step_right:
		if (int sel_idx = *exedit.SettingDialogObjectIndex; sel_idx >= 0) {
			moveto = Timeline::find_adjacent_right(pos, (*exedit.ObjectArray_ptr)[sel_idx].layer_set,
				skip_midpoints, settings.skips.skip_inactive_objects, len);
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
			if (settings.skips.skip_hidden_layers && Timeline::is_hidden(scene_layer_settings[j])) continue;
			moveto = std::min(moveto,
				Timeline::find_adjacent_right(pos, j, skip_midpoints, settings.skips.skip_inactive_objects, len));
		}
		break;
	}

	#define ChainBegin(obj)	(skip_midpoints ? Timeline::chain_begin(obj) : (obj)->frame_begin)
	#define ChainEnd(obj)		(skip_midpoints ? Timeline::chain_end(obj) : (obj)->frame_end)
		// 現在選択中のオブジェクト or 中間点区間まで現在フレームを移動．
	case Menu::StepIntoSelObj: skip_midpoints = true; goto step_into_selected;
	case Menu::StepIntoSelMidpt: skip_midpoints = false; goto step_into_selected;
	step_into_selected:
		if (auto sel_idx = *exedit.SettingDialogObjectIndex; sel_idx >= 0) {
			const auto& sel_obj = (*exedit.ObjectArray_ptr)[sel_idx];
			if (auto begin = ChainBegin(&sel_obj); pos < begin) moveto = begin;
			else if (auto end = ChainEnd(&sel_obj); end < pos) moveto = end;
		}
		break;
	#undef ChainBegin
	#undef ChainEnd
		}

		{
		int dir;
		// タイムラインの拡大率に応じたフレーム数で，画面上一定長さだけ移動．
	case Menu::StepLenLeft: dir = -1; goto step_len_LR;
	case Menu::StepLenRight: dir = 1; goto step_len_LR;
	step_len_LR:
		if (int coeff = *exedit.curr_timeline_scale_len; coeff > 0) {
			dir *= settings.scroll.seek_amount / coeff;
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

		// BPM グリッドに沿った移動．
		{
		int dir, step_n, step_d;
		// 小節線に移動．
	case Menu::StepMeasureBPMLeft: dir = -1; goto step_BPM_measure_LR;
	case Menu::StepMeasureBPMRight: dir = 1; goto step_BPM_measure_LR;
	step_BPM_measure_LR:
		step_n = *exedit.timeline_BPM_num_beats; step_d = 1;
		goto step_BPM;

		// 拍数で移動．
	case Menu::StepBeatBPMLeft: dir = -1; goto step_BPM_beat_LR;
	case Menu::StepBeatBPMRight: dir = 1; goto step_BPM_beat_LR;
	step_BPM_beat_LR:
		step_n = 1; step_d = 1;
		goto step_BPM;

		// 1/4 拍 (16分音符) 単位で移動．
	case Menu::StepQuarterBPMLeft: dir = -1; goto step_BPM_quarter_LR;
	case Menu::StepQuarterBPMRight: dir = 1; goto step_BPM_quarter_LR;
	step_BPM_quarter_LR:
		step_n = 1; step_d = 4;
		goto step_BPM;

		// 1/N 拍単位で移動．N は設定ファイルから指定．
	case Menu::StepNthBPMLeft: dir = -1; goto step_BPM_nth_LR;
	case Menu::StepNthBPMRight: dir = 1; goto step_BPM_nth_LR;
	step_BPM_nth_LR:
		step_n = 1; step_d = settings.bpm_grid.nth_beat;
		goto step_BPM;

	step_BPM:
		if (const BPM_Grid grid{ step_n, step_d, editp }) {
			// calculate the destination frame.
			if (dir < 0) moveto = grid.pos_from_beat(grid.beat_from_pos(pos - 1));
			else moveto = grid.pos_from_beat(grid.beat_from_pos(pos) + 1);

			moveto = std::clamp(moveto, 0, len - 1);
		}
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
		else moveto = std::clamp(pos, curr_scr_pos + margin, curr_scr_pos + page_size - margin);

		moveto = std::clamp(moveto, 0, len - 1);
		break;
	}
	}

	// 位置を特定できたので必要なら移動．
	if (pos == moveto) return false;

	// disable SHIFT key
	ForceKeyState shift{ settings.keyboard.suppress_shift ? VK_SHIFT : 0, false };
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
	ForceKeyState k{ VK_SHIFT, shift };

	// 拡張編集にコマンドメッセージ送信．
	return exedit.fp->func_WndProc(exedit.fp->hwnd, FilterPlugin::WindowMessage::Command,
		Menu::exedit_command_select_obj, 0, editp, exedit.fp) != FALSE;
}

// BPM 基準フレーム操作．
inline bool menu_misc_handler(Menu::ID id, EditHandle* editp, FilterPlugin* fp)
{
	int const pos = *exedit.timeline_BPM_frame_origin;
	int moveto = pos;
	switch (id) {
	case Menu::ID::BPMMoveOriginLeft: moveto--; break;
	case Menu::ID::BPMMoveOriginRight: moveto++; break;
	case Menu::ID::BPMMoveOriginCurrent:
		moveto = fp->exfunc->get_frame(editp) + 1;
		break;
	case Menu::ID::BPMFitBarToCurrent:
		if (const BPM_Grid grid{ *exedit.timeline_BPM_num_beats, 1, editp }) {
			// find the nearest two measure bars from the current frame.
			auto curr = fp->exfunc->get_frame(editp);
			auto b = grid.beat_from_pos(curr);
			auto l = grid.pos_from_beat(b), r = grid.pos_from_beat(b + 1);

			// move the origin to the nearer side.
			if (l + r < 2 * curr)
				moveto -= r - curr; // right.
			else moveto += curr - l; // left.
		}
		break;
	default: return false;
	}

	if (pos != moveto) {
		// apply the new value.
		*exedit.timeline_BPM_frame_origin = moveto;

		// redraw the grid if it's visible.
		if (*exedit.timeline_BPM_show_grid != 0)
			::InvalidateRect(exedit.fp->hwnd, nullptr, FALSE);
	}

	// no need to update the video frame.
	return false;
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

	// 必要ならドラッグ操作のために関数乗っ取り．
	Drag::init();

	return TRUE;
}

BOOL func_wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, EditHandle* editp, FilterPlugin* fp)
{
	using Message = FilterPlugin::WindowMessage;
	switch (message) {
	case Message::Exit:
		// ドラッグ操作中なら解除．関数の差し戻しはしないほうが無難．
		Drag::force_cancel();

		// message-only window を削除．必要ないかもしれないけど．
		fp->hwnd = nullptr; ::DestroyWindow(hwnd);
		break;

		// コマンドハンドラ．
	case Message::Command:
		if (auto id = static_cast<Menu::ID>(wparam); !Menu::is_invalid(id) &&
			fp->exfunc->is_editing(editp) && !fp->exfunc->is_saving(editp)) {

			// 系統によって処理の毛色が違うので分岐．
			return (
				Menu::is_seek(id)   ? menu_seek_obj_handler(id, editp, fp)	:
				Menu::is_scroll(id) ? menu_scroll_handler(id, editp, fp)	:
				Menu::is_select(id) ? menu_select_obj_handler(id, editp)	:
				Menu::is_misc(id)	? menu_misc_handler(id, editp, fp)		:
				false) ? TRUE : FALSE;
		}
		break;
	}
	return FALSE;
}


////////////////////////////////
// Entry point.
////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hinst);
		break;
	}
	return TRUE;
}


////////////////////////////////
// 看板．
////////////////////////////////
#define PLUGIN_NAME		"TLショトカ移動"
#define PLUGIN_VERSION	"v1.23"
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
		.func_WndProc = func_wndproc,
		.information = PLUGIN_INFO,
	};
	return &filter;
}
