/**
 * @file   python_fix.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.08
 *
 * @brief  Fixes the python include problem that a debug library is required.
 */

#pragma once

// Taken from pybind11: https://github.com/pybind/pybind11/blob/f7bc18f528bb35cd06c93d0a58c17e6eea3fa68c/include/pybind11/detail/common.h#L98 [8/8/2018 Sebastian Maisch]
 /// Include Python header, disable linking to pythonX_d.lib on Windows in debug mode
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable: 4510 4610 4512 4005)
#  if defined(_DEBUG)
#    define PYTHON_DEBUG_MARKER
#    undef _DEBUG
#  endif
#endif

#include <Python.h>

#if defined(_MSC_VER)
#  if defined(PYTHON_DEBUG_MARKER)
#    define _DEBUG
#    undef PYTHON_DEBUG_MARKER
#  endif
#  pragma warning(pop)
#endif
