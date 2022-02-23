import os
import re
import argparse
import shutil

import xml.etree.ElementTree as ETree
import doc_utils.doc_tools as doc_tools
import doc_utils.api_tools as api_tools

from functools import partial


api = None

classes = None
functions = None
constants = None
enums = None


#
def now_as_iso_datetime():
	#return datetime.datetime.now().isoformat()
	return '2020-10-05T08:48:40+00:00'


#
man_pages_cache = {}
man_pages_spacing = []

def parse_man_page(path):
	with open(os.path.join(args.doc, path + '.md')) as md:
		md_lines = md.readlines()

	man_page = {'lines': []}

	for line in md_lines:
		if line.startswith('.title '):
			man_page['title'] = line[7:].strip()
		elif line.strip() == '[TOC]':
			pass  # ignore TOC
		else:
			man_page['lines'].append(line)

	while (len(man_page['lines']) > 0) and (man_page['lines'][0] == '\n'):
		man_page['lines'].pop(0)

	man_pages_cache[path] = man_page


#
def format_natural_list(vals):
	if len(vals) == 1:
		return vals[0]
	return '%s and %s' % (', '.join(vals[:-1]), vals[-1])


#
unresolved_links = []

def report_unresolved_links():
	if len(unresolved_links) > 0:
		print('Unresolved links (%d):' % len(unresolved_links))
		for link in unresolved_links:
			print('  - %s' % link)


link_re = re.compile('\[[^\[\]]+\]')

def get_hardcoded_types(lang):
	if lang == 'cpython':
		return {
			'IntPtr': 'pointer',
			'void': '[None](https://docs.python.org/3/library/stdtypes.html)',
			'bool': '[bool](https://docs.python.org/3/library/stdtypes.html)',
			'int': '[int](https://docs.python.org/3/library/stdtypes.html)',
			'Int8' : '[int](https://docs.python.org/3/library/stdtypes.html)',
			'Int16' : '[int](https://docs.python.org/3/library/stdtypes.html)',
			'Int32' : '[int](https://docs.python.org/3/library/stdtypes.html)',
			'Int64' : '[int](https://docs.python.org/3/library/stdtypes.html)',
			'UInt8' : '[int](https://docs.python.org/3/library/stdtypes.html)',
			'UInt16' : '[int](https://docs.python.org/3/library/stdtypes.html)',
			'UInt32' : '[int](https://docs.python.org/3/library/stdtypes.html)',
			'UInt64' : '[int](https://docs.python.org/3/library/stdtypes.html)',
			'float': '[float](https://docs.python.org/3/library/stdtypes.html)',
			'size_t' : '[int](https://docs.python.org/3/library/stdtypes.html)',
			'string' : '[str](https://docs.python.org/3/library/stdtypes.html)' }

	if lang == 'lua':
		return {
			'IntPtr': 'pointer',
			'void': '[nil](https://www.lua.org/manual/5.3/manual.html)',
			'bool': '[boolean](https://www.lua.org/manual/5.3/manual.html)',
			'int': '[int](https://www.lua.org/manual/5.3/manual.html)',
			'Int8' : '[int](https://www.lua.org/manual/5.3/manual.html)',
			'Int16' : '[int](https://www.lua.org/manual/5.3/manual.html)',
			'Int32' : '[int](https://www.lua.org/manual/5.3/manual.html)',
			'Int64' : '[int](https://www.lua.org/manual/5.3/manual.html)',
			'UInt8' : '[int](https://www.lua.org/manual/5.3/manual.html)',
			'UInt16' : '[int](https://www.lua.org/manual/5.3/manual.html)',
			'UInt32' : '[int](https://www.lua.org/manual/5.3/manual.html)',
			'UInt64' : '[int](https://www.lua.org/manual/5.3/manual.html)',
			'float': '[number](https://www.lua.org/manual/5.3/manual.html)',
			'size_t' : '[int](https://www.lua.org/manual/5.3/manual.html)',
			'string' : '[string](https://www.lua.org/manual/5.3/manual.html)' }

def link_formatter(lang, link):
	match = link.string[link.regs[0][0]:link.regs[0][1]]
	raw = match[1:-1]

	hardcoded_types = get_hardcoded_types(lang)

	if raw in hardcoded_types:
		return hardcoded_types[raw]

	if match.startswith('[man.'):
		man_page = man_pages_cache[raw]
		return '[%s]({{< relref "/docs/%s/%s.md" >}})' % (man_page['title'], args.version, raw)

	for fn in functions:
		if fn.attrib['name'] == raw:
			return '[%s]({{< relref "/api/%s/%s/functions.md#%s" >}})' % (fn.attrib['name'], args.version, lang, raw.lower())

	for cl in classes:
		if cl.attrib['name'] == raw:
			return '[%s]({{< relref "/api/%s/%s/classes.md#%s" >}})' % (cl.attrib['name'], args.version, lang, raw.lower())

		for cc in cl:
			if cc.tag == 'function' and cc.attrib['uid'] == raw:
				return '[%s.%s]({{< relref "/api/%s/%s/classes.md#%s" >}})' % (cl.attrib['name'], cc.attrib['name'], args.version, lang, cl.attrib['name'].lower())  # TODO redirect to the method (we have no anchor atm)

	for e in enums:
		if e.attrib['name'] == raw:
			return '[%s]({{< relref "/api/%s/%s/constants.md#%s" >}})' % (raw, args.version, lang, e.attrib['name'].lower())

		for v in e:
			if v.attrib['name'] == raw:
				return '[%s]({{< relref "/api/%s/%s/constants.md#%s" >}})' % (raw, args.version, lang, e.attrib['name'].lower())

	for e in constants:
		if e.attrib['name'] == raw:
			return '[%s]({{< relref "/api/%s/%s/constants.md#%s" >}})' % (raw, args.version, lang, e.attrib['name'].lower())

		for v in e:
			if v.attrib['name'] == raw:
				return '[%s]({{< relref "/api/%s/%s/constants.md#%s" >}})' % (raw, args.version, lang, e.attrib['name'].lower())

	if raw not in unresolved_links:
		unresolved_links.append(raw)

	return match

def process_lines_links(lang, in_lines):
	return [link_re.sub(partial(link_formatter, lang), in_line) for in_line in in_lines]

def process_links(lang, input):
	return link_re.sub(partial(link_formatter, lang), input)

def remove_links(input):
	def mofo(link):
		return link.string[link.regs[0][0]:link.regs[0][1]][1:-1]

	return link_re.sub(mofo, input)


#
def gather_uid_function_links(uid):
	links = []

	for fn in functions:
		if fn.attrib['uid'] != uid:
			if fn.attrib['returns'] == uid:
				links.append('[%s]' % fn.attrib['uid'])
			elif 'returns_constants_group' in fn.attrib and fn.attrib['returns_constants_group'] == uid:
				links.append('[%s]' % fn.attrib['uid'])
			else:
				for parm in fn:
					if (parm.attrib['type'] == uid) or ('constants_group' in parm.attrib and parm.attrib['constants_group'] == uid):
						links.append('[%s]' % fn.attrib['uid'])
						break

	return sorted(list(set(links)))

def get_uid_doc(uid):
	if uid in doc_tools.man:
		return doc_tools.man[uid]
	if uid in doc_tools.doc:
		return doc_tools.doc[uid]
	return ''

def gather_uid_class_links(uid):
	links = []

	for cl in classes:
		if cl.attrib['uid'] != uid:
			for c in cl:
				if c.tag == 'variable':
					if c.attrib['type'] == uid:
						links.append('[%s]' % cl.attrib['uid'])
				elif c.tag == 'function':
					if c.attrib['returns'] == uid:
						links.append('[%s]' % cl.attrib['uid'])
					else:
						for parm in c:
							if (parm.attrib['type'] == uid) or ('constants_group' in parm.attrib and parm.attrib['constants_group'] == uid):
								links.append('[%s]' % cl.attrib['uid'])
								break

	return sorted(list(set(links)))

def format_related_links(uid):
	cl_links = gather_uid_class_links(uid)
	fn_links = gather_uid_function_links(uid)

	out = ''

	if len(cl_links) > 0:
		out = out +	'\n<small>Related classes: %s.</small>\n' % format_natural_list(cl_links)

	if len(fn_links) > 0:
		out = out +	'\n<small>Related functions: %s.</small>\n' % format_natural_list(fn_links)

	if len(cl_links) > 0 or len(fn_links) > 0:
		out = out + '\n'

	return out


#
def get_api_tags(type):
	out = []
	for e in api:
		if e.tag in type:
			out.append(e)
	return out

def make_api_glossary(tags):
	glossary = {}

	for tag in tags:
		letter = tag.attrib['uid'][0:1].lower()
		if letter not in glossary:
			glossary[letter] = []
		glossary[letter].append(tag)

	for letter, tags in glossary.items():
		glossary[letter] = sorted(tags, key=lambda tag: tag.attrib['name'])

	return glossary


def prepare_proto(proto, lang):
	#
	out_parms = []
	for parm in proto:
		if parm.attrib['name'].startswith('OUTPUT'):  # output parameter
			out_parms.append(parm)

	#
	rvals = []
	if proto.attrib['returns'] == 'void':
		if len(out_parms) == 0:
			rvals.append('[void]')
	else:
		def get_parm_link(parm):
			if 'returns_constants_group' in parm.attrib:
				cg = parm.attrib['returns_constants_group']

				for e in constants:
					if e.attrib['name'] == cg:
						return '[%s]' % cg

			return '[%s]' % parm.attrib['returns']

		rvals.append(get_parm_link(proto))

	#
	def get_parm_link(parm):
		if 'constants_group' in parm.attrib:
			cg = parm.attrib['constants_group']

			for e in constants:
				if e.attrib['name'] == cg:
					return '[%s]' % cg

		return '[%s]' % parm.attrib['type']

	for out_parm in out_parms:
		rvals.append(get_parm_link(out_parm))

	rval = ', '.join(rvals)

	if lang == 'cpython':
		_args = ['_%s:_ %s' % (parm.attrib['name'], get_parm_link(parm)) for parm in proto if not parm.attrib['name'].startswith('OUTPUT')]
	else:
		_args = ['%s _%s_' % (get_parm_link(parm), parm.attrib['name']) for parm in proto if not parm.attrib['name'].startswith('OUTPUT')]

	_args = ', '.join(_args) if len(_args) >  0 else ''

	return rval, _args

def generate_api_classes_page_content(lang):
	out = '''\
---
title : "API Classes"
date: 2020-10-06T08:47:36+00:00
lastmod: 2020-10-06T08:47:36+00:00
draft: false
images: []
layout: single
weight: 10
---
'''

	glossary = make_api_glossary(classes)

	for entry, tags in sorted(glossary.items()):
		for tag in tags:
			name, uid = tag.attrib['name'], tag.attrib['uid']

			out = out + '### %s\n\n' % name

			out = out + get_uid_doc(uid) + '\n'
			out = out + format_related_links(uid)

			members = [child for child in tag if child.tag == 'variable']
			members = sorted(members, key=lambda tag: tag.attrib['name'])

			methods = [child for child in tag if child.tag == 'function']
			methods = sorted(methods, key=lambda tag: tag.attrib['name'])

			if len(members) > 0 or len(methods) > 0:
				out = out + '\n----\n'

			if len(members) > 0:
				out = out + '\n'
				out = out + '| Member | Type |\n'
				out = out + '| - | - |\n'

				for member in members:
					out = out + '| %s | %s[%s] |\n' % (member.attrib['name'], 'static ' if 'static' in member.attrib else '', member.attrib['type'])
					#doc = get_uid_doc(member.attrib['uid'])
					#if doc != '':
					#	out = out + '|| {{< div-table-cell >}}%s{{< /div-table-cell >}} |\n' % doc

				out = out + '<p></p>\n\n'

			if len(methods) > 0:
				out = out + '| Method | Prototype |\n'
				out = out + '| - | - |\n'

				for method in methods:
					rval, args = prepare_proto(method, lang)

					if lang == 'cpython':
						out = out + '| %s | (%s) `->` %s |\n' % (method.attrib['name'], args, rval)  #, doc)
					else:
						out = out + '| %s | %s (%s) |\n' % (method.attrib['name'], rval, args)  #, doc)

					doc = get_uid_doc(method.attrib['uid'])
					if doc != '':
						out = out + '|| {{< div-table-cell >}}%s{{< /div-table-cell >}} |\n' % doc

				out = out + '\n'

	return process_links(lang, out)

def generate_api_functions_page_content(lang):
	out = '''\
---
title : "API Functions"
date: 2020-10-06T08:47:36+00:00
lastmod: 2020-10-06T08:47:36+00:00
draft: false
images: []
layout: single
weight: 20
---
'''

	glossary = make_api_glossary(functions)

	for entry, tags in sorted(glossary.items()):
		protos = {}
		for tag in tags:
			name, uid = tag.attrib['name'], tag.attrib['uid']
			if uid not in protos:
				protos[uid] = []
			protos[uid].append(tag)

		for uid, protos_ in protos.items():
			proto = protos_[0]
			name, uid = proto.attrib['name'], proto.attrib['uid']

			out = out + '\n### %s\n\n' % name

			# function prototypes
			out = out + '| | |\n'
			out = out + '| - | - |\n'

			for proto in protos_:
				rval, args = prepare_proto(proto, lang)
				if lang == 'cpython':
					out = out + '|| (%s) `->` %s |\n' % (args, rval)
				else:
					out = out + '| %s | (%s) |\n' % (rval, args)

			out = out + '----\n'

			#
			doc = doc_tools.get_content_always(uid)
			doc = '\n'.join(list(filter(lambda line : not line.startswith('.proto'), doc.splitlines())))
			out = out + doc.strip() + '\n'

	return process_links(lang, out)

def generate_api_constants_page_content(lang):
	out = '''\
---
title : "API Constants"
date: 2020-10-06T08:47:36+00:00
draft: false
images: []
layout: single
weight: 40
---
'''

	glossary = make_api_glossary(constants + enums)

	for entry, tags in sorted(glossary.items()):
		for tag in tags:
			name, uid = tag.attrib['name'], tag.attrib['uid']

			out = out + '### %s\n\n' % name
			for entry in tag:
				out = out + '- %s\n' % entry.attrib['name']
			out = out + '\n'

			out = out + format_related_links(uid)

	return process_links(lang, out)


#
def convert(api, doc, out):
	# prepare man pages
	man_pages = []

	with open(os.path.join(doc, 'tree_desc.txt')) as tree_desc:
		for page in tree_desc.readlines():
			page = page.strip()
			if page == '':
				man_pages_spacing.append(man_pages[-1])
			else:
				man_pages.append(page)

	for page in man_pages:
		parse_man_page(page)

	# output docs/ pages
	docs_out_path = os.path.join(out, 'docs', args.version)
	os.makedirs(docs_out_path)

	with open(os.path.join(docs_out_path, '_index.md'), mode='w', encoding='utf-8') as index:
		index.write('''\
---
title : "%s"
date: %s
lastmod: %s
draft: false
---\
''' % (args.version, now_as_iso_datetime(), now_as_iso_datetime()))

	weight = 10

	for page in man_pages:
		man_page = man_pages_cache[page]

		out_md_path = os.path.join(docs_out_path, page + '.md')
		with open(out_md_path, 'w', encoding='utf-8') as md:
			# output front matter
			md.write('''\
---
title: "%s"
date: %s
draft: false
weight: %d
toc: true
''' % (man_page['title'], now_as_iso_datetime(), weight))

			if page in man_pages_spacing:
				md.write('spacing: true\n')

			md.write('---\n')

			# processed content
			md_lines = process_lines_links('cpython', man_page['lines'])
			md_lines = [line.replace('${HG_VERSION}', args.version) for line in md_lines]
			md.writelines(md_lines)

		weight = weight + 10

	# output API
	api_out_path = os.path.join(out, 'api', args.version)

	for lang in ['cpython', 'lua']:
		os.makedirs(os.path.join(api_out_path, lang, 'classes'))
		with open(os.path.join(api_out_path, lang, 'classes', 'index.md'), 'w', encoding='utf-8') as file:
			file.write(generate_api_classes_page_content(lang))

		os.makedirs(os.path.join(api_out_path, lang, 'functions'))
		with open(os.path.join(api_out_path, lang, 'functions', 'index.md'), 'w', encoding='utf-8') as file:
			file.write(generate_api_functions_page_content(lang))

		os.makedirs(os.path.join(api_out_path, lang, 'constants'))
		with open(os.path.join(api_out_path, lang, 'constants', 'index.md'), 'w', encoding='utf-8') as file:
			file.write(generate_api_constants_page_content(lang))

	# output search database
	with open(os.path.join(api_out_path, 'search.json'), 'w', encoding='utf-8') as file:
		def first_statement(s: str) -> str:
			statements = s.split('. ');
			return statements[0] if len(statements) > 1 else s

		def get_doc(uid: str) -> list:
			doc = get_uid_doc(uid)
			lines = list(filter(lambda line : not line.startswith('.proto'), doc.splitlines()))
			return lines

		def sanitize(s : str) -> str:
			return remove_links(s.replace('"', "'"))

		def get_desc(uid: str) -> str:
			doc = get_doc(uid)
			if len(doc) == 0:
				return '-'
			return sanitize(first_statement(doc[0])).strip()

		id = 0

		cls = {}
		for cl in classes:
			cls[cl.attrib['name']] = cl.attrib['uid']

		json_content = []

		for name, uid in cls.items():
			json_content.append('''\
{
	"id": %d,
	"href": "../classes/#%s",
	"title": "%s",
	"description": %s
}''' % (id, name.lower(), name, '"%s"' % get_desc(uid)))
			id = id + 1

		fns = {}
		for fn in functions:
			fns[fn.attrib['name']] = fn.attrib['uid']

		for name, uid in fns.items():
			json_content.append('''\
{
	"id": %d,
	"href": "../functions/#%s",
	"title": "%s",
	"description": %s
}''' % (id, name.lower(), name, '"%s"' % get_desc(uid)))
			id = id + 1

		csts = {}
		for cst in enums:
			csts[cst.attrib['name']] = cst.attrib['uid']
		for cst in constants:
			csts[cst.attrib['name']] = cst.attrib['uid']

		for name, uid in csts.items():
			json_content.append('''\
{
	"id": %d,
	"href": "../constants/#%s",
	"title": "%s",
	"description": %s
}''' % (id, name.lower(), name, '"%s"' % get_desc(uid)))
			id = id + 1

		for cl in classes:
			class_name = cl.attrib['name']
			for cc in cl:
				if cc.tag == 'function':
					name, uid = cc.attrib['name'], cc.attrib['uid']
					json_content.append('''\
{
	"id": %d,
	"href": "../classes/#%s",
	"title": "%s",
	"description": %s
}''' % (id, class_name.lower(), name, '"%s"' % get_desc(uid)))
					id = id + 1

		file.write('[\n')
		file.write(',\n'.join(json_content))
		file.write(']\n')


#
if __name__ == '__main__':
	parser = argparse.ArgumentParser(description="Convert the internal documentation format to Hugo")
	parser.add_argument('--api', type=str, help="API path", required=True)
	parser.add_argument('--doc', type=str, help="Documentation path", required=True)
	parser.add_argument('--out', type=str, help="Output folder (hugo content/ folder)", required=True)
	parser.add_argument('--version', type=str, help="Documentation version", required=True)
	args = parser.parse_args()

	static_img = os.path.join(args.out, '..', 'static', 'images', 'docs', args.version)
	shutil.copytree('img', static_img)

	with open(args.api, "r") as file:
		api = ETree.fromstring(file.read())

	api_tools.load_api(args.api)
	doc_tools.load_doc_folder(args.doc)

	classes = get_api_tags(['class'])
	functions = get_api_tags(['function'])
	constants = get_api_tags(['constants'])
	enums = get_api_tags(['enum'])

	convert(args.api, args.doc, args.out)

	report_unresolved_links()
