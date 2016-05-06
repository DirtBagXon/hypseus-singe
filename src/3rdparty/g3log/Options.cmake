# g3log is a KjellKod Logger
# 2015 @author Kjell Hedström, hedstrom@kjellkod.cc 
# ==================================================================
# 2015 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own
#    risk and comes  with no warranties.
#
# This code is yours to share, use and modify with no strings attached
#   and no restrictions or obligations.
# ===================================================================


# PLEASE NOTE THAT:
# the following definitions can through options be added 
# to the auto generated file src/g3log/generated_definitions.hpp
#   add_definitions(-DG3_DYNAMIC_LOGGING)
#   add_definitions(-DCHANGE_G3LOG_DEBUG_TO_DBUG)
#   add_definitions(-DDISABLE_FATAL_SIGNALHANDLING)
#   add_definitions(-DDISABLE_VECTORED_EXCEPTIONHANDLING)
#   add_definitions(-DDEBUG_BREAK_AT_FATAL_SIGNAL)

# Used for generating a macro definitions file  that is to be included
# that way you do not have to re-state the Options.cmake definitions when 
# compiling your binary (if done in a separate build step from the g3log library)
SET(G3_DEFINITIONS "")

SET(USE_DYNAMIC_LOGGING_LEVELS ON)
LIST(APPEND G3_DEFINITIONS G3_DYNAMIC_LOGGING)
SET(CHANGE_G3LOG_DEBUG_TO_DBUG ON)
LIST(APPEND G3_DEFINITIONS CHANGE_G3LOG_DEBUG_TO_DBUG)
SET(ENABLE_FATAL_SIGNALHANDLING ON)

IF(NOT ENABLE_FATAL_SIGNALHANDLING)
	LIST(APPEND G3_DEFINITIONS DISABLE_FATAL_SIGNALHANDLING)
ENDIF(NOT ENABLE_FATAL_SIGNALHANDLING)

# WINDOWS OPTIONS
IF (MSVC OR MINGW) 
	SET(ENABLE_VECTORED_EXCEPTIONHANDLING ON)

	IF(NOT ENABLE_VECTORED_EXCEPTIONHANDLING)
		LIST(APPEND G3_DEFINITIONS DISABLE_VECTORED_EXCEPTIONHANDLING)
	ENDIF(NOT ENABLE_VECTORED_EXCEPTIONHANDLING)

	SET(DEBUG_BREAK_AT_FATAL_SIGNAL ON)
	IF(DEBUG_BREAK_AT_FATAL_SIGNAL)
		LIST(APPEND G3_DEFINITIONS DEBUG_BREAK_AT_FATAL_SIGNAL)
	ENDIF(DEBUG_BREAK_AT_FATAL_SIGNAL)

ENDIF (MSVC OR MINGW)
