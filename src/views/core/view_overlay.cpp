#include "gta_util.hpp"
#include "gui.hpp"
#include "pointers.hpp"
#include "views/view.hpp"

namespace big
{
	void view::overlay()
	{
		if (!g.window.ingame_overlay.opened || (g_gui->is_open() && !g.window.ingame_overlay.show_with_menu_opened))
			return;

		g_gui->push_theme_colors();

		ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver, ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.5f);

		if (ImGui::Begin("overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
		{
			ImGui::Text("YimMenu");

			ImGui::Separator();

			if (g.window.ingame_overlay.show_fps)
				ImGui::Text("%.0f FPS", ImGui::GetIO().Framerate / 2);

			if (CNetworkPlayerMgr* network_player_mgr = gta_util::get_network_player_mgr(); g.window.ingame_overlay.show_players)
				ImGui::Text(std::format("Players: {}/{}", network_player_mgr->m_player_count, network_player_mgr->m_player_limit)
				                .c_str());

			if (g.window.ingame_overlay.show_position && g_local_player)
			{
				ImGui::Separator();

				auto& pos = *g_local_player->get_position();

				ImGui::Text("Pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
			}

			// can't easily get used item count using pools, so keeping replay interface for now
			if (auto replay_interface = *g_pointers->m_gta.m_replay_interface; g.window.ingame_overlay.show_replay_interface)
			{
				if (replay_interface->m_ped_interface || replay_interface->m_vehicle_interface || replay_interface->m_object_interface)
					ImGui::Separator();

				if (replay_interface->m_ped_interface)
					ImGui::Text(std::format("Ped Pool: {}/{}",
					    replay_interface->m_ped_interface->m_cur_peds,
					    replay_interface->m_ped_interface->m_max_peds)
					                .c_str());

				if (replay_interface->m_vehicle_interface)
					ImGui::Text(std::format("Vehicle Pool: {}/{}",
					    replay_interface->m_vehicle_interface->m_cur_vehicles,
					    replay_interface->m_vehicle_interface->m_max_vehicles)
					                .c_str());

				if (replay_interface->m_object_interface)
					ImGui::Text(std::format("Object Pool: {}/{}",
					    replay_interface->m_object_interface->m_cur_objects,
					    replay_interface->m_object_interface->m_max_objects)
					                .c_str());
			}

			if (g.window.ingame_overlay.show_game_versions)
			{
				ImGui::Separator();
				ImGui::Text(std::format("Game Version: {}", g_pointers->m_gta.m_game_version).c_str());
				ImGui::Text(std::format("Online Version: {}", g_pointers->m_gta.m_online_version).c_str());
			}
		}
		ImGui::End();

		g_gui->pop_theme_colors();
	}
}
