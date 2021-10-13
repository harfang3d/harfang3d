// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

namespace hg {

/// Return true is the provided character is uppercase.
bool IsUpperCase(const char s);
/// Return whether a constant is float or int.
bool IsFloatConstant(const char *s, const char *e);
/// Find a character inside an ASCII block.
const char *Find(const char *s, const char *e, char f, bool skip_parenthesis = true);
/// Run to end of string.
const char *RunToEOS(const char *s, const char *e);
/// Run to end of line marker.
const char *RunToEOL(const char *s, const char *e);
/// Skip end of line marker.
const char *SkipEOL(const char *s, const char *e);
/**
    @short Run to the end of a group started with character 'op' closed with character 'cl'.
    @note This function automatically skips nested group of the same kind.
*/
const char *RunToEOG(const char *s, const char *e, char op, char cl);
/// Run to the end of a C comment block (/*...*/).
const char *RunToEOC(const char *s, const char *e);
/// Run to the end of a C expression.
const char *RunToEOE(const char *s, const char *e);
/// Skip non-{space/tab/comments} string.
const char *SkipEntry(const char *s, const char *e, bool skip_minus = false);
/// Skip spaces.
const char *SkipSpace(const char *s, const char *e);
/// Go to the next entry.
const char *NextEntry(const char *s, const char *e, bool skip_minus = false);

} // namespace hg
