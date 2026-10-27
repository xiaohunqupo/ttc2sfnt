#pragma once

/*!
 * \file ttc_collection.h
 * \brief The collection header, and the extraction entry points that work on memory or on a file.
 */

#include "ttc2sfnt/ttc2sfnt_config.h"
#include "ttc2sfnt/ttc_error.h"
#include "ttc2sfnt/ttc_sfnt.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ttc2sfnt
{
	/*!
	 * \brief Metadata of one face, read from the `name` and `maxp` tables.
	 */
	struct Face
	{
		std::uint32_t sfnt_version = 0;	//!< 0x00010000 / `true` / `OTTO`
		std::uint32_t table_count = 0;	//!< number of tables in the face
		std::uint16_t num_glyphs = 0;	//!< `maxp.numGlyphs`, 0 when the table is missing
		std::string family;				//!< `name` id 1, UTF-8
		std::string subfamily;			//!< `name` id 2, UTF-8
	};

	/*!
	 * \brief A collection unpacked in memory.
	 */
	struct TTC
	{
		typedef std::vector<std::uint8_t> TTF;

		std::uint32_t version = 0;		//!< collection version from the header
		std::vector<TTF> ttfs;			//!< one complete sfnt per face, in file order
		std::vector<Face> faces;		//!< metadata, always parallel to `ttfs`
	};

	/*!
	 * \brief The header of a `.ttc`: where every face starts.
	 *
	 * An instance borrows the collection bytes; it never copies them. Parsing is cheap and does not
	 * touch the table data, so a caller that only wants the face list or one face pays for neither.
	 */
	class Collection
	{
	public:
		Collection();

		/*!
		 * \brief Parse and validate the collection header.
		 *
		 * Checks the signature, the version, the face count against both the supported limit and the
		 * file size, and that every face offset leaves room for at least a sfnt header.
		 *
		 * \param data Collection bytes.
		 * \param size Byte count of `data`.
		 * \param [out] error Optional diagnostic.
		 * \return `false` and an empty collection when the header is not usable.
		 */
		bool parse(const std::uint8_t* data, std::size_t size, error_info* error = nullptr);

		bool valid() const { return m_valid; }
		std::uint32_t version() const { return m_version; }		//!< 1.0 (`0x00010000`) or 2.0 (`0x00020000`).
		std::size_t face_count() const { return m_offsets.size(); }

		/*!
		 * \brief Parse the table directory of one face.
		 * \param index Face index, `0 .. face_count()-1`.
		 * \param [out] out Receives the directory.
		 * \param [out] error Optional diagnostic, `err_out_of_range` for a bad index.
		 */
		bool sfnt_of(std::size_t index, Sfnt& out, error_info* error = nullptr) const;

		/*!
		 * \brief Read the metadata of one face.
		 */
		bool describe(std::size_t index, Face& out, error_info* error = nullptr) const;

		/*!
		 * \brief Rebuild one face as a standalone sfnt.
		 * \param [out] out Receives the font bytes.
		 * \param [out] face Optional metadata of the extracted face.
		 */
		bool extract(std::size_t index, TTC::TTF& out, Face* face = nullptr, error_info* error = nullptr) const;

		/*!
		 * \brief Rebuild every face as a standalone sfnt.
		 *
		 * Cost: roughly `face_count() x size()` bytes, because the table data the faces share has to
		 * be copied into each of them. Use `extract()` to hold one face at a time.
		 */
		bool extract_all(TTC& out, error_info* error = nullptr) const;

	private:
		bool m_valid;
		const std::uint8_t* m_data;
		std::size_t m_size;
		std::uint32_t m_version;
		std::vector<std::uint32_t> m_offsets;
	};

	/*!
	 * \brief Read face metadata without rebuilding the faces.
	 * \param data Collection bytes.
	 * \param size Byte count.
	 * \param [out] faces One entry per face, in file order.
	 * \param [out] error Optional diagnostic.
	 */
	bool ListTTC(const void* data, std::size_t size, std::vector<Face>& faces, error_info* error = nullptr);

	/*!
	 * \brief Read face metadata straight from a file.
	 */
	bool ListTTC(const std::string& filename, std::vector<Face>& faces, error_info* error = nullptr);

	/*!
	 * \brief Rebuild every face of a collection as standalone sfnts in memory.
	 */
	bool UnzipTTC(const void* data, std::size_t size, TTC& ttc, error_info* error = nullptr);

	/*!
	 * \brief Rebuild every face of a file as standalone sfnts in memory.
	 */
	bool UnzipTTC(const std::string& filename, TTC& ttc, error_info* error = nullptr);

	/*!
	 * \brief Rebuild one face as a standalone sfnt in memory.
	 *
	 * The bytes are the caller's. Note that some consumers take a `const std::uint8_t*` and write
	 * into it anyway (`ttf2mesh` converts the directory to host byte order in place), so a face that
	 * has been handed to such a consumer is damaged beyond repair - see `with_scratch_copy()` in
	 * `ttc_sfnt.h` for the safe way to do that, and `Sfnt::verify()` for the check.
	 *
	 * \param index Face index, `0 .. face_count()-1`.
	 * \param [out] ttf Receives the complete sfnt bytes.
	 * \param [out] face Optional metadata.
	 */
	bool UnzipTTCFace(const void* data, std::size_t size, std::size_t index,
		TTC::TTF& ttf, Face* face = nullptr, error_info* error = nullptr);

	/*!
	 * \brief Rebuild one face of a file.
	 */
	bool UnzipTTCFace(const std::string& filename, std::size_t index,
		TTC::TTF& ttf, Face* face = nullptr, error_info* error = nullptr);
}

#ifdef TTC2SFNT_HEAD_ONLY
#include "ttc2sfnt/ttc_collection.cpp"
#endif // TTC2SFNT_HEAD_ONLY
