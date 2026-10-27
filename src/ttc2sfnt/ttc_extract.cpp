/*!
 * \file ttc_extract.cpp
 * \brief Writing faces to disk, named after the font they contain.
 *
 * The naming rules are the interesting part here: two faces of one collection may carry the same
 * family name (`Same Family` / `Regular` and `Same Family` / `Bold`), a name may contain characters
 * a file system rejects, and a face may carry no `name` table at all. The stem therefore comes from
 * the family, gains the subfamily and finally a counter until it is unique.
 */

#include "ttc2sfnt/ttc_extract.h"
#include "ttc2sfnt/common.h"
#include "ttc2sfnt/ttc_file.h"
#include "ttc2sfnt/ttc_name.h"

namespace ttc2sfnt
{
	namespace
	{
		//! Has a stem already been handed out? Case insensitive, the way Windows compares file names.
		bool stem_taken(const std::string& stem, const std::vector<std::string>& used)
		{
			for (std::size_t i = 0; i < used.size(); ++i)
			{
				if (same_file_name(stem, used[i]))
					return true;
			}
			return false;
		}
	}

	TTC2SFNT_HEAD_ONLY_INLINE std::string file_stem_for(const Face& face, std::size_t index,
		const std::string& fallback_prefix, const std::vector<std::string>& used)
	{
		std::string base = sanitize_file_name(face.family);
		if (base.empty())
			base = sanitize_file_name(fallback_prefix) + "_" + details::number_suffix(index);
		if (base.empty())
			base = "File_" + details::number_suffix(index);

		std::string candidate = base;
		const std::string sub = sanitize_file_name(face.subfamily);
		if (stem_taken(candidate, used) && !sub.empty())
			candidate = base + "_" + sub;
		// `used` holds at most one stem per face and the numeric candidates below are distinct from
		// each other, so among `used.size() + 1` of them at least one cannot be taken.
		for (std::size_t n = 2; stem_taken(candidate, used) && n <= used.size() + 2; ++n)
			candidate = base + "_" + (sub.empty() ? std::string() : sub + "_") + std::to_string((unsigned long long)n);
		return candidate;
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool UnzipTTCToDir(const void* data, std::size_t size, const std::string& dir,
		const std::string& no_name_prefix, std::vector<std::string>* written, error_info* error)
	{
		if (written)
			written->clear();

		Collection collection;
		if (!collection.parse((const std::uint8_t*)data, size, error))
			return false;
		if (!ensure_directory(dir, error))
			return false;

		std::vector<std::string> used;
		std::vector<std::uint8_t> face_bytes;
		for (std::size_t i = 0; i < collection.face_count(); ++i)
		{
			Face face;
			if (!collection.extract(i, face_bytes, &face, error))
				return false;

			const std::string stem = file_stem_for(face, i, no_name_prefix, used);
			const std::string extension = (face.sfnt_version == details::kSfntOTTO) ? ".otf" : ".ttf";
			const std::string path = join_path(dir, stem + extension);

			if (!write_file(path, face_bytes.empty() ? nullptr : &face_bytes[0], face_bytes.size(), error))
				return false;

			used.push_back(stem);
			if (written)
				written->push_back(path);
		}
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool UnzipTTCToDir(const std::string& filename, const std::string& dir,
		const std::string& no_name_prefix, std::vector<std::string>* written, error_info* error)
	{
		std::vector<std::uint8_t> data;
		if (!read_file(filename, data, error))
		{
			if (written)
				written->clear();
			return false;
		}
		return UnzipTTCToDir(data.empty() ? nullptr : &data[0], data.size(), dir, no_name_prefix, written, error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool UnzipTTCFaceToFile(const std::string& filename, std::size_t index,
		const std::string& path, Face* face, error_info* error)
	{
		TTC::TTF bytes;
		if (!UnzipTTCFace(filename, index, bytes, face, error))
			return false;
		if (!ensure_directory(parent_directory(path), error))
			return false;
		return write_file(path, bytes.empty() ? nullptr : &bytes[0], bytes.size(), error);
	}
}
