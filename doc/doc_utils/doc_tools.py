import xml.etree.ElementTree as ETree
import doc_utils.api_tools as api_tools
from doc_utils.metadata import parse_metadata, MetaData, Directive
from collections import OrderedDict
import os


# ------------------------------------------------------------------------------
builtin_types = {
	'void': 'void',
	'bool': 'bool',
	'char': 'int',
	'short': 'int',
	'int': 'int',
	'long': 'int',
	'uchar': 'int',
	'ushort': 'int',
	'uint': 'int',
	'uint8_t': 'int',
	'uint16_t': 'int',
	'uint32_t': 'int',
	'int8_t': 'int',
	'int16_t': 'int',
	'int32_t': 'int',
	'Int8': 'int',
	'Int16': 'int',
	'Int32': 'int',
	'Int64': 'int',
	'Char16': 'int',
	'Char32': 'int',
	'UInt8': 'int',
	'UInt16': 'int',
	'UInt32': 'int',
	'UInt64': 'int',
	'IntPtr': 'int',
	'size_t': 'int',
	'ptrdiff_t': 'int',
	'float': 'float',
	'double': 'float',
	'string': 'string',
}


def format_array_type(uid):
	return "[%s]" % format_uid_link(uid[7:-1])


def format_list_type(uid):
	return "[%s]" % format_uid_link(uid[6:-1])


dynamic_types = {
	'$array': format_array_type,
	'$list': format_list_type,
}


def format_uid_link(uid):
	# dynamic type
	for pfx, fmt in dynamic_types.items():
		if uid.startswith(pfx):
			return fmt(uid)

	# builtin type
	if uid in builtin_types:
		uid = builtin_types[uid]

	# manual page
	if uid in man or uid in api_tools.api:
		return '[%s]' % uid

	return uid  # nothing special


# ------------------------------------------------------------------------------
def get_all_classes():
	return sorted([uid for uid, tag in api_tools.api.items() if tag[0].tag == 'class'])


# ------------------------------------------------------------------------------
def list_to_natural_string(l, separator):
	return (' %s ' % separator).join([', '.join(l[:-1]), l[-1]]) if len(l) > 1 else l[0]


inheritance = None


def build_class_inheritance():
	global inheritance
	if inheritance is None:
		inheritance = {}
		for uid, t in api_tools.api.items():
			tag = t[0]
			if tag.tag == 'class':
				inheritance[uid] = [i.get('uid') for i in tag.iter('inherits')]
	return inheritance


# ------------------------------------------------------------------------------
default_value_reformat = {
	"nullptr": "None"
}


def format_function_proto_rvalue(tag):
	parms = []

	returns_constants_group = tag.get('returns_constants_group')

	if returns_constants_group:
		parms.append(format_uid_link(returns_constants_group))
	else:
		parms.append(format_uid_link(tag.get('returns', 'void')))

	for parm_tag in tag.iter('parm'):
		parm = format_uid_link(parm_tag.get('type'))

		display_name = parm_tag.get('name')
		if display_name and (str(display_name).startswith("OUTPUT") or str(display_name).startswith("INOUT")):
			parms.append(parm)

	# strip the first entry if it is a void
	if len(parms) > 1 and parms[0] == "void":
		del parms[0]

	return ", ".join(parms)


def format_function_proto_parms(tag):
	parms = []
	for parm_tag in tag.iter('parm'):
		constants_group = parm_tag.get('constants_group')

		if constants_group:
			parm = format_uid_link(constants_group)
		else:
			parm = format_uid_link(parm_tag.get('type'))

		# optional variable name
		display_name = parm_tag.get('name')
		if display_name:
			if display_name.startswith("OUTPUT"):  # skip OUTPUT parameters
				continue

			if display_name.startswith("INOUT"):
				parm += " _%s_" % "v"
			else:
				parm += " _%s_" % display_name

		# optional default value
		value = parm_tag.get('default_value')
		if value:
			if value in default_value_reformat:
				value = default_value_reformat[value]

			# catch c++ floating point values
			if value[-2:] == ".f":
				value = value[:-2] + ".0"

			# replace c++ namespace delimiter
			value = value.replace("::", ".")

			parm += "` = %s`" % value

		parms.append(parm)

	return "(" + ", ".join(parms) + ")\n"


# ------------------------------------------------------------------------------
def generate_api_enum_autodoc(tag):
	text = '# <span class="text-default">%s</span> Enumeration #\n\n' % tag.get('name')
	text += '%EnumConstant%\n'
	return text


def generate_api_class_autodoc(tag):
	text = '# <span class="text-default">%s</span> Class #\n\n' % tag.get('name')
	text += '%ClassInfo%\n'
	text += '%ClassContentIndex%\n'
	text += '%ClassContentDocumentation%\n'
	return text


def generate_api_function_autodoc(tag):
	autodoc = '.proto %s %s' % (format_uid_link(tag.get('returns')), tag.get('name'))
	autodoc += format_function_proto_parms(tag)
	return autodoc


def generate_api_tag_autodoc(uid):
	"""Generate the autodoc for an API element."""
	generators = {
		'enum': generate_api_enum_autodoc,
		'class': generate_api_class_autodoc,
		'function': generate_api_function_autodoc,
	}

	autodoc = ""
	for tag in api_tools.api[uid]:
		if tag.tag in generators:
			autodoc += generators[tag.tag](tag)
	return autodoc


# ------------------------------------------------------------------------------
def gather_manual_links(man_uid):
	"""Retrieve all links to a manual page in a page body"""
	text = man[man_uid]

	uid_links = []
	for uid, value in man.items():
		if text.find("[%s]" % uid) != -1:
			uid_links.append(uid)
	return uid_links


def generate_manual_tree(blacklist=[]):
	parent_score = OrderedDict()
	link_parent = OrderedDict()

	def get_parent_score(link):
		return parent_score[link] if link in parent_score else -1

	for uid, value in man.items():
		if uid in blacklist:
			continue

		links = gather_manual_links(uid)

		for link in links:
			# increase link score
			parent_score[uid] = parent_score.get(uid, 0) + 1
			# build parent lookup
			link_parent[link] = link_parent.get(link, []) + [uid]

	# finalize the parent links
	man_hierarchy = OrderedDict()
	for link, parents in link_parent.items():
		man_hierarchy[link] = sorted(parents, key=get_parent_score, reverse=True)[0]

	return man_hierarchy


def build_man_page_breadcrumb(man_tree, uid):
	breadcrumb = [uid]
	while uid in man_tree:
		uid = man_tree[uid]  # fetch parent
		if uid in breadcrumb:
			break  # prevent loop (A->B->A->B->A->B->...)
		breadcrumb.append(uid)

	# all breadcrumbs should fallback to the documentation root page
	if breadcrumb[-1] != 'man.Index':
		breadcrumb.append('man.Index')

	return breadcrumb


# ------------------------------------------------------------------------------
def get_content_always(uid):
	"""Return the documentation for an element, generate it if necessary."""
	if uid in man:
		return man[uid]
	if uid in doc:
		return doc[uid]
	if uid in api_tools.api:
		return generate_api_tag_autodoc(uid)
	return None


# ------------------------------------------------------------------------------
def parse_element_metadata(uid):
	"""Parse the metadata for a given element from its UID"""
	data = None
	if uid in man:
		data = man[uid]
	if data is None and uid in doc:
		data = doc[uid]

	# TODO cache the parse result
	return None if data is None else parse_metadata(data)


def get_metadata_object_directives(o, directive_name):
	if isinstance(o, MetaData):
		out = []
		for s in o:
			out += get_metadata_object_directives(s, directive_name)
		return out
	elif isinstance(o, Directive):
		if o.name == directive_name:
			return [o]
	return []


def get_element_directives(uid, directive_name):
	o = parse_element_metadata(uid)
	return get_metadata_object_directives(o, directive_name)


def get_element_title(uid):
	"""Return the title of an element from its UID"""
	directives = get_element_directives(uid, 'title')
	if len(directives) == 0:
		return uid
	return str(directives[0].content[0])


# ------------------------------------------------------------------------------
doc, man = OrderedDict(), OrderedDict()


def load_doc_single_file(path):
	with open(path, "r", encoding='utf-8') as file:
		xml_root = ETree.fromstring(file.read())

	# load documented elements
	global doc
	doc = OrderedDict()
	for doc_elm in xml_root.iter('api'):
		uid, data = doc_elm.get('uid'), doc_elm.get('data')
		if uid is not None and data is not None:
			doc[uid] = data.replace("<br>", "\n")

	global man
	man = OrderedDict()
	for man_elm in xml_root.iter('man'):
		uid, data = man_elm.get('uid'), man_elm.get('data')
		if uid is not None and data is not None:
			man[uid] = data.replace("<br>", "\n")


def load_doc_folder(path):
	mds = [os.path.splitext(file)[0] for file in os.listdir(path) if file.endswith(".md")]

	global doc, man
	doc, man = OrderedDict(), OrderedDict()

	for uid in mds:
		md_path = os.path.join(path, uid + ".md")
		with open(md_path, "r", encoding="utf-8") as file:
			data = file.read()

			if uid.startswith('man.'):
				man[uid] = data
			else:
				doc[uid] = data
