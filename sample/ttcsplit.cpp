// ttcsplit: write every face of a TrueType Collection as a standalone .ttf / .otf.
//
//   ttcsplit <input.ttc> [output_dir]       write every face as <family>.ttf / .otf
//   ttcsplit -l <input.ttc>                 list the faces
//   ttcsplit -x <input.ttc> <index> <out>   extract a single face
//   ttcsplit -v                             print the version
//   ttcsplit -h                             print this help

#include "ttc2sfnt/ttc2sfnt.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{
	void print_usage(std::FILE* out)
	{
		std::fprintf(out,
			"ttcsplit %s - split a TrueType Collection into standalone fonts\n"
			"\n"
			"usage:\n"
			"  ttcsplit <input.ttc> [output_dir]       extract every face\n"
			"  ttcsplit -l <input.ttc>                 list the faces\n"
			"  ttcsplit -x <input.ttc> <index> <out>   extract one face\n"
			"  ttcsplit -v                             print the version\n"
			"  ttcsplit -h                             print this help\n"
			"\n"
			"Faces are written as <family>.ttf, or .otf for CFF outlines. The output directory\n"
			"defaults to the directory of the input file and is created when missing. Results go\n"
			"to stdout, problems to stderr.\n",
			TTC2SFNT_VERSION_STRING);
	}

	void print_error(const std::string& what, const ttc2sfnt::error_info& error)
	{
		std::fprintf(stderr, "ttcsplit: %s: %s (%s)\n",
			what.c_str(), error.message.c_str(), ttc2sfnt::error_name(error.code));
	}

	const char* sfnt_name(std::uint32_t version)
	{
		switch (version)
		{
		case 0x00010000u: return "TrueType";
		case 0x74727565u: return "true";
		case 0x4F54544Fu: return "CFF";
		default: return "unknown";
		}
	}

	int list_faces(const std::string& input)
	{
		std::vector<ttc2sfnt::Face> faces;
		ttc2sfnt::error_info error;
		if (!ttc2sfnt::ListTTC(input, faces, &error))
		{
			print_error(input, error);
			return 1;
		}
		std::printf("%s: %u face(s)\n", input.c_str(), (unsigned int)faces.size());
		for (std::size_t i = 0; i < faces.size(); ++i)
		{
			std::printf("  [%u] %-34s %-16s %6u glyphs  %2u tables  %s\n",
				(unsigned int)i,
				faces[i].family.empty() ? "(no name)" : faces[i].family.c_str(),
				faces[i].subfamily.c_str(),
				(unsigned int)faces[i].num_glyphs,
				(unsigned int)faces[i].table_count,
				sfnt_name(faces[i].sfnt_version));
		}
		return 0;
	}

	int extract_all(const std::string& input, const std::string& output_dir)
	{
		const std::string dir = output_dir.empty() ? ttc2sfnt::parent_directory(input) : output_dir;
		std::vector<std::string> written;
		ttc2sfnt::error_info error;
		if (!ttc2sfnt::UnzipTTCToDir(input, dir, "Face", &written, &error))
		{
			print_error(input, error);
			return 1;
		}
		for (std::size_t i = 0; i < written.size(); ++i)
			std::printf("%s\n", written[i].c_str());
		std::printf("%u face(s) written to %s\n", (unsigned int)written.size(), dir.c_str());
		return 0;
	}

	int extract_one(const std::string& input, std::size_t index, const std::string& output)
	{
		ttc2sfnt::Face info;
		ttc2sfnt::error_info error;
		if (!ttc2sfnt::UnzipTTCFaceToFile(input, index, output, &info, &error))
		{
			print_error(input, error);
			return 1;
		}
		std::printf("%s: %s %s, %u glyphs\n", output.c_str(),
			info.family.empty() ? "(no name)" : info.family.c_str(),
			info.subfamily.c_str(), (unsigned int)info.num_glyphs);
		return 0;
	}
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		print_usage(stderr);
		return 2;
	}

	const std::string first = argv[1];
	if (first == "-h" || first == "--help")
	{
		print_usage(stdout);
		return 0;
	}
	if (first == "-v" || first == "--version")
	{
		std::printf("ttcsplit %s\n", TTC2SFNT_VERSION_STRING);
		return 0;
	}
	if (first == "-l" || first == "--list")
	{
		if (argc < 3)
		{
			print_usage(stderr);
			return 2;
		}
		return list_faces(argv[2]);
	}
	if (first == "-x" || first == "--extract")
	{
		if (argc < 5)
		{
			print_usage(stderr);
			return 2;
		}
		char* end = nullptr;
		const long index = std::strtol(argv[3], &end, 10);
		if (!end || *end != '\0' || index < 0)
		{
			std::fprintf(stderr, "ttcsplit: '%s' is not a face index\n", argv[3]);
			return 2;
		}
		return extract_one(argv[2], (std::size_t)index, argv[4]);
	}
	if (!first.empty() && first[0] == '-')
	{
		std::fprintf(stderr, "ttcsplit: unknown option '%s'\n", first.c_str());
		print_usage(stderr);
		return 2;
	}

	return extract_all(first, argc > 2 ? std::string(argv[2]) : std::string());
}
