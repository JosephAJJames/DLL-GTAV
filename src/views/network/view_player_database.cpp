#include "core/data/block_join_reasons.hpp"
#include "core/data/command_access_levels.hpp"
#include "core/data/infractions.hpp"
#include "fiber_pool.hpp"
#include "pointers.hpp"
#include "services/api/api_service.hpp"
#include "services/player_database/player_database_service.hpp"
#include "services/players/player_service.hpp"
#include "util/session.hpp"
#include "views/view.hpp"

namespace big
{
	char name_buf[32];
	char search[64];
	char note_buffer[1024];
	bool notes_dirty = false;
	std::shared_ptr<persistent_player> current_player;

	void draw_player_db_entry(std::shared_ptr<persistent_player> player, const std::string& lower_search)
	{
		std::string name = player->name;
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);

		if (lower_search.empty() || name.find(lower_search) != std::string::npos)
		{
			ImGui::PushID(player->rockstar_id);

			float circle_size = 7.5f;
			auto cursor_pos   = ImGui::GetCursorScreenPos();
			auto plyr_state   = player->online_state;

			//render status circle
			ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(cursor_pos.x + 4.f + circle_size, cursor_pos.y + 4.f + circle_size),
			    circle_size,
			    ImColor(plyr_state == PlayerOnlineStatus::ONLINE  ? ImVec4(0.f, 1.f, 0.f, 1.f) :
			            plyr_state == PlayerOnlineStatus::OFFLINE ? ImVec4(1.f, 0.f, 0.f, 1.f) :
			            plyr_state == PlayerOnlineStatus::UNKNOWN ? ImVec4(.5f, .5f, .5f, 1.0f) :
			                                                        ImVec4(.5f, .5f, .5f, 1.0f)));

			//we need some padding
			ImVec2 cursor = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cursor.x + 25.f, cursor.y));

			if (components::selectable(player->name, player == g_player_database_service->get_selected()))
			{
				if (notes_dirty)
				{
					// Ensure notes are saved
					g_player_database_service->save();
					notes_dirty = false;
				}

				g_player_database_service->set_selected(player);
				current_player = player;
				strncpy(name_buf, current_player->name.data(), sizeof(name_buf));
				strncpy(note_buffer, current_player->notes.data(), sizeof(note_buffer));
			}

			ImGui::PopID();
		}
	}

	void view::player_database()
	{
		ImGui::SetNextItemWidth(300.f);
		components::input_text_with_hint("PLAYER"_T, "SEARCH"_T, search, sizeof(search), ImGuiInputTextFlags_None);

		if (ImGui::BeginListBox("###players", {180, static_cast<float>(*g_pointers->m_gta.m_resolution_y - 400 - 38 * 4)}))
		{
			auto& item_arr = g_player_database_service->get_sorted_players();
			if (item_arr.size() > 0)
			{
				std::string lower_search = search;
				std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), tolower);

				for (auto& player : item_arr | std::ranges::views::values)
				{
					if (player->online_state == PlayerOnlineStatus::ONLINE)
						draw_player_db_entry(player, lower_search);
				}

				for (auto& player : item_arr | std::ranges::views::values)
				{
					if (player->online_state != PlayerOnlineStatus::ONLINE)
						draw_player_db_entry(player, lower_search);
				}
			}
			else
			{
				ImGui::Text("NO_STORED_PLAYERS"_T.data());
			}

			ImGui::EndListBox();
		}

		if (auto selected = g_player_database_service->get_selected())
		{
			ImGui::SameLine();
			if (ImGui::BeginChild("###selected_player", {500, static_cast<float>(*g_pointers->m_gta.m_resolution_y - 388 - 38 * 4)}, false, ImGuiWindowFlags_NoBackground))
			{
				if (ImGui::InputText("NAME"_T.data(), name_buf, sizeof(name_buf)))
				{
					current_player->name = name_buf;
				}

				if (ImGui::InputScalar("RID"_T.data(), ImGuiDataType_S64, &current_player->rockstar_id)
				    || ImGui::Checkbox("IS_MODDER"_T.data(), &current_player->is_modder)
				    || ImGui::Checkbox("BLOCK_JOIN"_T.data(), &current_player->block_join)
				    || ImGui::Checkbox("Notify When Online", &current_player->notify_online))
				{
					if (current_player->rockstar_id != selected->rockstar_id)
						g_player_database_service->update_rockstar_id(selected->rockstar_id, current_player->rockstar_id);
					g_player_database_service->save();
				}

				if (ImGui::BeginCombo("BLOCK_JOIN_ALERT"_T.data(), block_join_reasons[current_player->block_join_reason]))
				{
					for (const auto& reason : block_join_reasons)
					{
						if (ImGui::Selectable(reason.second, reason.first == current_player->block_join_reason))
						{
							current_player->block_join_reason = reason.first;
							g_player_database_service->save();
						}

						if (reason.first == current_player->block_join_reason)
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("ONLY_AS_HOST"_T.data());


				if (ImGui::BeginCombo("CHAT_COMMAND_PERMISSIONS"_T.data(),
				        COMMAND_ACCESS_LEVELS[current_player->command_access_level.value_or(g.session.chat_command_default_access_level)]))
				{
					for (const auto& [type, name] : COMMAND_ACCESS_LEVELS)
					{
						if (ImGui::Selectable(name, type == current_player->command_access_level.value_or(g.session.chat_command_default_access_level)))
						{
							current_player->command_access_level = type;
							g_player_database_service->save();
						}

						if (type == current_player->command_access_level.value_or(g.session.chat_command_default_access_level))
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}

				if (!current_player->infractions.empty())
				{
					ImGui::Text("INFRACTIONS"_T.data());

					for (auto& infraction : current_player->infractions)
					{
						ImGui::BulletText(infraction_desc[(Infraction)infraction]);
					}
				}

				if (ImGui::InputTextMultiline("Notes", note_buffer, sizeof(note_buffer)))
				{
					current_player->notes = note_buffer;
					notes_dirty           = true;
				}

				components::button("JOIN_SESSION"_T, [] {
					session::join_by_rockstar_id(current_player->rockstar_id);
				});

				static char message[256];
				components::input_text("INPUT_MSG"_T, message, sizeof(message));
				if (components::button("SEND_MSG"_T))
				{
					g_thread_pool->push([selected] {
						if (g_api_service->send_socialclub_message(selected->rockstar_id, message))
						{
							g_notification_service->push_success("SCAPI"_T.data(), "MSG_SENT_SUCCESS"_T.data());
							return;
						}
						g_notification_service->push_error("SCAPI"_T.data(), "MSG_SENT_FAIL"_T.data());
					});
				};

				if (ImGui::Button("SAVE"_T.data()))
				{
					if (current_player->rockstar_id != selected->rockstar_id)
						g_player_database_service->update_rockstar_id(selected->rockstar_id, current_player->rockstar_id);

					selected = current_player;
					g_player_database_service->save();
				}

				ImGui::SameLine();

				if (ImGui::Button("REMOVE"_T.data()))
				{
					g_player_database_service->remove_rockstar_id(selected->rockstar_id);
				}
			}
			ImGui::EndChild();
		}

		if (ImGui::Button("REMOVE_ALL"_T.data()))
		{
			g_player_database_service->set_selected(nullptr);
			g_player_database_service->get_players().clear();
			g_player_database_service->get_sorted_players().clear();
			g_player_database_service->save();
		}

		ImGui::SameLine();

		components::button("RELOAD_PLYR_ONLINE_STATES"_T, [] {
			g_player_database_service->update_player_states();
		});

		if (components::command_checkbox<"player_db_auto_update_states">())
			g_player_database_service->start_update_loop();

		ImGui::Separator();
		components::sub_title("NEW_ENTRY"_T);

		static char new_name[64];
		static int64_t new_rockstar_id;

		components::input_text("NAME"_T, new_name, sizeof(new_name));
		ImGui::InputScalar("RID"_T.data(), ImGuiDataType_S64, &new_rockstar_id);

		if (ImGui::Button("ADD"_T.data()))
		{
			current_player = g_player_database_service->add_player(new_rockstar_id, new_name);
			g_player_database_service->save();
		}
	}
}
