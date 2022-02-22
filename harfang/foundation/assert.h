// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

namespace hg {

/// Trigger a system assert error (may be swapped by a platform specialized version).
extern void (*trigger_assert)(const char *source, int line, const char *function, const char *condition, const char *message);

} // namespace hg

//#define ALWAYS_ENABLE_ASSERT

#if defined(ALWAYS_ENABLE_ASSERT) || defined(_DEBUG) || defined(MIXED_RELEASE)

#define __ASSERT__(_EXP_)                                                                                                                                      \
	do {                                                                                                                                                       \
		if (!(_EXP_))                                                                                                                                          \
			hg::trigger_assert(__FILE__, __LINE__, __FUNCTION__, #_EXP_, nullptr);                                                                             \
	} while (0)

#define __ASSERT_IF__(_CND_, _EXP_)                                                                                                                            \
	do {                                                                                                                                                       \
		if (_CND_)                                                                                                                                             \
			if (!(_EXP_))                                                                                                                                      \
				hg::trigger_assert(__FILE__, __LINE__, __FUNCTION__, #_EXP_, nullptr);                                                                         \
	} while (0)

#define __ASSERT_MSG__(_EXP_, _MSG_)                                                                                                                           \
	do {                                                                                                                                                       \
		if (!(_EXP_))                                                                                                                                          \
			hg::trigger_assert(__FILE__, __LINE__, __FUNCTION__, #_EXP_, _MSG_);                                                                               \
	} while (0)

#define __ASSERT_ALWAYS__(_MSG_) hg::trigger_assert(__FILE__, __LINE__, __FUNCTION__, "ALWAYS", _MSG_)

#else

#define __ASSERT__(_EXP_) false
#define __ASSERT_IF__(_CND_, _EXP_) false
#define __ASSERT_MSG__(_EXP_, _MSG_) false
#define __ASSERT_ALWAYS__(_MSG_) false

#endif

#define __RASSERT__(_EXP_)                                                                                                                                     \
	do {                                                                                                                                                       \
		if (!(_EXP_))                                                                                                                                          \
			hg::trigger_assert(__FILE__, __LINE__, __FUNCTION__, #_EXP_, nullptr);                                                                             \
	} while (0)

#define __RASSERT_MSG__(_EXP_, _MSG_)                                                                                                                          \
	do {                                                                                                                                                       \
		if (!(_EXP_))                                                                                                                                          \
			hg::trigger_assert(__FILE__, __LINE__, __FUNCTION__, #_EXP_, _MSG_);                                                                               \
	} while (0)
