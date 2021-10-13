// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/parser.h"
#include "foundation/cext.h"
#include "foundation/math.h"

namespace hg {

bool IsUpperCase(const char c) { return asbool(c >= 'A' && c <= 'Z'); }

const char *RunToEOS(const char *s, const char *e) {
	while (s < e) {
		if (s[0] == '\\')
			s += 2; // jump modifiers
		else if (s[0] == '"')
			break;
		else
			s++;
	}
	return s;
}

bool IsFloatConstant(const char *s, const char *e) {
	const char *eoc = SkipEntry(s, e);
	if (s[0] == '-') {
		s++;
		eoc = SkipEntry(s, e);
	}
	while (s < eoc) {
		if (s[0] == '.' || s[0] == 'f')
			return true;
		s++;
	}
	return false;
}

const char *Find(const char *s, const char *e, char f, bool skip_parenthesis) {
	for (;;) {
		s = SkipSpace(s, e);
		if (s == e)
			return nullptr;
		if (skip_parenthesis && (s[0] == '('))
			s = RunToEOG(s, e, '(', ')');
		else {
			if (s[0] == f)
				break;
			s++;
		}
	}
	return s;
}

const char *SkipEntry(const char *s, const char *e, bool skip_minus) {
	{
		while (
			s[0] != 0x20 &&
			!(s[0] == 0xd && s[1] == 0xa) &&
			s[0] != 0x9 &&
			s[0] != 0xa &&
			s[0] != 0xd &&
			s[0] != '/' &&
			s[0] != '*' &&
			s[0] != '+' &&
			s[0] != '=' &&
			s[0] != ';' &&
			s[0] != ':' &&
			s[0] != ',' &&
			s[0] != '<' &&
			s[0] != '>' &&
			s[0] != '(' &&
			s[0] != ')' &&
			s[0] != '[' &&
			s[0] != ']' &&
			s[0] != '\"') {
			if (!skip_minus && s[0] == '-')
				break;
			if (s == e)
				break;
			++s;
		}
	}
	return s;
}

const char *RunToEOL(const char *s, const char *e) {
	while (
		(s[0] != 0xd || s[1] != 0xa) &&
		s[0] != 0xa &&
		s[0] != 0xd &&
		s < e)
		++s;

	return s;
}

const char *SkipEOL(const char *s, const char *e) {
	if (s[0] == 0xd && s[1] == 0xa)
		s += 2;
	else if (s[0] == 0xa || s[0] == 0xd)
		++s;
	return s > e ? e : s;
}

const char *RunToEOG(const char *s, const char *e, char op, char cl) {
	uint32_t pc = 0;
	s++;
	while (s < e) {
		if (s[0] == op)
			pc++;
		if (s[0] == cl) {
			if (!pc)
				break;
			pc--;
		}
		s++;
	}
	if (s == e)
		return nullptr;
	return s;
}

const char *RunToEOC(const char *s, const char *e) {
	s += 2;
	while ((s[0] != '*' || s[1] != '/') && s < e)
		s += s[0] == 0xd && s[1] == 0xa ? 2 : 1;
	return s >= e ? e : s + 2;
}

const char *RunToEOE(const char *s, const char *e) {
	while (s < e) {
		s = SkipSpace(s, e);
		if (s[0] == '(')
			s = RunToEOG(s, e, '(', ')');

		else if (s[0] == '\"') {
			s++;
			while (s < e && s[0] != '\"')
				s++;
			if (s < e)
				s++;
		} else {
			if (s[0] == ',' || s[0] == ';')
				break;
			s++;
		}
	}
	return s;
}

const char *SkipSpace(const char *s, const char *e) {
	while (s < e) {
		if (s[0] == 0x20)
			++s;
		else if (s[0] == 0xd && s[1] == 0xa)
			s += 2;
		else if (s[0] == '/' && s[1] == '/')
			s = RunToEOL(s, e);
		else if (s[0] == '/' && s[1] == '*')
			s = RunToEOC(s, e);
		else if (s[0] == 0x9)
			++s;
		else if (s[0] == 0xa)
			++s;
		else if (s[0] == 0xd)
			++s;
		else
			break;
	}
	return s;
}

const char *NextEntry(const char *s, const char *e, bool skip_minus) {
	s = SkipEntry(s, e, skip_minus);
	s = SkipSpace(s, e);
	return s;
}

} // namespace hg
