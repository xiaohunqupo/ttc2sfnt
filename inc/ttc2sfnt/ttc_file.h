#pragma once

/*!
 * \file ttc_file.h
 * \brief Whole file reads and writes plus the directory handling the extractors need.
 *
 * Paths are UTF-8 on every platform. On Windows they are converted to UTF-16 before the CRT is
 * called, so a font whose family name is Chinese still lands in a file with that name.
 */

#include "ttc2sfnt/ttc2sfnt_config.h"
#include "ttc2sfnt/ttc_error.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ttc2sfnt
{
	/*!
	 * \brief Read a whole file into a buffer.
	 *
	 * One allocation, one `fread`: the file is a font, not a stream, and the parser needs random
	 * access to every table anyway.
	 *
	 * \param path Path of the file to read.
	 * \param [out] out Receives the bytes; cleared when the call fails.
	 * \param [out] error Optional diagnostic.
	 * \return `false` when the file cannot be opened, sized or read to the end.
	 */
	bool read_file(const std::string& path, std::vector<std::uint8_t>& out, error_info* error = nullptr);

	/*!
	 * \brief Write a whole buffer to a file, replacing it when it exists.
	 * \param path Path of the file to write.
	 * \param data Bytes to write, may be `nullptr` when `size` is zero.
	 * \param size Byte count.
	 * \param [out] error Optional diagnostic.
	 */
	bool write_file(const std::string& path, const std::uint8_t* data, std::size_t size, error_info* error = nullptr);

	/*!
	 * \brief Create `path` and every missing parent directory.
	 * \param path Directory path, may be empty (nothing to do).
	 * \param [out] error Optional diagnostic.
	 * \return `true` when the directory exists afterwards, including the case where it already did.
	 *         `false` with `err_mkdir_failed` when the path cannot be created, and also when
	 *         something that is not a directory already owns one of the names.
	 */
	bool ensure_directory(const std::string& path, error_info* error = nullptr);

	/*!
	 * \brief Directory part of a path.
	 * \return Everything before the last `/` or `\`, `"."` for a bare file name, and the leading
	 *         `/` or `C:` when the path is at the root.
	 */
	std::string parent_directory(const std::string& path);

	/*!
	 * \brief Join a directory and a file name with a single separator.
	 */
	std::string join_path(const std::string& dir, const std::string& name);
}

#ifdef TTC2SFNT_HEAD_ONLY
#include "ttc2sfnt/ttc_file.cpp"
#endif // TTC2SFNT_HEAD_ONLY
