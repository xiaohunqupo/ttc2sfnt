#pragma once
#ifndef TTC2SFNT_COMMON_H
#define TTC2SFNT_COMMON_H

/*!
 * \file common.h
 * \brief Internal helpers shared by the sources: byte order, limits, checksums and small string
 *        utilities.
 *
 * This header belongs to `src` and is not part of the public interface. It is included by the
 * sources only, so nothing here has to stay binary compatible. In the header only build the sources
 * are pulled into the public headers, which means this file reaches consumer translation units as
 * well - hence the project prefixed include guard, which no other library is expected to claim.
 */

#include "ttc2sfnt/ttc2sfnt_config.h"
#include "ttc2sfnt/ttc_error.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace ttc2sfnt
{
	namespace details
	{
		/* ------------------------------------------------------------------ bytes */

		//! Big endian 16 bit read.
		inline std::uint16_t rd_u16(const std::uint8_t* p)
		{
			return (std::uint16_t)(((std::uint16_t)p[0] << 8) | (std::uint16_t)p[1]);
		}

		//! Big endian 32 bit read.
		inline std::uint32_t rd_u32(const std::uint8_t* p)
		{
			return ((std::uint32_t)p[0] << 24) | ((std::uint32_t)p[1] << 16) |
				((std::uint32_t)p[2] << 8) | (std::uint32_t)p[3];
		}

		//! Big endian 16 bit write.
		inline void wr_u16(std::uint8_t* p, std::uint16_t value)
		{
			p[0] = (std::uint8_t)((value >> 8) & 0xFF);
			p[1] = (std::uint8_t)(value & 0xFF);
		}

		//! Big endian 32 bit write.
		inline void wr_u32(std::uint8_t* p, std::uint32_t value)
		{
			p[0] = (std::uint8_t)((value >> 24) & 0xFF);
			p[1] = (std::uint8_t)((value >> 16) & 0xFF);
			p[2] = (std::uint8_t)((value >> 8) & 0xFF);
			p[3] = (std::uint8_t)(value & 0xFF);
		}

		//! Round up to the four byte alignment sfnt requires for every table.
		inline std::size_t align4(std::size_t value)
		{
			return (value + 3u) & ~(std::size_t)3u;
		}

		/*!
		 * \brief `true` when `[offset, offset + length)` lies inside `size`.
		 * \note The comparison is done by subtraction, so a crafted `offset` cannot wrap the sum and
		 *       silently disable the check (which is what `offset + length > size` does on a 32 bit
		 *       build).
		 */
		inline bool fits(std::size_t offset, std::size_t length, std::size_t size)
		{
			return offset <= size && length <= size - offset;
		}

		//! Hex dump of a 32 bit value, used in error messages.
		inline std::string hex32(std::uint32_t value)
		{
			char buffer[16];
			std::snprintf(buffer, sizeof(buffer), "0x%08X", (unsigned int)value);
			return std::string(buffer);
		}

		/* -------------------------------------------------------------- constants */

		const std::uint32_t kTagTTCF = 0x74746366u;			//!< 'ttcf'
		const std::uint32_t kSfntTrueType = 0x00010000u;	//!< TrueType outlines
		const std::uint32_t kSfntTrue = 0x74727565u;		//!< 'true', old Apple sfnt
		const std::uint32_t kSfntOTTO = 0x4F54544Fu;		//!< 'OTTO', CFF outlines
		const std::uint32_t kTagHead = 0x68656164u;			//!< 'head'
		const std::uint32_t kTagName = 0x6E616D65u;			//!< 'name'
		const std::uint32_t kTagMaxp = 0x6D617870u;			//!< 'maxp'
		const std::uint32_t kCheckSumMagic = 0xB1B0AFBAu;	//!< whole file checksum of a valid sfnt

		const std::size_t kHeaderBytes = 12;				//!< 'ttcf' + version + face count
		const std::size_t kSfntHeaderBytes = 12;			//!< sfnt version + table count + search fields
		const std::size_t kTableRecordBytes = 16;			//!< tag + checksum + offset + length

		const std::uint32_t kMaxTables = 4096u;				//!< rejected above this
		const std::uint32_t kMaxFaces = 1024u;				//!< rejected above this

		/* ----------------------------------------------------------------- errors */

		//! Record a failure and return `false`, so call sites can `return fail(...)`.
		inline bool fail(error_info* error, int code, const std::string& message)
		{
			if (error)
			{
				error->code = code;
				error->message = message;
			}
			return false;
		}

		//! Clear the diagnostic and return `true`, so call sites can `return pass(...)`.
		inline bool pass(error_info* error)
		{
			if (error)
			{
				error->code = err_none;
				error->message.clear();
			}
			return true;
		}

		/* ------------------------------------------------------------------ text */

		//! Append one code point as UTF-8.
		inline void append_utf8(std::string& out, std::uint32_t cp)
		{
			if (cp < 0x80u)
			{
				out.push_back((char)cp);
			}
			else if (cp < 0x800u)
			{
				out.push_back((char)(0xC0u | (cp >> 6)));
				out.push_back((char)(0x80u | (cp & 0x3Fu)));
			}
			else if (cp < 0x10000u)
			{
				out.push_back((char)(0xE0u | (cp >> 12)));
				out.push_back((char)(0x80u | ((cp >> 6) & 0x3Fu)));
				out.push_back((char)(0x80u | (cp & 0x3Fu)));
			}
			else
			{
				out.push_back((char)(0xF0u | (cp >> 18)));
				out.push_back((char)(0x80u | ((cp >> 12) & 0x3Fu)));
				out.push_back((char)(0x80u | ((cp >> 6) & 0x3Fu)));
				out.push_back((char)(0x80u | (cp & 0x3Fu)));
			}
		}

		//! ASCII only lower case, enough for case insensitive file name comparison.
		inline int ascii_lower(int c)
		{
			return (c >= 'A' && c <= 'Z') ? c + 32 : c;
		}

		//! Two digit index, the suffix of a fallback file name.
		inline std::string number_suffix(std::size_t index)
		{
			std::string digits = std::to_string((unsigned long long)index);
			if (digits.size() < 2)
				digits.insert(digits.begin(), '0');
			return digits;
		}
	}
}

#endif // TTC2SFNT_COMMON_H
