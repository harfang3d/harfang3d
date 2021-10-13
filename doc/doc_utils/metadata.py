from pypeg2 import *


# --
number = re.compile(r"-?([0-9]+\.([0-9]+)?f|\.[0-9]+(e[0-9]+)?f?|[0-9]+(e[0-9]+)?f?)")
string = re.compile(r"\"[^\"\\\r\n]*(?:\\.[^\"\\\r\n]*)*\"")
link = re.compile(r"\[(\w|\.)+\]")


# --Sample Text
class DirectiveParameter:
	grammar = name(), optional("=", attr("value", [number, Literal, string, link]))


class DirectiveParameters(Namespace):
	grammar = csl(DirectiveParameter)


# --
class DirectiveName(Plain):
	grammar = contiguous('.', name())


class Directive(Plain):
	"""Recursively parse a single or multi line directive declaration."""
	@staticmethod
	def parse(parser, text, pos):
		o = Directive()

		try:
			text, name = parser.parse(text, DirectiveName)
		except SyntaxError:
			return text, parser.generate_syntax_error("expecting DirectiveName", pos)
		setattr(o, 'name', name.name)

		# short form directives .end is implicit
		short_form_directives = ['img']
		is_short_form = name.name in short_form_directives

		# grab directive parameters
		if text[:1] == '(':
			eop = text.find(')')
			if eop != -1:
				try:
					# multiple parameters
					t, r = parser.parse(text[1:eop], DirectiveParameters)
					setattr(o, 'parms', r)
					text = text[eop+1:]
				except SyntaxError:
					# single string parameter
					if text[1:2] == '"' and text[eop-1:eop] == '"':
						setattr(o, 'parm', text[2:eop-1])

				if is_short_form:
					result = (text[eop+1:], o)

		if not is_short_form:
			# test for multi line directive
			endmarker = ".end%s" % name.name

			end = text.find(endmarker)
			if end == -1:
				end = text.find('\n')

				if end == -1:
					content = text  # last single line directive
					result = ('', o)
				else:
					content = text[:end]  # single line directive
					result = (text[end + 1:], o)
			else:
				content = text[:end]  # multi line directive
				result = (text[end + len(endmarker):], o)

			# recursive directive content parse
			t, content = parser.parse(content, MetaData)

			setattr(o, 'content', content)  # multi line directive

		return result


class TextBlock(Plain):
	@staticmethod
	def parse(parser, text, pos):
		if text == '':
			return text, parser.generate_syntax_error("end of file!", pos)

		o = TextBlock()

		# accept anything until we hit something looking like a directive declaration
		pos, end = 0, len(text)
		while pos < end:
			# skip to end of line
			endl = text.find('\n', pos)

			if endl != -1:
				pos = endl + 1  # skip over the end of line

				# test directive
				try:
					parser.parse(text[pos:], DirectiveName)
					break  # we hit a directive, end block here!
				except SyntaxError:
					pass  # not a directive, proceed...
			else:
				pos = end

		setattr(o, 'content', text[0:pos])
		return text[pos:], o

	def __str__(self):
		return self.content


# --
class MetaData(List):
	grammar = maybe_some([Directive, TextBlock])


metadata_parse_cache = {}


def parse_metadata(data):
	if data in metadata_parse_cache:
		return metadata_parse_cache[data]

	output = parse(data, MetaData)

	metadata_parse_cache[data] = output
	return output
