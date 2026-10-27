/*!
 * \file ttc_error.cpp
 * \brief Names of the error codes, for logs and for the command line front end.
 */

#include "ttc2sfnt/ttc_error.h"

namespace ttc2sfnt
{
	TTC2SFNT_HEAD_ONLY_INLINE const char* error_name(int code)
	{
		switch (code)
		{
		case err_none: return "err_none";
		case err_open_failed: return "err_open_failed";
		case err_read_failed: return "err_read_failed";
		case err_write_failed: return "err_write_failed";
		case err_mkdir_failed: return "err_mkdir_failed";
		case err_not_a_collection: return "err_not_a_collection";
		case err_unsupported_version: return "err_unsupported_version";
		case err_empty_collection: return "err_empty_collection";
		case err_bad_face_count: return "err_bad_face_count";
		case err_truncated: return "err_truncated";
		case err_bad_face: return "err_bad_face";
		case err_bad_table_count: return "err_bad_table_count";
		case err_out_of_range: return "err_out_of_range";
		default: return "err_unknown";
		}
	}
}
