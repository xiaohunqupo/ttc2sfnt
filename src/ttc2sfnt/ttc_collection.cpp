/*!
 * \file ttc_collection.cpp
 * \brief The collection header, and the free functions that extract faces into memory.
 *
 * The header is the only place where a `.ttc` differs from a plain font: a `ttcf` signature, a
 * version, a face count, and one absolute offset per face. Everything after that offset is an
 * ordinary sfnt, which `Sfnt` knows how to parse and rebuild.
 */

#include "ttc2sfnt/ttc_collection.h"
#include "ttc2sfnt/common.h"
#include "ttc2sfnt/ttc_file.h"
#include "ttc2sfnt/ttc_name.h"

namespace ttc2sfnt
{
	TTC2SFNT_HEAD_ONLY_INLINE Collection::Collection()
		: m_valid(false), m_data(nullptr), m_size(0), m_version(0)
	{
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool Collection::parse(const std::uint8_t* data, std::size_t size, error_info* error)
	{
		m_valid = false;
		m_data = data;
		m_size = size;
		m_version = 0;
		m_offsets.clear();

		if (!data || size < details::kHeaderBytes)
			return details::fail(error, err_truncated, "file is smaller than the 12 byte collection header");
		if (details::rd_u32(data) != details::kTagTTCF)
			return details::fail(error, err_not_a_collection, "signature is not 'ttcf'");

		const std::uint32_t version = details::rd_u32(data + 4);
		if (version != 0x00010000u && version != 0x00020000u)
			return details::fail(error, err_unsupported_version, "unsupported collection version " + details::hex32(version));

		const std::uint32_t count = details::rd_u32(data + 8);
		if (count == 0)
			return details::fail(error, err_empty_collection, "collection declares zero faces");
		if (count > details::kMaxFaces)
			return details::fail(error, err_bad_face_count, "collection declares " +
				std::to_string((unsigned long long)count) + " faces, limit is " +
				std::to_string((unsigned long long)details::kMaxFaces));
		if ((std::size_t)count > (size - details::kHeaderBytes) / 4)
			return details::fail(error, err_truncated, "face offset array runs past the end of the file");

		// The offsets themselves are checked by `Sfnt::parse()`, which owns the sfnt rules and runs
		// for every face anyway.
		m_offsets.resize(count);
		for (std::uint32_t i = 0; i < count; ++i)
			m_offsets[i] = details::rd_u32(data + details::kHeaderBytes + (std::size_t)i * 4);

		m_version = version;
		m_valid = true;
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool Collection::sfnt_of(std::size_t index, Sfnt& out, error_info* error) const
	{
		if (!m_valid)
			return details::fail(error, err_not_a_collection, "the collection header was not parsed");
		if (index >= m_offsets.size())
		{
			out = Sfnt();
			return details::fail(error, err_out_of_range, "face index " + std::to_string((unsigned long long)index) +
				" is outside a collection of " + std::to_string((unsigned long long)m_offsets.size()));
		}
		if (out.parse(m_data, m_size, m_offsets[index], error))
			return true;

		// Say which face failed, the directory itself only knows its offset.
		if (error && !error->message.empty())
			error->message = "face " + std::to_string((unsigned long long)index) + ": " + error->message;
		return false;
	}

	//! Fill the metadata of one face from an already parsed directory.
	TTC2SFNT_HEAD_ONLY_INLINE void describe_face(const std::uint8_t* data, const Sfnt& sfnt, Face& out)
	{
		out = Face();
		out.sfnt_version = sfnt.version();
		out.table_count = (std::uint32_t)sfnt.table_count();

		const Table_Record* maxp = sfnt.find(details::kTagMaxp);
		if (maxp && maxp->length >= 6)
			out.num_glyphs = details::rd_u16(data + maxp->offset + 4);
		details::name_string(data, sfnt, 1, out.family);
		details::name_string(data, sfnt, 2, out.subfamily);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool Collection::describe(std::size_t index, Face& out, error_info* error) const
	{
		out = Face();
		Sfnt sfnt;
		if (!sfnt_of(index, sfnt, error))
			return false;
		describe_face(m_data, sfnt, out);
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool Collection::extract(std::size_t index, TTC::TTF& out, Face* face, error_info* error) const
	{
		out.clear();
		if (face)
			*face = Face();

		Sfnt sfnt;
		if (!sfnt_of(index, sfnt, error))
			return false;
		if (!sfnt.build(m_data, m_size, out, error))
			return false;
		if (face)
			describe_face(m_data, sfnt, *face);
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool Collection::extract_all(TTC& out, error_info* error) const
	{
		out.version = 0;
		out.ttfs.clear();
		out.faces.clear();
		if (!m_valid)
			return details::fail(error, err_not_a_collection, "the collection header was not parsed");

		out.ttfs.resize(m_offsets.size());
		out.faces.resize(m_offsets.size());
		for (std::size_t i = 0; i < m_offsets.size(); ++i)
		{
			if (!extract(i, out.ttfs[i], &out.faces[i], error))
			{
				out.version = 0;
				out.ttfs.clear();
				out.faces.clear();
				return false;
			}
		}
		out.version = m_version;
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool ListTTC(const void* data, std::size_t size, std::vector<Face>& faces, error_info* error)
	{
		faces.clear();
		Collection collection;
		if (!collection.parse((const std::uint8_t*)data, size, error))
			return false;

		faces.resize(collection.face_count());
		for (std::size_t i = 0; i < collection.face_count(); ++i)
		{
			if (!collection.describe(i, faces[i], error))
			{
				faces.clear();
				return false;
			}
		}
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool ListTTC(const std::string& filename, std::vector<Face>& faces, error_info* error)
	{
		std::vector<std::uint8_t> data;
		if (!read_file(filename, data, error))
		{
			faces.clear();
			return false;
		}
		return ListTTC(data.empty() ? nullptr : &data[0], data.size(), faces, error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool UnzipTTC(const void* data, std::size_t size, TTC& ttc, error_info* error)
	{
		Collection collection;
		if (!collection.parse((const std::uint8_t*)data, size, error))
		{
			ttc.version = 0;
			ttc.ttfs.clear();
			ttc.faces.clear();
			return false;
		}
		return collection.extract_all(ttc, error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool UnzipTTC(const std::string& filename, TTC& ttc, error_info* error)
	{
		std::vector<std::uint8_t> data;
		if (!read_file(filename, data, error))
		{
			ttc.version = 0;
			ttc.ttfs.clear();
			ttc.faces.clear();
			return false;
		}
		return UnzipTTC(data.empty() ? nullptr : &data[0], data.size(), ttc, error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool UnzipTTCFace(const void* data, std::size_t size, std::size_t index,
		TTC::TTF& ttf, Face* face, error_info* error)
	{
		ttf.clear();
		if (face)
			*face = Face();

		Collection collection;
		if (!collection.parse((const std::uint8_t*)data, size, error))
			return false;
		return collection.extract(index, ttf, face, error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool UnzipTTCFace(const std::string& filename, std::size_t index,
		TTC::TTF& ttf, Face* face, error_info* error)
	{
		std::vector<std::uint8_t> data;
		if (!read_file(filename, data, error))
		{
			ttf.clear();
			if (face)
				*face = Face();
			return false;
		}
		return UnzipTTCFace(data.empty() ? nullptr : &data[0], data.size(), index, ttf, face, error);
	}
}
