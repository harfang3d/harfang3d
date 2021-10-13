import xml.etree.ElementTree as ETree
from collections import OrderedDict


escaped_symbol_patterns = OrderedDict([
	('|=', '_ior_op_'),
	('&=', '_iand_op_'),
	('+=', '_iadd_op_'),
	('-=', '_isub_op_'),
	('*=', '_imul_op_'),
	('/=', '_idiv_op_'),
	('::', '_namespace_'),
	(':', '_col_'),
	('<=', '_le_'),
	('>=', '_ge_'),
	('==', '_eq_'),
	('!=', '_neq_'),
	('+', '_add_'),
	('-', '_sub_'),
	('*', '_mul_'),
	('/', '_div_'),
	('<', '_lt_'),
	('>', '_gt_'),
	('=', '_copy_'),
	('.', '_dot_'),
])


def escape_symbol(symbol):
	for pattern, escaped in escaped_symbol_patterns.items():
		symbol = symbol.replace(pattern, escaped)
	if symbol.endswith('_'):
		symbol = symbol[:-1]
	return symbol


# list of UIDs we do not want to include in the documentation
# TODO load from a file
blacklist = [
	'string',
	'SurfaceShader_Create',
	'Node_SetUid',
]


api_tag_symbols = [
	'class',
	'function',
	'enum',
	'constants'
]


api = None
api_parent_map = None


def load_tag(tag):
	for child in tag:
		if child.tag in api_tag_symbols:
			uid = child.get('uid')
			if uid and uid not in blacklist:
				if uid not in api:
					api[uid] = []
				api[uid].append(child)
				load_tag(child)


def load_api(path):
	with open(path, "r") as file:
		xml_root = ETree.fromstring(file.read())

	global api
	api = OrderedDict()
	load_tag(xml_root)

	global api_parent_map
	api_parent_map = {c: p for p in xml_root.iter() for c in p}
