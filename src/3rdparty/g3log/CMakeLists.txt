# ==========================================================================
# 2010 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
# with no warranties. This code is yours to share, use and modify with no
# strings attached and no restrictions or obligations.
#
# For more information see g3log/LICENSE or refer refer to http://unlicense.org
# ============================================================================*/

cmake_minimum_required (VERSION 3.0)
ENABLE_LANGUAGE(CXX)

project (g3log)

use_cxx11()

# ============================================================================
# G3LOG OPTIONAL FEATURES
# ============================================================================
INCLUDE (${g3log_SOURCE_DIR}/Options.cmake)

# =========================================================================
# G3 Macro definitions in Options.cmake are written to file
#    this avoids having to re-state your definitions in your source code
#    or compile options
#==========================================================================
INCLUDE (${g3log_SOURCE_DIR}/GenerateMacroDefinitionsFile.cmake)

# =========================================================================
# G3LOG BUILD
#==========================================================================
INCLUDE (${g3log_SOURCE_DIR}/Build.cmake)
