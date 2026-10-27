/*!
 * \file ttc_file.cpp
 * \brief Reading and writing whole files, plus the directory handling the extractors need.
 *
 * The library has no dependencies and therefore uses the C `stdio` and `mkdir` families. Everything
 * platform specific is confined to this file: the seek calls that can address more than two
 * gigabytes, and the UTF-8 to UTF-16 conversion that makes non ASCII font names into valid Windows
 * paths.
 */

#include "ttc2sfnt/ttc_file.h"
#include "ttc2sfnt/common.h"
#include <cerrno>
#include <cstdio>

#if defined(_WIN32)
#	include <direct.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

namespace ttc2sfnt
{
	namespace
	{
		/*!
		 * \brief Size of an open file, leaving the read position at the beginning.
		 * \return `false` when the stream cannot be seeked or is too large to address.
		 */
		bool file_size(FILE* file, std::uint64_t& size)
		{
			size = 0;
#if defined(_MSC_VER)
			if (_fseeki64(file, 0, SEEK_END) != 0)
				return false;
			const __int64 end = _ftelli64(file);
			if (end < 0 || _fseeki64(file, 0, SEEK_SET) != 0)
				return false;
			size = (std::uint64_t)end;
			return true;
#elif defined(__GNUC__) || defined(__clang__) || defined(__APPLE__)
			if (fseeko(file, 0, SEEK_END) != 0)
				return false;
			const off_t end = ftello(file);
			if (end < 0 || fseeko(file, 0, SEEK_SET) != 0)
				return false;
			size = (std::uint64_t)end;
			return true;
#else
			if (std::fseek(file, 0, SEEK_END) != 0)
				return false;
			const long end = std::ftell(file);
			if (end < 0 || std::fseek(file, 0, SEEK_SET) != 0)
				return false;
			size = (std::uint64_t)end;
			return true;
#endif
		}

#if defined(_WIN32)
		/*!
		 * \brief UTF-8 to UTF-16, so that non ASCII font names become valid paths.
		 */
		std::wstring utf8_to_wide(const std::string& text)
		{
			std::wstring out;
			out.reserve(text.size());
			std::size_t i = 0;
			while (i < text.size())
			{
				const unsigned char lead = (unsigned char)text[i];
				std::uint32_t cp = 0xFFFDu;
				int extra = 0;
				if (lead < 0x80u)
					cp = lead;
				else if ((lead & 0xE0u) == 0xC0u)
				{
					cp = lead & 0x1Fu;
					extra = 1;
				}
				else if ((lead & 0xF0u) == 0xE0u)
				{
					cp = lead & 0x0Fu;
					extra = 2;
				}
				else if ((lead & 0xF8u) == 0xF0u)
				{
					cp = lead & 0x07u;
					extra = 3;
				}
				++i;
				for (int k = 0; k < extra && i < text.size(); ++k, ++i)
					cp = (cp << 6) | ((unsigned char)text[i] & 0x3Fu);
				if (cp >= 0x10000u)
				{
					cp -= 0x10000u;
					out.push_back((wchar_t)(0xD800u + (cp >> 10)));
					out.push_back((wchar_t)(0xDC00u + (cp & 0x3FFu)));
				}
				else
					out.push_back((wchar_t)cp);
			}
			return out;
		}
#endif

		FILE* open_read(const std::string& path)
		{
#if defined(_MSC_VER)
			FILE* file = nullptr;
			if (_wfopen_s(&file, utf8_to_wide(path).c_str(), L"rb") != 0)
				return nullptr;
			return file;
#elif defined(_WIN32)
			return _wfopen(utf8_to_wide(path).c_str(), L"rb");
#else
			return std::fopen(path.c_str(), "rb");
#endif
		}

		FILE* open_write(const std::string& path)
		{
#if defined(_MSC_VER)
			FILE* file = nullptr;
			if (_wfopen_s(&file, utf8_to_wide(path).c_str(), L"wb") != 0)
				return nullptr;
			return file;
#elif defined(_WIN32)
			return _wfopen(utf8_to_wide(path).c_str(), L"wb");
#else
			return std::fopen(path.c_str(), "wb");
#endif
		}

		//! `true` when the path names an existing directory (a plain file does not count).
		bool is_directory(const std::string& path)
		{
#if defined(_WIN32)
			struct _stat info;
			if (::_wstat(utf8_to_wide(path).c_str(), &info) != 0)
				return false;
			return (info.st_mode & _S_IFDIR) != 0;
#else
			struct stat info;
			if (::stat(path.c_str(), &info) != 0)
				return false;
			return S_ISDIR(info.st_mode) != 0;
#endif
		}

		/*!
		 * \brief Create one directory.
		 * \return 1 when it exists afterwards (created, or already a directory), 0 when it cannot be
		 *         created, and -1 when something that is not a directory already owns the name.
		 * \note `mkdir` reports "already exists" for a file just like for a directory, so the state
		 *       has to be confirmed with a second query.
		 */
		int make_dir(const std::string& path)
		{
#if defined(_WIN32)
			if (::_wmkdir(utf8_to_wide(path).c_str()) == 0)
				return 1;
#else
			if (::mkdir(path.c_str(), 0777) == 0)
				return 1;
#endif
			if (errno == EEXIST)
				return is_directory(path) ? 1 : -1;
			return 0;
		}
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool read_file(const std::string& path, std::vector<std::uint8_t>& out, error_info* error)
	{
		out.clear();
		FILE* file = open_read(path);
		if (!file)
			return details::fail(error, err_open_failed, "cannot open '" + path + "'");

		std::uint64_t size = 0;
		bool ok = file_size(file, size) && size <= (std::uint64_t)(std::size_t)-1;
		if (ok)
		{
			out.resize((std::size_t)size);
			if (!out.empty() && std::fread(&out[0], 1, out.size(), file) != out.size())
				ok = false;
		}
		ok = ok && std::ferror(file) == 0;
		std::fclose(file);
		if (!ok)
		{
			out.clear();
			return details::fail(error, err_read_failed, "cannot read '" + path + "'");
		}
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool write_file(const std::string& path, const std::uint8_t* data, std::size_t size, error_info* error)
	{
		FILE* file = open_write(path);
		if (!file)
			return details::fail(error, err_write_failed, "cannot create '" + path + "'");
		bool ok = size == 0 || std::fwrite(data, 1, size, file) == size;
		ok = (std::fclose(file) == 0) && ok;
		if (!ok)
			return details::fail(error, err_write_failed, "cannot write '" + path + "'");
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool ensure_directory(const std::string& path, error_info* error)
	{
		if (path.empty())
			return details::pass(error);

		std::string partial;
		std::size_t i = 0;
		if (path.size() >= 2 && path[1] == ':')
		{
			partial = path.substr(0, 2);	// "C:"
			i = 2;
		}
		else if (path[0] == '/' || path[0] == '\\')
		{
			partial = path.substr(0, 1);	// "/"
			i = 1;
		}

		while (i < path.size())
		{
			while (i < path.size() && (path[i] == '/' || path[i] == '\\'))
				++i;
			const std::size_t start = i;
			while (i < path.size() && path[i] != '/' && path[i] != '\\')
				++i;
			if (start == i)
				break;
			const char last = partial.empty() ? '\0' : partial[partial.size() - 1];
			if (!partial.empty() && last != '/' && last != '\\')
				partial.push_back('/');
			partial.append(path, start, i - start);
			const int state = make_dir(partial);
			if (state == 0)
				return details::fail(error, err_mkdir_failed, "cannot create directory '" + partial + "'");
			if (state < 0)
				return details::fail(error, err_mkdir_failed,
					"cannot create directory '" + partial + "': a file with that name is in the way");
		}
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE std::string parent_directory(const std::string& path)
	{
		const std::size_t slash = path.find_last_of("/\\");
		if (slash == std::string::npos)
			return std::string(".");
		if (slash == 0)
			return path.substr(0, 1);
		return path.substr(0, slash);
	}

	TTC2SFNT_HEAD_ONLY_INLINE std::string join_path(const std::string& dir, const std::string& name)
	{
		if (dir.empty())
			return name;
		const char last = dir[dir.size() - 1];
		if (last == '/' || last == '\\')
			return dir + name;
		return dir + "/" + name;
	}
}
