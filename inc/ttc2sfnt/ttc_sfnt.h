#pragma once

/*!
 * \file ttc_sfnt.h
 * \brief One face of a collection: its table directory, and the rebuild that makes it standalone.
 *
 * A face already is a complete sfnt, except that its table data is stored somewhere else in the
 * collection and shared with the other faces. `parse()` records where every table lives, `build()`
 * copies the payloads into a new buffer, rewrites the directory with the new offsets and fixes
 * `head.checkSumAdjustment` so the result sums to `0xB1B0AFBA`.
 */

#include "ttc2sfnt/config.h"
#include "ttc2sfnt/ttc_error.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ttc2sfnt
{
	/*!
	 * \brief One entry of the sfnt table directory.
	 */
	struct Table_Record
	{
		std::uint32_t tag;			//!< Four character tag, e.g. `'head'`.
		std::uint32_t check_sum;	//!< Checksum stored in the file; `build()` recomputes it.
		std::uint32_t offset;		//!< Absolute offset inside the collection.
		std::uint32_t length;		//!< Payload length, before the four byte alignment.
	};

	/*!
	 * \brief What `Sfnt::verify()` found in a standalone font.
	 *
	 * Every counter is zero and both flags are `true` for a usable font; `first_problem` carries the
	 * first thing that was wrong, in printable form, so a failure can be logged without a formatter.
	 */
	struct Sfnt_Report
	{
		std::uint32_t table_count = 0;		//!< Number of tables the directory declares.
		std::uint32_t out_of_bounds = 0;	//!< Records or payloads that do not fit in the font.
		std::uint32_t unaligned = 0;		//!< Table payloads not starting on a four byte boundary.
		std::uint32_t bad_checksums = 0;	//!< Payloads that do not match their stored table checksum.
		bool head_magic = false;			//!< `head.magicNumber` is `0x5F0F3CF5` (a face needs one).
		bool whole_checksum_ok = false;		//!< The whole font sums to `0xB1B0AFBA`.
		std::uint32_t whole_checksum = 0;	//!< The sum that was actually computed.
		std::string first_problem;			//!< Empty when `ok()`.

		bool ok() const
		{
			return out_of_bounds == 0 && unaligned == 0 && bad_checksums == 0 &&
				head_magic && whole_checksum_ok;
		}
	};

	/*!
	 * \brief The table directory of one face inside a collection.
	 *
	 * An instance only describes a face; it borrows the collection bytes and copies nothing. Build
	 * the standalone font with `build()`.
	 */
	class Sfnt
	{
	public:
		//! Tags the extractor itself looks at.
		enum
		{
			tag_head = 0x68656164,	//!< 'head', carries `checkSumAdjustment`
			tag_name = 0x6E616D65,	//!< 'name', carries the family and subfamily
			tag_maxp = 0x6D617870	//!< 'maxp', carries `numGlyphs`
		};

		Sfnt();

		/*!
		 * \brief Sum of the big endian 32 bit words of a range, its tail zero padded.
		 * \param data Bytes to sum, may be `nullptr` when `size` is zero.
		 * \param size Byte count, the last partial word counts as if it were padded with zeros.
		 * \return The checksum, the unit every sfnt table and whole file checksum is built from.
		 */
		static std::uint32_t checksum(const std::uint8_t* data, std::size_t size);

		/*!
		 * \brief Four character tag as text.
		 * \param tag Tag value.
		 * \return Printable characters of the tag, `'?'` for bytes outside `0x20..0x7E`.
		 */
		static std::string tag_string(std::uint32_t tag);

		/*!
		 * \brief Parse and validate the table directory of one face.
		 *
		 * Every field that can point outside the collection is checked here: the header, the
		 * directory itself and the payload of every table.
		 *
		 * \param data Collection bytes.
		 * \param size Byte count of `data`.
		 * \param offset Where the face header starts inside the collection.
		 * \param [out] error Optional diagnostic.
		 * \return `false` and an untouched directory when the face is not a usable sfnt.
		 */
		bool parse(const std::uint8_t* data, std::size_t size, std::size_t offset, error_info* error = nullptr);

		/*!
		 * \brief Rebuild this face as a standalone sfnt.
		 *
		 * Table data is copied verbatim, table records get the new offsets, every table checksum is
		 * recomputed, and `head.checkSumAdjustment` is written so that the whole output sums to
		 * `0xB1B0AFBA`. Padding between the tables stays zero, so every table is four byte aligned.
		 *
		 * \param data Collection bytes the directory was parsed from.
		 * \param size Byte count of `data`.
		 * \param [out] out Receives the standalone font; cleared when the call fails.
		 * \param [out] error Optional diagnostic.
		 */
		bool build(const std::uint8_t* data, std::size_t size, std::vector<std::uint8_t>& out, error_info* error = nullptr) const;

		bool valid() const { return m_valid; }
		std::uint32_t version() const { return m_version; }				//!< `0x00010000`, `'true'` or `'OTTO'`.
		bool is_cff() const;											//!< `true` for `'OTTO'`, i.e. CFF outlines.
		std::size_t table_count() const { return m_tables.size(); }
		const Table_Record& table(std::size_t index) const { return m_tables[index]; }

		/*!
		 * \brief Look a table up by tag.
		 * \return Index usable with `table()`, or `-1` when the face has no such table.
		 */
		int index_of(std::uint32_t tag) const;

		/*!
		 * \brief Look a table up by tag.
		 * \return The record, or `nullptr` when the face has no such table.
		 */
		const Table_Record* find(std::uint32_t tag) const;

		/*!
		 * \brief Check a standalone font (the bytes `build()` produces, or any `.ttf` / `.otf`).
		 *
		 * Recomputes every table checksum from the payload, checks the four byte alignment of the
		 * tables, `head.magicNumber`, and that the whole font sums to `0xB1B0AFBA`. Copies nothing
		 * except the `head` payload (54 bytes), so it is cheap enough to call in a debug build or a
		 * self test.
		 *
		 * This is the guard for the one dangerous pattern in the calling convention: some consumers
		 * take a `const std::uint8_t*` and write into it anyway. `ttf2mesh`'s `ttf_load_from_mem()`
		 * converts the table directory and several header structures to host byte order in place, so
		 * a face handed to it is afterwards neither a valid big endian font nor a consistent little
		 * endian one - and that cannot be repaired, because which bytes were converted depends on the
		 * consumer's parse path. Hand it a copy (`with_scratch_copy()`) or re-extract the face; if a
		 * buffer may have been near such a consumer, `verify()` says whether it is still sound.
		 *
		 * \param face Bytes of the standalone font.
		 * \param size Byte count.
		 * \param [out] report Optional detail, always filled as far as the checks got.
		 * \return `true` when `report->ok()` (or, without a report, when nothing is wrong).
		 */
		static bool verify(const std::uint8_t* face, std::size_t size, Sfnt_Report* report = nullptr);

	private:
		bool m_valid;
		std::uint32_t m_version;
		std::vector<Table_Record> m_tables;
	};

	/*!
	 * \brief Hand a throwaway copy of `bytes` to a consumer that may write through its pointer.
	 *
	 * `ttf2mesh` is the reason this exists: `ttf_load_from_mem()` promises a `const std::uint8_t*`
	 * and then converts the table directory and several header structures to host byte order in
	 * place, which leaves the caller's buffer in a mixed endian state that nothing can repair. The
	 * only safe pattern is to give such a consumer a copy nobody else holds:
	 *
	 * \code
	 * const int rc = ttc2sfnt::with_scratch_copy(face, [&](const std::uint8_t* data, std::size_t size)
	 * {
	 *     return ttf_load_from_mem(data, (int)size, &ttf, false);
	 * });
	 * // `face` is untouched here, so it can still be written to disk or used elsewhere.
	 * \endcode
	 *
	 * The copy costs one `memcpy` of the face and is released when the consumer returns.
	 *
	 * \param bytes Buffer to copy.
	 * \param consumer Callable taking `(const std::uint8_t*, std::size_t)`.
	 * \return Whatever the consumer returns.
	 */
	template <typename Consumer>
	auto with_scratch_copy(const std::vector<std::uint8_t>& bytes, Consumer consumer)
		-> decltype(consumer(static_cast<const std::uint8_t*>(nullptr), std::size_t(0)))
	{
		const std::vector<std::uint8_t> scratch = bytes;
		return consumer(scratch.empty() ? static_cast<const std::uint8_t*>(nullptr) : &scratch[0], scratch.size());
	}
}

#ifdef TTC2SFNT_HEAD_ONLY
#include "ttc2sfnt/ttc_sfnt.cpp"
#endif // TTC2SFNT_HEAD_ONLY
