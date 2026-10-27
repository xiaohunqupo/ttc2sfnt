#pragma once

/*!
 * \file ttc2sfnt.h
 * \brief Umbrella header: pulls in the collection parser, the face rebuild and the extraction
 *        helpers.
 *
 * `ttc2sfnt` splits a TrueType Collection (`.ttc`) into standalone fonts (`.ttf` / `.otf`). A
 * collection stores several faces in one file and lets them share table data; most font consumers -
 * `ttf2mesh` among them - accept a single face only, so the shared tables have to be copied out and
 * the checksums recomputed before they can be used. See `docs/ttc_format.md` for the byte layout and
 * `README.md` for the usage.
 */

/*!
 * \name Version
 * Major / minor / patch number of the library, currently 0.5.0. The numbers are maintained here by
 * hand - the build system only reads them - and `CHANGELOG.md` records what changed in each version.
 * Bump them together with a new `CHANGELOG.md` entry.
 */
///@{
#define TTC2SFNT_VERSION_MAJOR 0 //!< Incompatible API changes.
#define TTC2SFNT_VERSION_MINOR 5 //!< New features, backwards compatible.
#define TTC2SFNT_VERSION_PATCH 0 //!< Fixes and internal changes.
#define TTC2SFNT_VERSION_STRING "0.5.0" //!< The three numbers as one string, for logs and `--version`.
///@}

#include "ttc2sfnt/ttc2sfnt_config.h"
#include "ttc2sfnt/ttc_error.h"
#include "ttc2sfnt/ttc_file.h"
#include "ttc2sfnt/ttc_sfnt.h"
#include "ttc2sfnt/ttc_name.h"
#include "ttc2sfnt/ttc_collection.h"
#include "ttc2sfnt/ttc_extract.h"
