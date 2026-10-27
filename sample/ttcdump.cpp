// ttcdump: print the structure of a TrueType Collection and verify every face it contains.
//
//   ttcdump <input.ttc> [...]
//
// For every face the table directory is printed and then checked with Sfnt::verify() - table
// checksums, four byte alignment, head.magicNumber and the whole font checksum 0xB1B0AFBA. The exit
// code is the number of problems found, so the program doubles as a validator.

#include "ttc2sfnt/ttc2sfnt.h"
#include <cstdio>
#include <string>
#include <vector>

namespace
{
	int dump_file(const std::string& path)
	{
		std::vector<std::uint8_t> data;
		ttc2sfnt::error_info error;
		if (!ttc2sfnt::read_file(path, data, &error))
		{
			std::fprintf(stderr, "ttcdump: %s: %s\n", path.c_str(), error.message.c_str());
			return 1;
		}

		ttc2sfnt::Collection collection;
		if (!collection.parse(data.empty() ? nullptr : &data[0], data.size(), &error))
		{
			std::fprintf(stderr, "ttcdump: %s: %s (%s)\n", path.c_str(),
				error.message.c_str(), ttc2sfnt::error_name(error.code));
			return 1;
		}

		std::printf("%s\n", path.c_str());
		std::printf("  collection version %.1f, %u face(s), %u bytes\n",
			collection.version() == 0x00020000u ? 2.0 : 1.0,
			(unsigned int)collection.face_count(),
			(unsigned int)data.size());

		int problems = 0;
		std::vector<std::uint8_t> rebuilt;
		for (std::size_t i = 0; i < collection.face_count(); ++i)
		{
			ttc2sfnt::Sfnt sfnt;
			ttc2sfnt::Face face;
			if (!collection.sfnt_of(i, sfnt, &error) || !collection.describe(i, face, &error))
			{
				std::fprintf(stderr, "  face %u: %s\n", (unsigned int)i, error.message.c_str());
				++problems;
				continue;
			}

			std::printf("  face %u: '%s' '%s', %u table(s), %u glyph(s)\n",
				(unsigned int)i,
				face.family.empty() ? "(no name)" : face.family.c_str(),
				face.subfamily.c_str(),
				(unsigned int)sfnt.table_count(),
				(unsigned int)face.num_glyphs);
			for (std::size_t t = 0; t < sfnt.table_count(); ++t)
			{
				const ttc2sfnt::Table_Record& table = sfnt.table(t);
				std::printf("    %s  off=0x%08X len=%7u sum=0x%08X\n",
					ttc2sfnt::Sfnt::tag_string(table.tag).c_str(),
					(unsigned int)table.offset,
					(unsigned int)table.length,
					(unsigned int)table.check_sum);
			}

			if (!collection.extract(i, rebuilt, nullptr, &error))
			{
				std::fprintf(stderr, "    rebuild failed: %s\n", error.message.c_str());
				++problems;
				continue;
			}
			ttc2sfnt::Sfnt_Report report;
			const bool ok = ttc2sfnt::Sfnt::verify(&rebuilt[0], rebuilt.size(), &report);
			if (!ok)
				++problems;
			std::printf("    rebuilt %u bytes, verify %s (whole 0x%08X)%s%s\n",
				(unsigned int)rebuilt.size(), ok ? "ok" : "FAILED",
				(unsigned int)report.whole_checksum,
				report.first_problem.empty() ? "" : ": ",
				report.first_problem.c_str());
		}
		return problems;
	}
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::fprintf(stderr,
			"ttcdump %s - print the structure of a TrueType Collection\n"
			"\n"
			"usage: ttcdump <input.ttc> [...]\n"
			"\n"
			"Exit code is the number of problems found (bad checksums, unreadable faces).\n",
			TTC2SFNT_VERSION_STRING);
		return 2;
	}

	int problems = 0;
	for (int i = 1; i < argc; ++i)
		problems += dump_file(argv[i]);
	return problems > 255 ? 255 : problems;
}
