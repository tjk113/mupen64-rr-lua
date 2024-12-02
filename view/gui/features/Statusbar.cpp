#include "Statusbar.hpp"

#include <Windows.h>
#include <commctrl.h>
#include <numeric>

#include <shared/Messenger.h>
#include <core/r4300/r4300.h>
#include <core/r4300/vcr.h>
#include "RomBrowser.hpp"
#include <view/resource.h>
#include <view/helpers/WinHelpers.h>
#include <shared/Config.hpp>
#include <view/gui/Main.h>
#include <view/gui/Loggers.h>

namespace Statusbar
{
	typedef struct Segment
	{
		Section section;
		size_t width;
		bool overlap_with_previous = false;
	} t_segment;

	typedef struct
	{
		std::vector<t_segment> emu_parts;
		std::vector<t_segment> idle_parts;
	} t_segment_layout;

	std::unordered_map<StatusbarLayout, t_segment_layout> g_layout_map = {
		{
			StatusbarLayout::Classic, t_segment_layout{
				.emu_parts = {
					t_segment{
						.section = Section::VCR,
						.width = 260,
					},
					t_segment{
						.section = Section::Notification,
						.width = 260,
						.overlap_with_previous = true,
					},
					t_segment{
						.section = Section::FPS,
						.width = 70,
					},
					t_segment{
						.section = Section::VIs,
						.width = 70,
					},
					t_segment{
						.section = Section::Input,
						.width = 140,
					},
				},
				.idle_parts = {},
			}
		},
		{
			StatusbarLayout::Modern, t_segment_layout{
				.emu_parts = {
					t_segment{
						.section = Section::Notification,
						.width = 200,
					},
					t_segment{
						.section = Section::VCR,
						.width = 200,
					},
					t_segment{
						.section = Section::Input,
						.width = 80,
					},
					t_segment{
						.section = Section::Rerecords,
						.width = 80,
					},
					t_segment{
						.section = Section::FPS,
						.width = 80,
					},
					t_segment{
						.section = Section::VIs,
						.width = 80,
					},
					t_segment{
						.section = Section::Slot,
						.width = 80,
					},
				},
				.idle_parts = {},
			}
		},
		{
			StatusbarLayout::ModernWithReadonly, t_segment_layout{
				.emu_parts = {},
				.idle_parts = {},
			}
		}
	};


	HWND statusbar_hwnd;

	HWND hwnd()
	{
		return statusbar_hwnd;
	}

	std::vector<t_segment> get_current_parts()
	{
		const t_segment_layout& layout = g_layout_map[static_cast<StatusbarLayout>(g_config.statusbar_layout)];
		return emu_launched ? layout.emu_parts : layout.idle_parts;
	}

	size_t section_to_segment_index(const Section section)
	{
		size_t i = 0;
		auto parts = get_current_parts();

		for (const auto& part : parts)
		{
			if (part.overlap_with_previous)
			{
				continue;
			}

			if (part.section == section)
			{
				return i;
			}

			i++;
		}

		return SIZE_MAX;
	}

	void post(const std::string& text, Section section)
	{
		const auto segment_index = section_to_segment_index(section);

		if (segment_index == SIZE_MAX)
		{
			return;
		}

		SendMessage(statusbar_hwnd, SB_SETTEXT, segment_index, (LPARAM)text.c_str());
	}

	void update_size()
	{
		auto parts = get_current_parts();

		std::vector<int32_t> sizes;
		for (const auto& part : parts)
		{
			if (part.overlap_with_previous)
			{
				continue;
			}
			sizes.push_back(static_cast<int32_t>(part.width));
		}

		RECT rect{};
		GetClientRect(g_main_hwnd, &rect);

		if (parts.empty())
		{
			set_statusbar_parts(statusbar_hwnd, {-1});
			return;
		}

		// Compute the desired size of the statusbar and use that for the scaling factor
		auto desired_size = std::accumulate(sizes.begin(), sizes.end() - 1, 0);

		auto scale = static_cast<float>(rect.right - rect.left) / static_cast<float>(desired_size);

		if (!g_config.statusbar_scale_down)
		{
			scale = max(scale, 1.0f);
		}

		if (!g_config.statusbar_scale_up)
		{
			scale = min(scale, 1.0f);
		}

		for (auto& size : sizes)
		{
			size = static_cast<int>(size * scale);
		}

		set_statusbar_parts(statusbar_hwnd, sizes);
	}

	void emu_launched_changed(std::any data)
	{
		auto value = std::any_cast<bool>(data);
		static auto previous_value = value;

		if (!statusbar_hwnd)
		{
			previous_value = value;
			return;
		}

		// We don't want to keep the weird crap from previous state, so let's clear everything
		for (int i = 0; i < 255; ++i)
		{
			SendMessage(statusbar_hwnd, SB_SETTEXT, i, (LPARAM)"");
		}

		// When starting the emu, we want to scale the statusbar segments to the window size
		if (value)
		{
			// Update this at first start, otherwise it doesnt initially appear
			Messenger::broadcast(Messenger::Message::SlotChanged, (size_t)g_config.st_slot);
		}

		update_size();

		previous_value = value;
	}

	void create()
	{
		// undocumented behaviour of CCS_BOTTOM: it skips applying SBARS_SIZEGRIP in style pre-computation phase
		statusbar_hwnd = CreateWindowEx(0, STATUSCLASSNAME, nullptr,
		                                WS_CHILD | WS_VISIBLE | CCS_BOTTOM,
		                                0, 0,
		                                0, 0,
		                                g_main_hwnd, (HMENU)IDC_MAIN_STATUS,
		                                g_app_instance, nullptr);
	}


	void statusbar_visibility_changed(std::any data)
	{
		auto value = std::any_cast<bool>(data);

		if (statusbar_hwnd)
		{
			DestroyWindow(statusbar_hwnd);
			statusbar_hwnd = nullptr;
		}

		if (value)
		{
			create();
		}
	}

	void on_readonly_changed(std::any data)
	{
		auto value = std::any_cast<bool>(data);
		post(value ? "Read-only" : "Read/write", Section::Readonly);
	}

	void on_rerecords_changed(std::any data)
	{
		auto value = std::any_cast<uint64_t>(data);
		post(std::format("{} rr", value), Section::Rerecords);
	}

	void on_task_changed(std::any data)
	{
		auto value = std::any_cast<e_task>(data);

		if (value == e_task::idle)
		{
			post("", Section::Rerecords);
		}
	}

	void on_slot_changed(std::any data)
	{
		auto value = std::any_cast<size_t>(data);
		post(std::format("Slot {}", value + 1), Section::Slot);
	}

	void on_size_changed(std::any)
	{
		update_size();
	}

	void init()
	{
		create();
		Messenger::subscribe(Messenger::Message::EmuLaunchedChanged,
		                     emu_launched_changed);
		Messenger::subscribe(Messenger::Message::StatusbarVisibilityChanged,
		                     statusbar_visibility_changed);
		Messenger::subscribe(Messenger::Message::ReadonlyChanged,
		                     on_readonly_changed);
		Messenger::subscribe(Messenger::Message::TaskChanged,
		                     on_task_changed);
		Messenger::subscribe(Messenger::Message::RerecordsChanged,
		                     on_rerecords_changed);
		Messenger::subscribe(Messenger::Message::SlotChanged,
		                     on_slot_changed);
		Messenger::subscribe(Messenger::Message::SizeChanged,
		                     on_size_changed);
	}
}
