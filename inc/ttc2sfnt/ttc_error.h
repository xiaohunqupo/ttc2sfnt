#pragma once

/*!
 * \file ttc_error.h
 * \brief Failure reporting: one code per reason, plus an optional printable message.
 */

#include "ttc2sfnt/ttc2sfnt_config.h"
#include <string>

namespace ttc2sfnt
{
	/*!
	 * \brief Reason a call failed, see `error_info::code`.
	 */
	enum error_code
	{
		err_none = 0,					//!< success
		err_open_failed = 1,			//!< input file could not be opened
		err_read_failed = 2,			//!< input file could not be read to the end
		err_write_failed = 3,			//!< a face could not be written
		err_mkdir_failed = 4,			//!< the output directory could not be created
		err_not_a_collection = 5,		//!< signature is not `ttcf`
		err_unsupported_version = 6,	//!< collection version is not 1.0 or 2.0
		err_empty_collection = 7,		//!< the header declares zero faces
		err_bad_face_count = 8,			//!< face count exceeds the supported limit
		err_truncated = 9,				//!< header, directory or table data runs past the end of the file
		err_bad_face = 10,				//!< a face is not a known sfnt (TrueType / `true` / `OTTO`)
		err_bad_table_count = 11,		//!< a face declares zero or too many tables
		err_out_of_range = 12			//!< face index is outside the collection
	};

	/*!
	 * \brief Short name of an error code, for logs.
	 * \param code Value of `error_info::code`, any value is accepted.
	 * \return A static string, never `nullptr`; `"err_unknown"` for values outside the enum.
	 */
	const char* error_name(int code);

	/*!
	 * \brief Diagnostic for the last call.
	 *
	 * Every function that can fail takes an optional `error_info*`; pass `nullptr` when only the
	 * boolean result matters. On failure the code says what went wrong and the message carries the
	 * offending path, offset or index, so it can be printed without further formatting.
	 */
	struct error_info
	{
		int code = err_none;	//!< One of `error_code`.
		std::string message;	//!< Human readable reason, empty on success.

		bool ok() const { return code == err_none; }
		void clear() { code = err_none; message.clear(); }
	};
}

#ifdef TTC2SFNT_HEAD_ONLY
#include "ttc2sfnt/ttc_error.cpp"
#endif // TTC2SFNT_HEAD_ONLY
