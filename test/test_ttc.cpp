// ttc2sfnt regression test.
//
// Self contained: no framework, no test data in the repository. Synthetic
// collections are built byte by byte, and a real `.ttc` is exercised when one
// can be found (env TTC2SFNT_TEST_FONT, then fonts/simsun.ttc, then the
// Windows font directory).
//
// The checks never call ttc2sfnt internals: the source collection and every
// produced face are parsed by an independent reader below, so that a bug in the
// library cannot hide behind the same bug in the test.

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#if defined(_WIN32)
#	include <direct.h>
#else
#	include <unistd.h>
#endif

#include "ttc2sfnt/ttc2sfnt.h"

/* ------------------------------------------------------------------ harness */

static int g_checks = 0;
static int g_failures = 0;

static void remove_dir(const std::string& path)
{
#if defined(_WIN32)
	::_rmdir(path.c_str());
#else
	::rmdir(path.c_str());
#endif
}

static void check_impl(bool condition, const char* expression, const char* file, int line)
{
	++g_checks;
	if (condition)
		return;
	++g_failures;
	std::printf("  FAIL %s:%d  %s\n", file, line, expression);
}

#define CHECK(expr) check_impl((expr), #expr, __FILE__, __LINE__)
#define CHECK_MSG(expr, msg) do { if (!(expr)) { std::printf("  note: %s\n", std::string(msg).c_str()); } check_impl((expr), #expr, __FILE__, __LINE__); } while (0)

static void section(const char* name)
{
	std::printf("[%s]\n", name);
}

/* ------------------------------------------------------- independent reader */

namespace ref
{
	typedef std::vector<std::uint8_t> Bytes;

	std::uint16_t be16(const Bytes& v, std::size_t off)
	{
		return (std::uint16_t)(((std::uint16_t)v[off] << 8) | v[off + 1]);
	}

	std::uint32_t be32(const Bytes& v, std::size_t off)
	{
		return ((std::uint32_t)v[off] << 24) | ((std::uint32_t)v[off + 1] << 16) |
			((std::uint32_t)v[off + 2] << 8) | (std::uint32_t)v[off + 3];
	}

	std::uint32_t sum32(const std::uint8_t* data, std::size_t size)
	{
		std::uint32_t sum = 0;
		std::size_t i = 0;
		for (; i + 4 <= size; i += 4)
		{
			sum += ((std::uint32_t)data[i] << 24) | ((std::uint32_t)data[i + 1] << 16) |
				((std::uint32_t)data[i + 2] << 8) | (std::uint32_t)data[i + 3];
		}
		if (i < size)
		{
			std::uint32_t tail = 0;
			for (std::size_t k = 0; k < 4; ++k)
			{
				tail <<= 8;
				if (i + k < size)
					tail |= data[i + k];
			}
			sum += tail;
		}
		return sum;
	}

	struct Table
	{
		std::uint32_t tag;
		Bytes data;
	};

	struct Face
	{
		std::uint32_t sfnt;
		std::vector<Table> tables;

		const Table* find(std::uint32_t tag) const
		{
			for (std::size_t i = 0; i < tables.size(); ++i)
			{
				if (tables[i].tag == tag)
					return &tables[i];
			}
			return nullptr;
		}
	};

	//! @return false when the face is structurally broken.
	bool parse_sfnt(const Bytes& bytes, Face& out, std::string& why)
	{
		out.tables.clear();
		if (bytes.size() < 12)
		{
			why = "shorter than a sfnt header";
			return false;
		}
		out.sfnt = be32(bytes, 0);
		const std::uint16_t num_tables = be16(bytes, 4);
		if (num_tables == 0 || 12 + (std::size_t)num_tables * 16 > bytes.size())
		{
			why = "table directory out of bounds";
			return false;
		}
		for (std::uint16_t i = 0; i < num_tables; ++i)
		{
			const std::size_t record = 12 + (std::size_t)i * 16;
			Table table;
			table.tag = be32(bytes, record);
			const std::uint32_t expected = be32(bytes, record + 4);
			const std::uint32_t offset = be32(bytes, record + 8);
			const std::uint32_t length = be32(bytes, record + 12);
			if ((std::size_t)offset + length > bytes.size())
			{
				why = "table data out of bounds";
				return false;
			}
			if (offset % 4 != 0)
			{
				why = "table offset is not 4 byte aligned";
				return false;
			}
			table.data.assign(bytes.begin() + offset, bytes.begin() + offset + length);

			Bytes checksum_me = table.data;
			if (table.tag == 0x68656164u && checksum_me.size() >= 12)
				std::memset(&checksum_me[8], 0, 4);	// head is hashed with a zero adjustment
			if (sum32(checksum_me.empty() ? nullptr : &checksum_me[0], checksum_me.size()) != expected)
			{
				why = "table checksum mismatch";
				return false;
			}
			out.tables.push_back(table);
		}
		return true;
	}

	//! Reads a whole collection and every face in it.
	bool parse_ttc(const Bytes& bytes, std::uint32_t& version, std::vector<Face>& faces, std::string& why)
	{
		faces.clear();
		version = 0;
		if (bytes.size() < 12)
		{
			why = "shorter than a ttc header";
			return false;
		}
		if (be32(bytes, 0) != 0x74746366u)
		{
			why = "not a ttc";
			return false;
		}
		version = be32(bytes, 4);
		const std::uint32_t count = be32(bytes, 8);
		if (count == 0 || 12 + (std::size_t)count * 4 > bytes.size())
		{
			why = "face offset array out of bounds";
			return false;
		}
		for (std::uint32_t i = 0; i < count; ++i)
		{
			const std::uint32_t offset = be32(bytes, 12 + (std::size_t)i * 4);
			if ((std::size_t)offset + 12 > bytes.size())
			{
				why = "face out of bounds";
				return false;
			}
			const std::uint16_t num_tables = be16(bytes, offset + 4);
			if (12 + (std::size_t)num_tables * 16 > bytes.size() - offset)
			{
				why = "face directory out of bounds";
				return false;
			}
			Face face;
			face.sfnt = be32(bytes, offset);
			for (std::uint16_t t = 0; t < num_tables; ++t)
			{
				const std::size_t record = offset + 12 + (std::size_t)t * 16;
				Table table;
				table.tag = be32(bytes, record);
				const std::uint32_t data_offset = be32(bytes, record + 8);
				const std::uint32_t length = be32(bytes, record + 12);
				if ((std::size_t)data_offset + length > bytes.size())
				{
					why = "table out of bounds";
					return false;
				}
				table.data.assign(bytes.begin() + data_offset, bytes.begin() + data_offset + length);
				face.tables.push_back(table);
			}
			faces.push_back(face);
		}
		return true;
	}

	//! Whole-file checksum, the value every valid sfnt must sum to.
	bool whole_checksum_ok(const Bytes& bytes)
	{
		return sum32(bytes.empty() ? nullptr : &bytes[0], bytes.size()) == 0xB1B0AFBAu;
	}
}

/* --------------------------------------------------------- fixture builder */

namespace fixture
{
	typedef std::vector<std::uint8_t> Bytes;

	void put16(Bytes& v, std::uint16_t value)
	{
		v.push_back((std::uint8_t)(value >> 8));
		v.push_back((std::uint8_t)(value & 0xFF));
	}

	void put32(Bytes& v, std::uint32_t value)
	{
		v.push_back((std::uint8_t)(value >> 24));
		v.push_back((std::uint8_t)(value >> 16));
		v.push_back((std::uint8_t)(value >> 8));
		v.push_back((std::uint8_t)(value & 0xFF));
	}

	std::uint32_t tag(const char* text)
	{
		return ((std::uint32_t)(std::uint8_t)text[0] << 24) | ((std::uint32_t)(std::uint8_t)text[1] << 16) |
			((std::uint32_t)(std::uint8_t)text[2] << 8) | (std::uint32_t)(std::uint8_t)text[3];
	}

	Bytes make_head()
	{
		Bytes head;
		put32(head, 0x00010000u);		// version
		put32(head, 0x00010000u);		// fontRevision
		put32(head, 0x12345678u);		// checkSumAdjustment, deliberately bogus
		put32(head, 0x5F0F3CF5u);		// magicNumber
		put16(head, 0x000Bu);			// flags
		put16(head, 1000);				// unitsPerEm
		put32(head, 0); put32(head, 0);	// created
		put32(head, 0); put32(head, 0);	// modified
		put16(head, 0); put16(head, 0);	// xMin, yMin
		put16(head, 1000); put16(head, 1000);	// xMax, yMax
		put16(head, 0);					// macStyle
		put16(head, 8);					// lowestRecPPEM
		put16(head, 2);					// fontDirectionHint
		put16(head, 0);					// indexToLocFormat
		put16(head, 0);					// glyphDataFormat
		return head;
	}

	Bytes make_maxp(std::uint16_t glyphs)
	{
		Bytes maxp;
		put32(maxp, 0x00010000u);
		put16(maxp, glyphs);
		for (int i = 0; i < 13; ++i)
			put16(maxp, 0);				// the rest of the 1.0 table
		return maxp;
	}

	Bytes make_name(const std::string& family, const std::string& subfamily)
	{
		// Two Windows platform records, UTF-16BE strings.
		Bytes strings;
		const std::string values[2] = { family, subfamily };
		for (int i = 0; i < 2; ++i)
		{
			for (std::size_t k = 0; k < values[i].size(); ++k)
				put16(strings, (std::uint16_t)(std::uint8_t)values[i][k]);	// UTF-16BE, ASCII only here
		}

		Bytes name;
		put16(name, 0);					// format
		put16(name, 2);					// count
		put16(name, 6 + 2 * 12);		// stringOffset
		std::uint16_t offset = 0;
		for (int i = 0; i < 2; ++i)
		{
			const std::uint16_t length = (std::uint16_t)(values[i].size() * 2);
			put16(name, 3);				// platformID: Microsoft
			put16(name, 1);				// encodingID: Unicode BMP
			put16(name, 0x0409);		// languageID: en-US
			put16(name, (std::uint16_t)(1 + i));	// nameID 1 = family, 2 = subfamily
			put16(name, length);
			put16(name, offset);
			offset = (std::uint16_t)(offset + length);
		}
		name.insert(name.end(), strings.begin(), strings.end());
		return name;
	}

	Bytes make_glyf()
	{
		// Odd length on purpose: the tail has to be padded to four bytes.
		Bytes glyf;
		glyf.push_back(0x00);
		glyf.push_back(0x01);
		glyf.push_back(0x00);
		glyf.push_back(0x00);
		glyf.push_back(0xFF);
		return glyf;
	}

	struct FaceSpec
	{
		std::string family;
		std::string subfamily;
		std::uint16_t glyphs;
		bool cff;			//!< 'OTTO' instead of 0x00010000
		bool share_tables;	//!< reuse another face's tables byte for byte
		bool empty_name;	//!< no usable `name` table, so the file name falls back
	};

	//! Build a structurally valid collection out of a handful of tiny tables.
	Bytes make_ttc(std::uint32_t version, const std::vector<FaceSpec>& specs)
	{
		struct PooledTable
		{
			std::uint32_t tag;
			Bytes data;
			std::uint32_t offset;
		};

		std::vector<PooledTable> pool;
		std::vector<std::vector<Bytes> > face_bytes(specs.size());

		for (std::size_t f = 0; f < specs.size(); ++f)
		{
			Bytes head = make_head();
			Bytes maxp = make_maxp(specs[f].glyphs);
			Bytes name;
			if (specs[f].empty_name)
			{
				put16(name, 0);		// format 0
				put16(name, 0);		// no records
				put16(name, 6);		// storage starts right after the records
			}
			else
				name = make_name(specs[f].family, specs[f].subfamily);
			Bytes glyf = make_glyf();
			Bytes cmap;
			put32(cmap, 0);				// opaque to us

			face_bytes[f].push_back(head);
			face_bytes[f].push_back(maxp);
			face_bytes[f].push_back(name);
			face_bytes[f].push_back(glyf);
			face_bytes[f].push_back(cmap);
		}

		const std::uint32_t tags[5] =
		{
			tag("head"), tag("maxp"), tag("name"), tag("glyf"), tag("cmap")
		};

		// Collect the table pool, sharing identical (tag, bytes) pairs.
		std::vector<std::vector<std::uint32_t> > face_table_index(specs.size());
		for (std::size_t f = 0; f < specs.size(); ++f)
		{
			for (std::size_t t = 0; t < 5; ++t)
			{
				std::uint32_t found = 0xFFFFFFFFu;
				if (specs[f].share_tables)
				{
					for (std::size_t p = 0; p < pool.size(); ++p)
					{
						if (pool[p].tag == tags[t] && pool[p].data == face_bytes[f][t])
						{
							found = (std::uint32_t)p;
							break;
						}
					}
				}
				if (found == 0xFFFFFFFFu)
				{
					PooledTable pooled;
					pooled.tag = tags[t];
					pooled.data = face_bytes[f][t];
					pooled.offset = 0;
					found = (std::uint32_t)pool.size();
					pool.push_back(pooled);
				}
				face_table_index[f].push_back(found);
			}
		}

		const std::size_t header_bytes = 12 + specs.size() * 4 + (version == 0x00020000u ? 12u : 0u);
		std::size_t position = header_bytes;
		std::vector<std::uint32_t> face_offset(specs.size());
		for (std::size_t f = 0; f < specs.size(); ++f)
		{
			face_offset[f] = (std::uint32_t)position;
			position += 12 + 5 * 16;
		}
		for (std::size_t p = 0; p < pool.size(); ++p)
		{
			pool[p].offset = (std::uint32_t)position;
			position += (pool[p].data.size() + 3u) & ~(std::size_t)3u;
		}

		Bytes out(position, 0);
		out[0] = 't'; out[1] = 't'; out[2] = 'c'; out[3] = 'f';
		out[4] = (std::uint8_t)(version >> 24); out[5] = (std::uint8_t)(version >> 16);
		out[6] = (std::uint8_t)(version >> 8); out[7] = (std::uint8_t)version;
		const std::uint32_t count = (std::uint32_t)specs.size();
		out[8] = (std::uint8_t)(count >> 24); out[9] = (std::uint8_t)(count >> 16);
		out[10] = (std::uint8_t)(count >> 8); out[11] = (std::uint8_t)count;
		for (std::size_t f = 0; f < specs.size(); ++f)
		{
			const std::uint32_t offset = face_offset[f];
			out[12 + f * 4] = (std::uint8_t)(offset >> 24);
			out[13 + f * 4] = (std::uint8_t)(offset >> 16);
			out[14 + f * 4] = (std::uint8_t)(offset >> 8);
			out[15 + f * 4] = (std::uint8_t)offset;
		}

		for (std::size_t f = 0; f < specs.size(); ++f)
		{
			const std::size_t base = face_offset[f];
			const std::uint32_t sfnt = specs[f].cff ? 0x4F54544Fu : 0x00010000u;
			out[base + 0] = (std::uint8_t)(sfnt >> 24); out[base + 1] = (std::uint8_t)(sfnt >> 16);
			out[base + 2] = (std::uint8_t)(sfnt >> 8); out[base + 3] = (std::uint8_t)sfnt;
			out[base + 4] = 0; out[base + 5] = 5;		// numTables
			out[base + 6] = 0; out[base + 7] = 64;		// searchRange
			out[base + 8] = 0; out[base + 9] = 2;		// entrySelector
			out[base + 10] = 0; out[base + 11] = 16;	// rangeShift

			for (std::size_t t = 0; t < 5; ++t)
			{
				const PooledTable& pooled = pool[face_table_index[f][t]];
				ref::Bytes hashed = pooled.data;
				if (pooled.tag == tag("head") && hashed.size() >= 12)
					std::memset(&hashed[8], 0, 4);
				const std::uint32_t checksum = ref::sum32(hashed.empty() ? nullptr : &hashed[0], hashed.size());

				const std::size_t record = base + 12 + t * 16;
				out[record + 0] = (std::uint8_t)(pooled.tag >> 24);
				out[record + 1] = (std::uint8_t)(pooled.tag >> 16);
				out[record + 2] = (std::uint8_t)(pooled.tag >> 8);
				out[record + 3] = (std::uint8_t)pooled.tag;
				out[record + 4] = (std::uint8_t)(checksum >> 24);
				out[record + 5] = (std::uint8_t)(checksum >> 16);
				out[record + 6] = (std::uint8_t)(checksum >> 8);
				out[record + 7] = (std::uint8_t)checksum;
				out[record + 8] = (std::uint8_t)(pooled.offset >> 24);
				out[record + 9] = (std::uint8_t)(pooled.offset >> 16);
				out[record + 10] = (std::uint8_t)(pooled.offset >> 8);
				out[record + 11] = (std::uint8_t)pooled.offset;
				const std::uint32_t length = (std::uint32_t)pooled.data.size();
				out[record + 12] = (std::uint8_t)(length >> 24);
				out[record + 13] = (std::uint8_t)(length >> 16);
				out[record + 14] = (std::uint8_t)(length >> 8);
				out[record + 15] = (std::uint8_t)length;
			}
		}

		for (std::size_t p = 0; p < pool.size(); ++p)
		{
			if (!pool[p].data.empty())
				std::memcpy(&out[pool[p].offset], &pool[p].data[0], pool[p].data.size());
		}
		return out;
	}

	std::vector<FaceSpec> two_faces()
	{
		std::vector<FaceSpec> specs(2);
		specs[0].family = "Test Sans"; specs[0].subfamily = "Regular"; specs[0].glyphs = 42;
		specs[0].cff = false; specs[0].share_tables = false;
		specs[1].family = "Test Serif"; specs[1].subfamily = "Bold"; specs[1].glyphs = 7;
		specs[1].cff = false; specs[1].share_tables = false;
		return specs;
	}
}

/* ---------------------------------------------------------------- utilities */

//! Table payloads must survive the trip byte for byte, except `head.checkSumAdjustment`,
//! which is recomputed for the standalone font by design.
static bool table_equal(const ref::Table& a, const ref::Table& b)
{
	if (a.tag != b.tag || a.data.size() != b.data.size())
		return false;
	const bool head = (a.tag == 0x68656164u && a.data.size() >= 12);
	for (std::size_t i = 0; i < a.data.size(); ++i)
	{
		if (head && i >= 8 && i < 12)
			continue;
		if (a.data[i] != b.data[i])
			return false;
	}
	return true;
}

static bool read_file(const std::string& path, std::vector<std::uint8_t>& out)
{
	out.clear();
	FILE* file = std::fopen(path.c_str(), "rb");
	if (!file)
		return false;
	std::fseek(file, 0, SEEK_END);
	const long size = std::ftell(file);
	std::fseek(file, 0, SEEK_SET);
	if (size > 0)
	{
		out.resize((std::size_t)size);
		if (std::fread(&out[0], 1, out.size(), file) != out.size())
			out.clear();
	}
	std::fclose(file);
	return !out.empty();
}

static bool file_exists(const std::string& path)
{
	FILE* file = std::fopen(path.c_str(), "rb");
	if (!file)
		return false;
	std::fclose(file);
	return true;
}

static std::string find_real_font(int argc, char** argv)
{
	// The corpora CMake passes on the command line win over the fallbacks below.
	for (int i = 1; i < argc; ++i)
	{
		if (file_exists(argv[i]))
			return argv[i];
	}
	const char* from_env = std::getenv("TTC2SFNT_TEST_FONT");
	if (from_env && *from_env && file_exists(from_env))
		return from_env;
	const char* candidates[] =
	{
		"fonts/simsun.ttc",
		"fonts/msyh.ttc",
#if defined(_WIN32)
		"C:/Windows/Fonts/simsun.ttc",
		"C:/Windows/Fonts/msgothic.ttc",
#endif
	};
	for (std::size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i)
	{
		if (file_exists(candidates[i]))
			return candidates[i];
	}
	return std::string();
}

/* -------------------------------------------------------------------- tests */

static void test_version()
{
	section("version");
	CHECK(std::string(TTC2SFNT_VERSION_STRING) == "0.4.0");
	CHECK(TTC2SFNT_VERSION_MAJOR == 0 && TTC2SFNT_VERSION_MINOR == 4 && TTC2SFNT_VERSION_PATCH == 0);
	CHECK(std::string(ttc2sfnt::error_name(ttc2sfnt::err_none)) == "err_none");
}

static void test_roundtrip()
{
	section("roundtrip of a synthetic collection");
	const ref::Bytes collection = fixture::make_ttc(0x00010000u, fixture::two_faces());

	std::uint32_t version = 0;
	std::vector<ref::Face> source;
	std::string why;
	CHECK_MSG(ref::parse_ttc(collection, version, source, why), why);
	CHECK(version == 0x00010000u);
	CHECK(source.size() == 2);

	ttc2sfnt::TTC ttc;
	ttc2sfnt::error_info error;
	CHECK_MSG(ttc2sfnt::UnzipTTC(&collection[0], collection.size(), ttc, &error), error.message);
	CHECK(ttc.version == 0x00010000u);
	CHECK(ttc.ttfs.size() == source.size());
	CHECK(ttc.faces.size() == ttc.ttfs.size());
	if (ttc.ttfs.size() != source.size())
		return;

	CHECK(ttc.faces[0].family == "Test Sans");
	CHECK(ttc.faces[0].subfamily == "Regular");
	CHECK(ttc.faces[0].num_glyphs == 42);
	CHECK(ttc.faces[1].family == "Test Serif");
	CHECK(ttc.faces[1].num_glyphs == 7);

	for (std::size_t i = 0; i < ttc.ttfs.size(); ++i)
	{
		ref::Face produced;
		CHECK_MSG(ref::parse_sfnt(ttc.ttfs[i], produced, why), why);
		CHECK(produced.sfnt == source[i].sfnt);
		CHECK(produced.tables.size() == source[i].tables.size());
		for (std::size_t t = 0; t < produced.tables.size(); ++t)
		{
			const ref::Table* original = source[i].find(produced.tables[t].tag);
			CHECK(original != nullptr);
			if (original)
				CHECK(table_equal(produced.tables[t], *original));
		}
		CHECK(ref::whole_checksum_ok(ttc.ttfs[i]));
		CHECK(ttc.ttfs[i].size() % 4 == 0);

		const ref::Table* head = produced.find(0x68656164u);
		CHECK(head != nullptr);
		if (head && head->data.size() >= 16)
		{
			CHECK(ref::be32(head->data, 12) == 0x5F0F3CF5u);		// magic kept
			CHECK(ref::be32(head->data, 8) != 0x12345678u);			// stale adjustment replaced
		}
	}
}

static void test_shared_tables()
{
	section("shared table pool and 2.0 header");
	std::vector<fixture::FaceSpec> specs(3);
	for (std::size_t i = 0; i < specs.size(); ++i)
	{
		specs[i].family = std::string("Shared ") + (char)('A' + i);
		specs[i].subfamily = "Regular";
		specs[i].glyphs = (std::uint16_t)(10 + i);
		specs[i].cff = false;
		specs[i].share_tables = true;		// every face points at the same table data
	}
	const ref::Bytes collection = fixture::make_ttc(0x00020000u, specs);

	std::uint32_t version = 0;
	std::vector<ref::Face> source;
	std::string why;
	CHECK_MSG(ref::parse_ttc(collection, version, source, why), why);
	CHECK(version == 0x00020000u);
	CHECK(source.size() == 3);

	ttc2sfnt::TTC ttc;
	ttc2sfnt::error_info error;
	CHECK_MSG(ttc2sfnt::UnzipTTC(&collection[0], collection.size(), ttc, &error), error.message);
	CHECK(ttc.version == 0x00020000u);
	CHECK(ttc.ttfs.size() == 3);
	for (std::size_t i = 0; i < ttc.ttfs.size() && i < source.size(); ++i)
	{
		ref::Face produced;
		CHECK_MSG(ref::parse_sfnt(ttc.ttfs[i], produced, why), why);
		for (std::size_t t = 0; t < produced.tables.size(); ++t)
		{
			const ref::Table* original = source[i].find(produced.tables[t].tag);
			CHECK(original != nullptr && table_equal(produced.tables[t], *original));
		}
		CHECK(ref::whole_checksum_ok(ttc.ttfs[i]));
	}
}

static void test_cff_face()
{
	section("OTTO face");
	std::vector<fixture::FaceSpec> specs(1);
	specs[0].family = "Test CFF";
	specs[0].subfamily = "Italic";
	specs[0].glyphs = 3;
	specs[0].cff = true;
	specs[0].share_tables = false;
	const ref::Bytes collection = fixture::make_ttc(0x00010000u, specs);

	ttc2sfnt::TTC ttc;
	CHECK(ttc2sfnt::UnzipTTC(&collection[0], collection.size(), ttc, nullptr));
	CHECK(ttc.ttfs.size() == 1);
	if (!ttc.ttfs.empty())
	{
		ref::Face produced;
		std::string why;
		CHECK_MSG(ref::parse_sfnt(ttc.ttfs[0], produced, why), why);
		CHECK(produced.sfnt == 0x4F54544Fu);
	}
}

static void test_list_and_single_face()
{
	section("ListTTC / UnzipTTCFace");
	const ref::Bytes collection = fixture::make_ttc(0x00010000u, fixture::two_faces());

	std::vector<ttc2sfnt::Face> faces;
	ttc2sfnt::error_info error;
	CHECK_MSG(ttc2sfnt::ListTTC(&collection[0], collection.size(), faces, &error), error.message);
	CHECK(faces.size() == 2);
	if (faces.size() == 2)
	{
		CHECK(faces[0].family == "Test Sans");
		CHECK(faces[1].family == "Test Serif");
		CHECK(faces[0].table_count == 5);
		CHECK(faces[0].sfnt_version == 0x00010000u);
	}

	ttc2sfnt::TTC::TTF face;
	ttc2sfnt::Face info;
	CHECK_MSG(ttc2sfnt::UnzipTTCFace(&collection[0], collection.size(), 1, face, &info, &error), error.message);
	CHECK(info.family == "Test Serif");
	CHECK(ref::whole_checksum_ok(face));

	CHECK(!ttc2sfnt::UnzipTTCFace(&collection[0], collection.size(), 2, face, nullptr, &error));
	CHECK(error.code == ttc2sfnt::err_out_of_range);
}

static void test_to_dir()
{
	section("UnzipTTCToDir");
	const ref::Bytes collection = fixture::make_ttc(0x00010000u, fixture::two_faces());
	const std::string dir = "ttc2sfnt_test_tmp/out";

	std::vector<std::string> written;
	ttc2sfnt::error_info error;
	CHECK_MSG(ttc2sfnt::UnzipTTCToDir(&collection[0], collection.size(), dir, "File", &written, &error), error.message);
	CHECK(written.size() == 2);
	if (written.size() == 2)
	{
		CHECK(written[0].find("Test Sans.ttf") != std::string::npos);
		CHECK(written[1].find("Test Serif.ttf") != std::string::npos);

		std::vector<std::uint8_t> on_disk;
		CHECK(read_file(written[0], on_disk));
		ref::Bytes disk(on_disk.begin(), on_disk.end());
		CHECK(ref::whole_checksum_ok(disk));

		ttc2sfnt::TTC ttc;
		CHECK(ttc2sfnt::UnzipTTC(&collection[0], collection.size(), ttc, nullptr));
		CHECK(disk == ttc.ttfs[0]);
	}

	// A second run must overwrite instead of failing.
	CHECK(ttc2sfnt::UnzipTTCToDir(&collection[0], collection.size(), dir, "File", &written, &error));
	CHECK(written.size() == 2);

	for (std::size_t i = 0; i < written.size(); ++i)
		std::remove(written[i].c_str());
}

static void test_name_collision()
{
	section("duplicate family names and the no-name fallback");
	std::vector<fixture::FaceSpec> specs(4);
	for (std::size_t i = 0; i < 3; ++i)
	{
		specs[i].family = "Same Family";
		specs[i].subfamily = (i == 0) ? "Regular" : "Bold";
		specs[i].glyphs = (std::uint16_t)(5 + i);
		specs[i].cff = false;
		specs[i].share_tables = false;
	}
	specs[3].family = "";
	specs[3].subfamily = "";
	specs[3].glyphs = 9;
	specs[3].cff = false;
	specs[3].share_tables = false;
	specs[3].empty_name = true;
	const ref::Bytes collection = fixture::make_ttc(0x00010000u, specs);

	std::vector<std::string> written;
	ttc2sfnt::error_info error;
	CHECK_MSG(ttc2sfnt::UnzipTTCToDir(&collection[0], collection.size(), "ttc2sfnt_test_tmp/dup", "Face", &written, &error), error.message);
	CHECK(written.size() == 4);
	if (written.size() == 4)
	{
		CHECK(written[0].find("Same Family.ttf") != std::string::npos);
		CHECK(written[1].find("Same Family_Bold.ttf") != std::string::npos);
		CHECK(written[2].find("Same Family_Bold_2.ttf") != std::string::npos);
		CHECK(written[3].find("Face_03.ttf") != std::string::npos);
		CHECK(written[0] != written[1] && written[1] != written[2] && written[2] != written[3]);
	}
	for (std::size_t i = 0; i < written.size(); ++i)
		std::remove(written[i].c_str());
}

static void expect_failure(const char* name, const ref::Bytes& bytes, int expected_code)
{
	std::vector<ttc2sfnt::Face> faces;
	ttc2sfnt::error_info error;
	const bool listed = ttc2sfnt::ListTTC((bytes.empty() ? nullptr : &bytes[0]), bytes.size(), faces, &error);
	CHECK(!listed);
	CHECK_MSG(error.code == expected_code, name + std::string(": got ") + ttc2sfnt::error_name(error.code));

	ttc2sfnt::TTC ttc;
	error.clear();
	const bool extracted = ttc2sfnt::UnzipTTC((bytes.empty() ? nullptr : &bytes[0]), bytes.size(), ttc, &error);
	CHECK(!extracted);
	CHECK_MSG(error.code == expected_code, name + std::string(": got ") + ttc2sfnt::error_name(error.code));
	CHECK(ttc.ttfs.empty());
}

static void test_malformed()
{
	section("malformed input");

	{	// empty
		expect_failure("empty", ref::Bytes(), ttc2sfnt::err_truncated);
	}
	{	// signature only
		ref::Bytes bytes(4, 0);
		bytes[0] = 't'; bytes[1] = 't'; bytes[2] = 'c'; bytes[3] = 'f';
		expect_failure("signature only", bytes, ttc2sfnt::err_truncated);
	}
	{	// not a collection
		ref::Bytes bytes(32, 0x41);
		expect_failure("not a collection", bytes, ttc2sfnt::err_not_a_collection);
	}
	{	// bad version
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		bytes[4] = 0x00; bytes[5] = 0x03; bytes[6] = 0x00; bytes[7] = 0x00;
		expect_failure("bad version", bytes, ttc2sfnt::err_unsupported_version);
	}
	{	// zero faces
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		bytes[8] = bytes[9] = bytes[10] = bytes[11] = 0;
		expect_failure("zero faces", bytes, ttc2sfnt::err_empty_collection);
	}
	{	// face count far beyond the file: must not allocate for it
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		bytes[8] = 0x7F; bytes[9] = 0xFF; bytes[10] = 0xFF; bytes[11] = 0xFF;
		expect_failure("huge face count", bytes, ttc2sfnt::err_bad_face_count);
	}
	{	// face count above the supported limit
		ref::Bytes bytes(12 + 4 * 2000, 0);
		bytes[0] = 't'; bytes[1] = 't'; bytes[2] = 'c'; bytes[3] = 'f';
		bytes[4] = 0x00; bytes[5] = 0x01;
		bytes[8] = 0x00; bytes[9] = 0x00; bytes[10] = 0x07; bytes[11] = 0xD0;	// 2000
		expect_failure("too many faces", bytes, ttc2sfnt::err_bad_face_count);
	}
	{	// face offset past the end
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		bytes[12] = 0xFF; bytes[13] = 0xFF; bytes[14] = 0xFF; bytes[15] = 0xF0;
		expect_failure("face past the end", bytes, ttc2sfnt::err_truncated);
	}
	{	// An offset near 4 GiB must not wrap the bounds check. On a 32 bit build
		// `offset + 12 > size` overflows and the check would disappear entirely.
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		bytes[12] = 0xF4; bytes[13] = 0xFF; bytes[14] = 0xFF; bytes[15] = 0xFF;	// face at 0xFFFFFFF4
		expect_failure("face offset wraps", bytes, ttc2sfnt::err_truncated);
	}
	{	// Same for a table whose offset + length wraps around to zero.
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		const std::uint32_t face = ref::be32(bytes, 12);
		const std::size_t record = face + 12;
		bytes[record + 8] = 0xC0; bytes[record + 9] = 0xFF;
		bytes[record + 10] = 0xFF; bytes[record + 11] = 0xFF;				// table at 0xFFFFFFC0
		bytes[record + 12] = 0x00; bytes[record + 13] = 0x00;
		bytes[record + 14] = 0x00; bytes[record + 15] = 0x40;				// length 0x40, sum is 2^32
		expect_failure("table size wraps", bytes, ttc2sfnt::err_truncated);
	}
	{	// unknown sfnt version inside the face
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		const std::uint32_t face = ref::be32(bytes, 12);
		bytes[face] = 0xDE; bytes[face + 1] = 0xAD; bytes[face + 2] = 0xBE; bytes[face + 3] = 0xEF;
		expect_failure("unknown sfnt", bytes, ttc2sfnt::err_bad_face);
	}
	{	// zero tables
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		const std::uint32_t face = ref::be32(bytes, 12);
		bytes[face + 4] = bytes[face + 5] = 0;
		expect_failure("zero tables", bytes, ttc2sfnt::err_bad_table_count);
	}
	{	// too many tables
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		const std::uint32_t face = ref::be32(bytes, 12);
		bytes[face + 4] = 0x20; bytes[face + 5] = 0x00;		// 8192
		expect_failure("too many tables", bytes, ttc2sfnt::err_bad_table_count);
	}
	{	// table data past the end
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		const std::uint32_t face = ref::be32(bytes, 12);
		const std::size_t record = face + 12 + 16;			// second table record
		bytes[record + 12] = 0x7F; bytes[record + 13] = 0xFF;
		bytes[record + 14] = 0xFF; bytes[record + 15] = 0xF0;
		expect_failure("table past the end", bytes, ttc2sfnt::err_truncated);
	}
	{	// table directory cut short
		ref::Bytes bytes = fixture::make_ttc(0x00010000u, fixture::two_faces());
		bytes.resize(20);
		expect_failure("truncated file", bytes, ttc2sfnt::err_truncated);
	}
	{	// missing file
		ttc2sfnt::TTC ttc;
		ttc2sfnt::error_info error;
		CHECK(!ttc2sfnt::UnzipTTC(std::string("ttc2sfnt_test_tmp/does_not_exist.ttc"), ttc, &error));
		CHECK(error.code == ttc2sfnt::err_open_failed);
		CHECK(ttc.ttfs.empty());

		std::vector<ttc2sfnt::Face> faces;
		CHECK(!ttc2sfnt::ListTTC(std::string("ttc2sfnt_test_tmp/does_not_exist.ttc"), faces, &error));
		CHECK(error.code == ttc2sfnt::err_open_failed);
	}
}

//! Every entry point has to reject these bytes the same way, and leave its outputs untouched.
static void check_rejection(const std::string& path, int expected_code)
{
	std::vector<ttc2sfnt::Face> faces;
	ttc2sfnt::error_info error;
	CHECK(!ttc2sfnt::ListTTC(path, faces, &error));
	CHECK_MSG(error.code == expected_code, std::string("ListTTC: ") + ttc2sfnt::error_name(error.code));
	CHECK(faces.empty());

	ttc2sfnt::TTC ttc;
	CHECK(!ttc2sfnt::UnzipTTC(path, ttc, &error));
	CHECK_MSG(error.code == expected_code, std::string("UnzipTTC: ") + ttc2sfnt::error_name(error.code));
	CHECK(ttc.ttfs.empty());
	CHECK(ttc.faces.empty());

	ttc2sfnt::TTC::TTF one;
	CHECK(!ttc2sfnt::UnzipTTCFace(path, 0, one, nullptr, &error));
	CHECK_MSG(error.code == expected_code, std::string("UnzipTTCFace: ") + ttc2sfnt::error_name(error.code));
	CHECK(one.empty());

	std::vector<std::string> written;
	CHECK(!ttc2sfnt::UnzipTTCToDir(path, "ttc2sfnt_test_tmp/rejected", "F", &written, &error));
	CHECK_MSG(error.code == expected_code, std::string("UnzipTTCToDir: ") + ttc2sfnt::error_name(error.code));
	CHECK(written.empty());

	CHECK(!ttc2sfnt::UnzipTTCFaceToFile(path, 0, "ttc2sfnt_test_tmp/rejected/x.ttf", nullptr, &error));
	CHECK_MSG(error.code == expected_code, std::string("UnzipTTCFaceToFile: ") + ttc2sfnt::error_name(error.code));
}

static void test_real_font(const std::string& path)
{
	std::printf("  using %s\n", path.c_str());

	ref::Bytes collection;
	{
		std::vector<std::uint8_t> raw;
		if (!read_file(path, raw))
		{
			CHECK_MSG(false, "cannot read the test font");
			return;
		}
		collection.assign(raw.begin(), raw.end());
	}

	std::uint32_t version = 0;
	std::vector<ref::Face> source;
	std::string why;
	const bool ref_ok = ref::parse_ttc(collection, version, source, why);

	ttc2sfnt::Collection parsed;
	ttc2sfnt::error_info library_error;
	const bool library_ok = parsed.parse(&collection[0], collection.size(), &library_error);

	// The corpus is whatever sits in fonts/, so it may hold files that are not collections at all:
	// encrypted by a DLP agent, truncated, or simply not a font. The two readers have to agree on
	// that verdict; when both reject, the rejection path is what gets exercised.
	CHECK_MSG(library_ok == ref_ok, std::string("verdict mismatch, independent reader says: ") + why);
	if (!library_ok)
	{
		std::printf("  rejected: %s: %s\n", ttc2sfnt::error_name(library_error.code), library_error.message.c_str());
		check_rejection(path, library_error.code);
		return;
	}
	if (source.empty())
		return;

	const clock_t started = std::clock();
	ttc2sfnt::TTC ttc;
	ttc2sfnt::error_info error;
	const bool ok = ttc2sfnt::UnzipTTC(&collection[0], collection.size(), ttc, &error);
	const double milliseconds = 1000.0 * (double)(std::clock() - started) / (double)CLOCKS_PER_SEC;
	CHECK_MSG(ok, error.message);
	if (!ok)
		return;

	std::printf("  %zu faces, %.1f ms, %.1f MB/s\n", ttc.ttfs.size(), milliseconds,
		milliseconds > 0.0 ? ((double)collection.size() / 1048576.0) / (milliseconds / 1000.0) : 0.0);
	CHECK(ttc.ttfs.size() == source.size());

	std::size_t total = 0;
	for (std::size_t i = 0; i < ttc.ttfs.size() && i < source.size(); ++i)
	{
		total += ttc.ttfs[i].size();
		ref::Face produced;
		CHECK_MSG(ref::parse_sfnt(ttc.ttfs[i], produced, why), why);
		CHECK(produced.tables.size() == source[i].tables.size());
		for (std::size_t t = 0; t < produced.tables.size(); ++t)
		{
			const ref::Table* original = source[i].find(produced.tables[t].tag);
			CHECK_MSG(original != nullptr, "missing table in source");
			if (original)
				CHECK(table_equal(produced.tables[t], *original));
		}
		CHECK(ref::whole_checksum_ok(ttc.ttfs[i]));
	}
	std::printf("  output %.1f MB (%.2fx)\n", (double)total / 1048576.0, (double)total / (double)collection.size());

	// One face at a time must match the bulk call.
	if (!ttc.ttfs.empty())
	{
		ttc2sfnt::TTC::TTF single;
		CHECK(ttc2sfnt::UnzipTTCFace(&collection[0], collection.size(), 0, single, nullptr, nullptr));
		CHECK(single == ttc.ttfs[0]);
	}

	// Writing next to the collection must match memory, and must create the tree.
	const std::string dir = std::string("ttc2sfnt_test_tmp/real/nested/");
	std::vector<std::string> written;
	CHECK_MSG(ttc2sfnt::UnzipTTCToDir(&collection[0], collection.size(), dir, "Face", &written, &error), error.message);
	CHECK(written.size() == ttc.ttfs.size());
	for (std::size_t i = 0; i < written.size() && i < ttc.ttfs.size(); ++i)
	{
		std::vector<std::uint8_t> raw;
		CHECK(read_file(written[i], raw));
		ref::Bytes disk(raw.begin(), raw.end());
		CHECK(disk == ttc.ttfs[i]);
		std::remove(written[i].c_str());
	}
}

static void test_classes()
{
	section("Sfnt / Collection classes");
	const ref::Bytes collection = fixture::make_ttc(0x00020000u, fixture::two_faces());

	ttc2sfnt::Collection parsed;
	ttc2sfnt::error_info error;
	CHECK_MSG(parsed.parse(&collection[0], collection.size(), &error), error.message);
	CHECK(parsed.valid());
	CHECK(parsed.version() == 0x00020000u);
	CHECK(parsed.face_count() == 2);

	ttc2sfnt::Sfnt sfnt;
	CHECK(parsed.sfnt_of(0, sfnt, &error));
	CHECK(sfnt.valid());
	CHECK(sfnt.table_count() == 5);
	CHECK(sfnt.version() == 0x00010000u);
	CHECK(!sfnt.is_cff());
	CHECK(sfnt.index_of(ttc2sfnt::Sfnt::tag_head) >= 0);
	CHECK(sfnt.find(0x7A7A7A7Au) == nullptr);
	CHECK(ttc2sfnt::Sfnt::tag_string(ttc2sfnt::Sfnt::tag_head) == "head");
	CHECK(ttc2sfnt::Sfnt::checksum(&collection[0], collection.size()) ==
		ref::sum32(&collection[0], collection.size()));

	// build() must agree with the free function, byte for byte.
	std::vector<std::uint8_t> built;
	CHECK_MSG(sfnt.build(&collection[0], collection.size(), built, &error), error.message);
	ttc2sfnt::TTC::TTF expected;
	ttc2sfnt::Face face;
	CHECK(ttc2sfnt::UnzipTTCFace(&collection[0], collection.size(), 0, expected, &face, &error));
	CHECK(built == expected);
	CHECK(ttc2sfnt::Sfnt::checksum(&built[0], built.size()) == 0xB1B0AFBAu);

	// describe() must agree with ListTTC().
	ttc2sfnt::Face described;
	CHECK(parsed.describe(0, described, &error));
	CHECK(described.family == face.family);
	CHECK(described.num_glyphs == face.num_glyphs);

	CHECK(!parsed.sfnt_of(2, sfnt, &error));
	CHECK(error.code == ttc2sfnt::err_out_of_range);

	section("file names");
	CHECK(ttc2sfnt::sanitize_file_name("  a/b:c*d  ") == "a_b_c_d");
	CHECK(ttc2sfnt::sanitize_file_name("..") == "");
	CHECK(ttc2sfnt::same_file_name("SimSun.TTF", "simsun.ttf"));
	CHECK(!ttc2sfnt::same_file_name("SimSun", "SimSun2"));

	ttc2sfnt::Face named;
	named.family = "Same";
	named.subfamily = "Bold";
	std::vector<std::string> used;
	CHECK(ttc2sfnt::file_stem_for(named, 3, "Face", used) == "Same");
	used.push_back("same");
	CHECK(ttc2sfnt::file_stem_for(named, 3, "Face", used) == "Same_Bold");
	used.push_back("Same_Bold");
	CHECK(ttc2sfnt::file_stem_for(named, 3, "Face", used) == "Same_Bold_2");

	ttc2sfnt::Face unnamed;
	CHECK(ttc2sfnt::file_stem_for(unnamed, 3, "Face", used) == "Face_03");
	CHECK(ttc2sfnt::file_stem_for(unnamed, 3, "", used) == "_03");

	// A pile of identical families must still end up with one distinct stem each.
	std::vector<std::string> many;
	for (std::size_t i = 0; i < 64; ++i)
	{
		const std::string stem = ttc2sfnt::file_stem_for(named, i, "Face", many);
		bool clash = false;
		for (std::size_t k = 0; k < many.size(); ++k)
			clash = clash || ttc2sfnt::same_file_name(stem, many[k]);
		CHECK_MSG(!clash, "duplicate stem from file_stem_for");
		many.push_back(stem);
	}

	section("paths");
	CHECK(ttc2sfnt::parent_directory("a/b/c.ttf") == "a/b");
	CHECK(ttc2sfnt::parent_directory("c.ttf") == ".");
	CHECK(ttc2sfnt::parent_directory("/c.ttf") == "/");
	CHECK(ttc2sfnt::join_path("a/b", "c.ttf") == "a/b/c.ttf");
	CHECK(ttc2sfnt::join_path("a/b/", "c.ttf") == "a/b/c.ttf");
	CHECK(ttc2sfnt::join_path("", "c.ttf") == "c.ttf");

	section("directories");
	{
		// A previous run that stopped early may have left this name behind in either shape.
		std::remove("ttc2sfnt_test_tmp/dirs/file_as_dir");
		remove_dir("ttc2sfnt_test_tmp/dirs/file_as_dir");

		// write_file() does not create directories, only the extractors do.
		const char payload[] = "x";
		CHECK(!ttc2sfnt::write_file("ttc2sfnt_test_tmp/dirs/file_as_dir", (const std::uint8_t*)payload, 1, &error));
		CHECK(error.code == ttc2sfnt::err_write_failed);

		CHECK(ttc2sfnt::ensure_directory("ttc2sfnt_test_tmp/dirs", &error));
		CHECK(error.ok());
		CHECK(ttc2sfnt::write_file("ttc2sfnt_test_tmp/dirs/file_as_dir", (const std::uint8_t*)payload, 1, &error));
		CHECK(error.ok());

		// A plain file in the way is not a directory; saying "ok" here used to be wrong.
		CHECK(!ttc2sfnt::ensure_directory("ttc2sfnt_test_tmp/dirs/file_as_dir", &error));
		CHECK(error.code == ttc2sfnt::err_mkdir_failed);
		CHECK(!error.message.empty());

		CHECK(ttc2sfnt::ensure_directory("ttc2sfnt_test_tmp/dirs/a/b/c", &error));
		CHECK(error.ok());
		CHECK(ttc2sfnt::ensure_directory("ttc2sfnt_test_tmp/dirs/a/b/c", &error));	// already there
		CHECK(error.ok());
		CHECK(ttc2sfnt::ensure_directory("", &error));
		CHECK(error.ok());
		std::remove("ttc2sfnt_test_tmp/dirs/file_as_dir");
	}
}

static void test_verify_and_handoff()
{
	section("face verification and hand-off");
	const ref::Bytes collection = fixture::make_ttc(0x00010000u, fixture::two_faces());
	ttc2sfnt::TTC ttc;
	ttc2sfnt::error_info error;
	CHECK(ttc2sfnt::UnzipTTC(&collection[0], collection.size(), ttc, &error));
	CHECK(ttc.ttfs.size() == 2);
	if (ttc.ttfs.empty())
		return;

	ttc2sfnt::Sfnt_Report report;
	CHECK(ttc2sfnt::Sfnt::verify(&ttc.ttfs[0][0], ttc.ttfs[0].size(), &report));
	CHECK(report.ok());
	CHECK(report.table_count == 5);
	CHECK(report.head_magic);
	CHECK(report.whole_checksum_ok);
	CHECK(report.whole_checksum == 0xB1B0AFBAu);
	CHECK(report.bad_checksums == 0);
	CHECK(report.unaligned == 0);
	CHECK(report.out_of_bounds == 0);
	CHECK(report.first_problem.empty());

	// One flipped payload byte has to be caught, by the table checksum and the whole font checksum.
	// The offset has to come from the rebuilt face, whose layout differs from the collection's.
	ttc2sfnt::Sfnt standalone;
	CHECK(standalone.parse(&ttc.ttfs[0][0], ttc.ttfs[0].size(), 0, &error));
	const int glyf = standalone.index_of(0x676C7966u);	// 'glyf'
	CHECK(glyf >= 0);
	if (glyf >= 0)
	{
		std::vector<std::uint8_t> damaged = ttc.ttfs[0];
		const std::size_t at = (std::size_t)standalone.table((std::size_t)glyf).offset;
		CHECK(at + 1 < damaged.size());
		if (at + 1 < damaged.size())
		{
			damaged[at] = (std::uint8_t)(damaged[at] ^ 0xFFu);
			ttc2sfnt::Sfnt_Report bad;
			CHECK(!ttc2sfnt::Sfnt::verify(&damaged[0], damaged.size(), &bad));
			CHECK(bad.bad_checksums >= 1);
			CHECK(!bad.whole_checksum_ok);
			CHECK(!bad.first_problem.empty());
		}
	}

	// What a consumer like ttf2mesh does: convert the table directory to host byte order in place.
	// The result is a mixed endian font that cannot be repaired, only detected.
	{
		std::vector<std::uint8_t> scribbled = ttc.ttfs[0];
		const std::size_t tables = (std::size_t)((scribbled[4] << 8) | scribbled[5]);
		for (std::size_t t = 0; t < tables; ++t)
		{
			for (int field = 1; field <= 3; ++field)	// checkSum, offset, length
			{
				std::uint8_t* p = &scribbled[12 + t * 16 + 4 * (std::size_t)field];
				std::uint8_t tmp = p[0]; p[0] = p[3]; p[3] = tmp;
				tmp = p[1]; p[1] = p[2]; p[2] = tmp;
			}
		}
		ttc2sfnt::Sfnt_Report bad;
		CHECK(!ttc2sfnt::Sfnt::verify(&scribbled[0], scribbled.size(), &bad));
		CHECK(bad.out_of_bounds >= 2);
		CHECK(!bad.first_problem.empty());
	}

	// The guard: hand the consumer a copy, and the caller's bytes stay untouched.
	{
		const std::vector<std::uint8_t> original = ttc.ttfs[0];
		std::size_t seen = 0;
		int first = -1;
		const int returned = ttc2sfnt::with_scratch_copy(ttc.ttfs[0], [&](const std::uint8_t* data, std::size_t size) {
			seen = size;
			first = data[0];
			data = data;								// the pointer is only borrowed
			const_cast<std::uint8_t*>(data)[0] = 0xEE;	// exactly what such a consumer does
			return 7;
		});
		CHECK(returned == 7);
		CHECK(seen == original.size());
		CHECK(first == (int)original[0]);
		CHECK(ttc.ttfs[0] == original);
		CHECK(ttc2sfnt::Sfnt::verify(&ttc.ttfs[0][0], ttc.ttfs[0].size(), nullptr));
	}
}

static void test_real_fonts(int argc, char** argv)
{
	section("real collections");

	std::vector<std::string> corpus;
	for (int i = 1; i < argc; ++i)
	{
		if (file_exists(argv[i]))
			corpus.push_back(argv[i]);
	}
	if (corpus.empty())
	{
		const std::string fallback = find_real_font(argc, argv);
		if (!fallback.empty() && corpus.empty())
			corpus.push_back(fallback);
	}
	if (corpus.empty())
	{
		std::printf("  SKIP: no .ttc found (pass one on the command line or set TTC2SFNT_TEST_FONT)\n");
		return;
	}
	for (std::size_t i = 0; i < corpus.size(); ++i)
		test_real_font(corpus[i]);
}

int main(int argc, char** argv)
{
	std::printf("ttc2sfnt %s regression test\n", TTC2SFNT_VERSION_STRING);
	test_version();
	test_roundtrip();
	test_shared_tables();
	test_cff_face();
	test_list_and_single_face();
	test_to_dir();
	test_name_collision();
	test_malformed();
	test_classes();
	test_verify_and_handoff();
	test_real_fonts(argc, argv);

	// The tests already removed their files; drop the scratch directories too.
	remove_dir("ttc2sfnt_test_tmp/out");
	remove_dir("ttc2sfnt_test_tmp/dup");
	remove_dir("ttc2sfnt_test_tmp/rejected");
	remove_dir("ttc2sfnt_test_tmp/dirs/a/b/c");
	remove_dir("ttc2sfnt_test_tmp/dirs/a/b");
	remove_dir("ttc2sfnt_test_tmp/dirs/a");
	remove_dir("ttc2sfnt_test_tmp/dirs/file_as_dir");
	remove_dir("ttc2sfnt_test_tmp/dirs");
	remove_dir("ttc2sfnt_test_tmp/real/nested");
	remove_dir("ttc2sfnt_test_tmp/real");
	remove_dir("ttc2sfnt_test_tmp");

	std::printf("%d checks, %d failures\n", g_checks, g_failures);
	if (g_failures != 0)
	{
		std::printf("RESULT: FAIL\n");
		return 1;
	}
	std::printf("RESULT: PASS\n");
	return 0;
}
