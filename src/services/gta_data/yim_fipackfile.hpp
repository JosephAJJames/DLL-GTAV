#pragma once
#include <pugixml.hpp>

namespace big
{
	using file_contents_callback = std::function<void(const std::unique_ptr<std::uint8_t[]>& file_content, const int data_size)>;

	class yim_fipackfile
	{
		rage::fiPackfile* rpf;
		std::string mount_name;

	public:
		explicit yim_fipackfile(rage::fiPackfile* rpf, const std::string& mount_name = "/");

		static inline std::vector<std::function<size_t(yim_fipackfile& rpf_wrapper)>> m_wrapper_call_back;

		static void add_wrapper_call_back(std::function<size_t(yim_fipackfile& rpf_wrapper)> cb);

		static void for_each_fipackfile();

		std::vector<std::filesystem::path> get_file_paths(std::string parent = {});

		const char* get_name();

		void read_file(const std::filesystem::path& path, file_contents_callback&& cb);

		void read_xml_file(const std::filesystem::path& path, std::function<void(pugi::xml_document& doc)> cb);
	};
}
