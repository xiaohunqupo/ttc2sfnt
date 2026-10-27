#pragma once

/*!
 * \file ttc_name.h
 * \brief The `name` table: family / subfamily as UTF-8, and the rules that turn them into a file
 *        name a file system accepts.
 */

#include "ttc2sfnt/config.h"
#include "ttc2sfnt/ttc_sfnt.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace ttc2sfnt
{
	/*!
	 * \brief Turn a font name into a file name a file system accepts.
	 *
	 * Control characters are dropped, `< > : " / \ | ? *` become `_`, leading and trailing spaces and
	 * dots are trimmed, and the result is capped at 120 bytes without cutting a UTF-8 sequence in
	 * half.
	 *
	 * \param name Font name, UTF-8.
	 * \return The sanitised name, empty when nothing usable was left.
	 */
	std::string sanitize_file_name(const std::string& name);

	/*!
	 * \brief Compare two file names the way Windows does.
	 * \return `true` when both names are equal ignoring ASCII case.
	 */
	bool same_file_name(const std::string& a, const std::string& b);

	namespace details
	{
		/*!
		 * \brief Decode big endian UTF-16, the encoding of the Windows `name` strings.
		 * \param data UTF-16BE bytes.
		 * \param size Byte count; a trailing odd byte is ignored.
		 * \return UTF-8 text; unpaired surrogates become `U+FFFD`, so the result is always valid
		 *         UTF-8.
		 */
		std::string utf16be_to_utf8(const std::uint8_t* data, std::size_t size);

		//! Decode Latin-1, the encoding of the Macintosh `name` strings.
		std::string latin1_to_utf8(const std::uint8_t* data, std::size_t size);

		/*!
		 * \brief Read one record of the `name` table.
		 *
		 * Preference order: Microsoft platform with encoding 1 or 10 (UTF-16BE), then the Unicode
		 * platform, then Macintosh (Latin-1); inside the Microsoft platform the en-US record wins. A
		 * record whose string would point outside the table is skipped instead of read.
		 *
		 * \param data Collection bytes.
		 * \param sfnt Directory of the face the string belongs to.
		 * \param name_id 1 for the family, 2 for the subfamily, 4 for the full name, ...
		 * \param [out] out Receives the UTF-8 text.
		 * \return `false` when the face has no `name` table or no record with that id.
		 */
		bool name_string(const std::uint8_t* data, const Sfnt& sfnt, std::uint16_t name_id, std::string& out);
	}
}

#ifdef TTC2SFNT_HEAD_ONLY
#include "ttc2sfnt/ttc_name.cpp"
#endif // TTC2SFNT_HEAD_ONLY
