/*!
 * \file ttc_sfnt.cpp
 * \brief Parsing one sfnt table directory out of a collection, and rebuilding it as a standalone
 *        font.
 *
 * The byte layout this file relies on is documented in `docs/ttc_format.md`. In short: a face is an
 * ordinary sfnt header plus one 16 byte record per table, but the table payloads live somewhere else
 * in the collection and are shared with the other faces. The rebuild therefore copies the payloads,
 * rewrites every record with its new offset, recomputes every table checksum, and finally writes
 * `head.checkSumAdjustment` so that the whole file sums to `0xB1B0AFBA`.
 */

#include "ttc2sfnt/ttc_sfnt.h"
#include "ttc2sfnt/common.h"
#include <cstring>

namespace ttc2sfnt
{
	TTC2SFNT_HEAD_ONLY_INLINE Sfnt::Sfnt()
		: m_valid(false), m_version(0)
	{
	}

	TTC2SFNT_HEAD_ONLY_INLINE std::uint32_t Sfnt::checksum(const std::uint8_t* data, std::size_t size)
	{
		std::uint32_t sum = 0;
		std::size_t i = 0;
		for (; i + 4 <= size; i += 4)
			sum += details::rd_u32(data + i);
		if (i < size)
		{
			std::uint32_t last = 0;
			for (std::size_t k = 0; k < 4; ++k)
			{
				last <<= 8;
				if (i + k < size)
					last |= data[i + k];
			}
			sum += last;
		}
		return sum;
	}

	TTC2SFNT_HEAD_ONLY_INLINE std::string Sfnt::tag_string(std::uint32_t tag)
	{
		std::string out(4, '?');
		for (int i = 0; i < 4; ++i)
		{
			const unsigned char c = (unsigned char)((tag >> (24 - i * 8)) & 0xFF);
			out[(std::size_t)i] = (c >= 0x20u && c <= 0x7Eu) ? (char)c : '?';
		}
		return out;
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool Sfnt::parse(const std::uint8_t* data, std::size_t size, std::size_t offset, error_info* error)
	{
		m_valid = false;
		m_version = 0;
		m_tables.clear();

		// Bounds are checked by subtraction, never by `offset + n > size`: on a 32 bit build the
		// addition wraps for a crafted offset and the whole check disappears.
		if (!data || offset > size || size - offset < details::kSfntHeaderBytes)
			return details::fail(error, err_truncated, "face at " + details::hex32((std::uint32_t)offset) + " runs past the end of the file");

		const std::uint8_t* face = data + offset;
		const std::uint32_t version = details::rd_u32(face);
		if (version != details::kSfntTrueType && version != details::kSfntTrue && version != details::kSfntOTTO)
			return details::fail(error, err_bad_face, "face at " + details::hex32((std::uint32_t)offset) +
				" has an unknown sfnt version " + details::hex32(version));

		const std::uint32_t count = details::rd_u16(face + 4);
		if (count == 0 || count > details::kMaxTables)
			return details::fail(error, err_bad_table_count, "face at " + details::hex32((std::uint32_t)offset) +
				" declares " + std::to_string((unsigned long long)count) + " tables, limit is " +
				std::to_string((unsigned long long)details::kMaxTables));

		const std::size_t directory_bytes = details::kSfntHeaderBytes + (std::size_t)count * details::kTableRecordBytes;
		if (directory_bytes > size - offset)
			return details::fail(error, err_truncated, "face at " + details::hex32((std::uint32_t)offset) +
				" table directory runs past the end of the file");

		m_tables.resize(count);
		for (std::uint32_t i = 0; i < count; ++i)
		{
			const std::uint8_t* record = face + details::kSfntHeaderBytes + (std::size_t)i * details::kTableRecordBytes;
			Table_Record& table = m_tables[i];
			table.tag = details::rd_u32(record);
			table.check_sum = details::rd_u32(record + 4);
			table.offset = details::rd_u32(record + 8);
			table.length = details::rd_u32(record + 12);
			if (!details::fits(table.offset, table.length, size))
				return details::fail(error, err_truncated, "face at " + details::hex32((std::uint32_t)offset) +
					" table '" + tag_string(table.tag) + "' runs past the end of the file");
		}

		m_version = version;
		m_valid = true;
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool Sfnt::build(const std::uint8_t* data, std::size_t size,
		std::vector<std::uint8_t>& out, error_info* error) const
	{
		out.clear();
		if (!m_valid)
			return details::fail(error, err_bad_face, "the face directory was not parsed");
		if (!data)
			return details::fail(error, err_truncated, "no bytes to copy the tables from");

		const std::size_t count = m_tables.size();

		// The directory was validated by parse(), but the caller may have handed in different bytes.
		for (std::size_t i = 0; i < count; ++i)
		{
			if (!details::fits(m_tables[i].offset, m_tables[i].length, size))
				return details::fail(error, err_truncated, "table '" + tag_string(m_tables[i].tag) +
					"' runs past the end of the file");
		}

		std::vector<std::uint32_t> offsets(count);
		std::size_t total = details::kSfntHeaderBytes + count * details::kTableRecordBytes;
		for (std::size_t i = 0; i < count; ++i)
		{
			// Check before aligning: align4() would wrap on a 32 bit build for a face close to 4 GiB.
			if (total > 0xFFFFFFFFu - 3u)
				return details::fail(error, err_bad_face, "face does not fit in 4 GiB");
			const std::size_t start = details::align4(total);
			if (m_tables[i].length > 0xFFFFFFFFu - start)
				return details::fail(error, err_bad_face, "face does not fit in 4 GiB");
			offsets[i] = (std::uint32_t)start;
			total = start + m_tables[i].length;
		}
		if (total > 0xFFFFFFFFu - 3u)
			return details::fail(error, err_bad_face, "face does not fit in 4 GiB");
		total = details::align4(total);

		out.assign(total, 0);	// also zeroes the padding between the tables

		std::uint16_t power = 1;
		std::uint16_t log = 0;
		while ((std::uint32_t)power * 2u <= (std::uint32_t)count)
		{
			power = (std::uint16_t)(power * 2u);
			++log;
		}

		details::wr_u32(&out[0], m_version);
		details::wr_u16(&out[4], (std::uint16_t)count);
		details::wr_u16(&out[6], (std::uint16_t)((std::uint32_t)power * 16u));
		details::wr_u16(&out[8], log);
		details::wr_u16(&out[10], (std::uint16_t)((std::uint32_t)count * 16u - (std::uint32_t)power * 16u));

		for (std::size_t i = 0; i < count; ++i)
		{
			const Table_Record& table = m_tables[i];
			std::uint8_t* record = &out[details::kSfntHeaderBytes + i * details::kTableRecordBytes];
			details::wr_u32(record, table.tag);
			details::wr_u32(record + 8, offsets[i]);
			details::wr_u32(record + 12, table.length);
			if (table.length != 0)
				std::memcpy(&out[offsets[i]], data + table.offset, table.length);
		}

		// `head` is hashed with a zero adjustment, then carries the one that makes the file sum up.
		const int head = index_of(details::kTagHead);
		const bool has_head = head >= 0 && m_tables[(std::size_t)head].length >= 12;
		if (has_head)
			details::wr_u32(&out[offsets[(std::size_t)head] + 8], 0);

		for (std::size_t i = 0; i < count; ++i)
		{
			std::uint8_t* record = &out[details::kSfntHeaderBytes + i * details::kTableRecordBytes];
			details::wr_u32(record + 4, checksum(&out[offsets[i]], m_tables[i].length));
		}

		if (has_head)
		{
			const std::uint32_t adjustment = details::kCheckSumMagic - checksum(&out[0], out.size());
			details::wr_u32(&out[offsets[(std::size_t)head] + 8], adjustment);
		}
		return details::pass(error);
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool Sfnt::is_cff() const
	{
		return m_version == details::kSfntOTTO;
	}

	TTC2SFNT_HEAD_ONLY_INLINE int Sfnt::index_of(std::uint32_t tag) const
	{
		for (std::size_t i = 0; i < m_tables.size(); ++i)
		{
			if (m_tables[i].tag == tag)
				return (int)i;
		}
		return -1;
	}

	TTC2SFNT_HEAD_ONLY_INLINE const Table_Record* Sfnt::find(std::uint32_t tag) const
	{
		const int index = index_of(tag);
		return index < 0 ? nullptr : &m_tables[(std::size_t)index];
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool Sfnt::verify(const std::uint8_t* face, std::size_t size, Sfnt_Report* report)
	{
		Sfnt_Report scratch;
		Sfnt_Report& r = report ? *report : scratch;
		r = Sfnt_Report();

		if (!face || size < details::kSfntHeaderBytes)
		{
			r.first_problem = "shorter than a sfnt header";
			return false;
		}

		const std::uint32_t version = details::rd_u32(face);
		if (version != details::kSfntTrueType && version != details::kSfntTrue && version != details::kSfntOTTO)
		{
			r.first_problem = "unknown sfnt version " + details::hex32(version);
			return false;
		}

		const std::uint32_t count = details::rd_u16(face + 4);
		r.table_count = count;
		if (count == 0 || count > details::kMaxTables)
		{
			r.first_problem = "declares " + std::to_string((unsigned long long)count) + " tables, limit is " +
				std::to_string((unsigned long long)details::kMaxTables);
			return false;
		}

		const std::size_t directory_bytes = details::kSfntHeaderBytes + (std::size_t)count * details::kTableRecordBytes;
		if (directory_bytes > size)
		{
			++r.out_of_bounds;
			r.first_problem = "table directory runs past the end of the font";
			return false;
		}

		for (std::uint32_t i = 0; i < count; ++i)
		{
			const std::uint8_t* record = face + details::kSfntHeaderBytes + (std::size_t)i * details::kTableRecordBytes;
			const std::uint32_t tag = details::rd_u32(record);
			const std::uint32_t stored = details::rd_u32(record + 4);
			const std::uint32_t offset = details::rd_u32(record + 8);
			const std::uint32_t length = details::rd_u32(record + 12);
			const std::string name = tag_string(tag);

			if (!details::fits(offset, length, size))
			{
				++r.out_of_bounds;
				if (r.first_problem.empty())
					r.first_problem = "table '" + name + "' at offset " + details::hex32(offset) + " length " +
						std::to_string((unsigned long long)length) + " runs past the end of the font";
				continue;
			}
			if (offset % 4 != 0)
			{
				++r.unaligned;
				if (r.first_problem.empty())
					r.first_problem = "table '" + name + "' starts at " + details::hex32(offset) + ", not a multiple of four";
			}
			if (tag == details::kTagHead)
			{
				if (length >= 16)
					r.head_magic = details::rd_u32(face + offset + 12) == 0x5F0F3CF5u;
				if (!r.head_magic && r.first_problem.empty())
					r.first_problem = "head.magicNumber is missing";
			}

			// The head table is hashed with its checkSumAdjustment zeroed, everything else as stored.
			std::uint32_t sum = 0;
			if (tag == details::kTagHead && length >= 12)
			{
				std::vector<std::uint8_t> payload(face + offset, face + offset + length);
				std::memset(&payload[8], 0, 4);
				sum = checksum(&payload[0], payload.size());
			}
			else
				sum = checksum(face + offset, length);
			if (sum != stored)
			{
				++r.bad_checksums;
				if (r.first_problem.empty())
					r.first_problem = "table '" + name + "' checksum is " + details::hex32(stored) +
						", its payload gives " + details::hex32(sum);
			}
		}

		r.whole_checksum = checksum(face, size);
		r.whole_checksum_ok = r.whole_checksum == details::kCheckSumMagic;
		if (!r.whole_checksum_ok && r.first_problem.empty())
			r.first_problem = "whole font checksum is " + details::hex32(r.whole_checksum) + ", expected " +
				details::hex32(details::kCheckSumMagic);
		if (!r.head_magic && r.first_problem.empty())
			r.first_problem = "the font has no head table";

		return r.ok();
	}
}
