/*!
 * \file ttc_name.cpp
 * \brief The `name` table, and the rules that turn a font name into a usable file name.
 *
 * A `name` record is a platform id, an encoding id, a language id, an id that says what the string
 * means (1 = family, 2 = subfamily, ...), and a range inside the table's string store. The store is
 * UTF-16BE for the Microsoft platform and Latin-1 for the Macintosh one; this file decodes both to
 * UTF-8 and picks the most useful record, so that the extractor can name files after the font
 * instead of after its index.
 */

#include "ttc2sfnt/ttc_name.h"
#include "ttc2sfnt/common.h"

namespace ttc2sfnt
{
	namespace details
	{
		TTC2SFNT_HEAD_ONLY_INLINE std::string utf16be_to_utf8(const std::uint8_t* data, std::size_t size)
		{
			std::string out;
			out.reserve(size / 2);
			std::size_t i = 0;
			while (i + 1 < size)
			{
				std::uint32_t cp = details::rd_u16(data + i);
				i += 2;
				if (cp >= 0xD800u && cp <= 0xDBFFu)
				{
					if (i + 1 < size)
					{
						const std::uint32_t low = details::rd_u16(data + i);
						if (low >= 0xDC00u && low <= 0xDFFFu)
						{
							i += 2;
							cp = 0x10000u + ((cp - 0xD800u) << 10) + (low - 0xDC00u);
						}
						else
							cp = 0xFFFDu;
					}
					else
						cp = 0xFFFDu;
				}
				else if (cp >= 0xDC00u && cp <= 0xDFFFu)
					cp = 0xFFFDu;
				details::append_utf8(out, cp);
			}
			return out;
		}

		TTC2SFNT_HEAD_ONLY_INLINE std::string latin1_to_utf8(const std::uint8_t* data, std::size_t size)
		{
			std::string out;
			out.reserve(size);
			for (std::size_t i = 0; i < size; ++i)
				details::append_utf8(out, data[i]);
			return out;
		}

		TTC2SFNT_HEAD_ONLY_INLINE bool name_string(const std::uint8_t* data, const Sfnt& sfnt,
			std::uint16_t name_id, std::string& out)
		{
			const Table_Record* table = sfnt.find(details::kTagName);
			if (!table || table->length < 6)
				return false;

			const std::uint8_t* name = data + table->offset;
			const std::uint16_t count = details::rd_u16(name + 2);
			const std::uint16_t storage = details::rd_u16(name + 4);
			if (6u + (std::size_t)count * 12u > table->length)
				return false;

			int best_score = 1000;
			std::string best;
			for (std::uint16_t i = 0; i < count; ++i)
			{
				const std::uint8_t* record = name + 6 + (std::size_t)i * 12;
				const std::uint16_t platform = details::rd_u16(record);
				const std::uint16_t encoding = details::rd_u16(record + 2);
				const std::uint16_t language = details::rd_u16(record + 4);
				const std::uint16_t id = details::rd_u16(record + 6);
				const std::uint16_t length = details::rd_u16(record + 8);
				const std::uint16_t offset = details::rd_u16(record + 10);
				if (id != name_id)
					continue;
				if ((std::size_t)storage + offset + length > table->length)
					continue;

				int score = 0;
				if (platform == 3)
					score = (encoding == 1 || encoding == 10) ? 0 : 1;
				else if (platform == 0)
					score = 1;
				else if (platform == 1)
					score = 3;
				else
					continue;
				if (platform == 3 && language != 0x0409u)
					++score;
				if (score >= best_score)
					continue;

				const std::uint8_t* text = name + storage + offset;
				best_score = score;
				best = (platform == 1) ? latin1_to_utf8(text, length) : utf16be_to_utf8(text, length);
			}

			if (best_score == 1000)
				return false;
			out = best;
			return true;
		}

	}

	TTC2SFNT_HEAD_ONLY_INLINE std::string sanitize_file_name(const std::string& name)
	{
		const std::size_t kMaxBytes = 120;

		std::string out;
		out.reserve(name.size());
		for (std::size_t i = 0; i < name.size(); ++i)
		{
			const unsigned char c = (unsigned char)name[i];
			if (c < 0x20u || c == 0x7Fu)
				continue;
			if (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' ||
				c == '|' || c == '?' || c == '*')
				out.push_back('_');
			else
				out.push_back((char)c);
		}

		std::size_t begin = 0;
		std::size_t end = out.size();
		while (begin < end && (out[begin] == ' ' || out[begin] == '.'))
			++begin;
		while (end > begin && (out[end - 1] == ' ' || out[end - 1] == '.'))
			--end;
		out = out.substr(begin, end - begin);

		if (out.size() > kMaxBytes)
		{
			std::size_t cut = kMaxBytes;
			while (cut > 0 && ((unsigned char)out[cut] & 0xC0u) == 0x80u)
				--cut;
			out.resize(cut);
			while (!out.empty() && (out[out.size() - 1] == ' ' || out[out.size() - 1] == '.'))
				out.erase(out.size() - 1);
		}
		return out;
	}

	TTC2SFNT_HEAD_ONLY_INLINE bool same_file_name(const std::string& a, const std::string& b)
	{
		if (a.size() != b.size())
			return false;
		for (std::size_t i = 0; i < a.size(); ++i)
		{
			if (details::ascii_lower((unsigned char)a[i]) != details::ascii_lower((unsigned char)b[i]))
				return false;
		}
		return true;
	}
}
