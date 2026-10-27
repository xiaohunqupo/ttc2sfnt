#pragma once

/*!
 * \file ttc_extract.h
 * \brief Write the faces of a collection next to each other, named after the font they contain.
 */

#include "ttc2sfnt/ttc2sfnt_config.h"
#include "ttc2sfnt/ttc_collection.h"
#include "ttc2sfnt/ttc_error.h"
#include <cstddef>
#include <string>
#include <vector>

namespace ttc2sfnt
{
	/*!
	 * \brief File stem to use for one face.
	 *
	 * The family name wins; a name that is already in `used` gets `_<subfamily>`, and still a clash
	 * gets `_2`, `_3` ... A face without a usable `name` table falls back to
	 * `<fallback_prefix>_<index>`. Comparison ignores ASCII case, the way Windows does.
	 *
	 * \param face Metadata of the face.
	 * \param index Face index, used for the fallback and the collision suffix.
	 * \param fallback_prefix Prefix for a face that carries no family name.
	 * \param used Stems handed out so far.
	 * \return The stem, without extension.
	 */
	std::string file_stem_for(const Face& face, std::size_t index,
		const std::string& fallback_prefix, const std::vector<std::string>& used);

	/*!
	 * \brief Write every face next to each other as `<family>.ttf` / `<family>.otf`.
	 *
	 * The output directory is created when missing. A file that already exists is replaced.
	 *
	 * \param filename Collection to read.
	 * \param dir Output directory, `"."` writes next to the input.
	 * \param no_name_prefix Prefix for faces without a family name.
	 * \param [out] written Optional list of the files that were written.
	 * \param [out] error Optional diagnostic.
	 * \return `false` as soon as one face cannot be extracted or written; the files written before
	 *         that point are kept and are listed in `written`.
	 */
	bool UnzipTTCToDir(const std::string& filename, const std::string& dir,
		const std::string& no_name_prefix = "File",
		std::vector<std::string>* written = nullptr, error_info* error = nullptr);

	/*!
	 * \brief Write every face of an in-memory collection.
	 */
	bool UnzipTTCToDir(const void* data, std::size_t size, const std::string& dir,
		const std::string& no_name_prefix = "File",
		std::vector<std::string>* written = nullptr, error_info* error = nullptr);

	/*!
	 * \brief Write one face to an explicit path, creating the parent directory when missing.
	 */
	bool UnzipTTCFaceToFile(const std::string& filename, std::size_t index,
		const std::string& path, Face* face = nullptr, error_info* error = nullptr);
}

#ifdef TTC2SFNT_HEAD_ONLY
#include "ttc2sfnt/ttc_extract.cpp"
#endif // TTC2SFNT_HEAD_ONLY
