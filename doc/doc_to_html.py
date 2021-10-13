import doc_utils.doc_tools as doc_tools
import doc_utils.api_tools as api_tools
import doc_utils.html_tools as html_tools
import search_index
from functools import partial
import argparse
import shutil
import sys
import os
import datetime
from collections import OrderedDict

gen_online_doc = None
version = "0.0.0"

def get_element_output_path(output_path, uid, ext='.html'):
	return os.path.join(output_path, uid + ext)


def html_link_formatter(output_path, uid, is_img_link=False):
	if is_img_link:
		if gen_online_doc:
			return "/documentations/%s/img/%s" % (version, uid)
		return "img/" + uid

	link, name, hint = get_element_output_path('', uid), uid, None

	if uid in doc_tools.man:
		return "[%s](%s)" % (doc_tools.get_element_title(uid), link)

	if uid in api_tools.api:
		tag = api_tools.api[uid][0]
		parent = api_tools.api_parent_map[tag]

		def format_enum_values(tag):
			values = [val.get('name') for val in tag.iter('entry')]
			return doc_tools.list_to_natural_string(values, "or")

		def format_constants_values(tag):
			values = [val.get('name') for val in tag.iter('entry')]
			return doc_tools.list_to_natural_string(values, "and")

		if tag.get("global") == "1":  # global symbol link
			global_type_pages = {"function": "man.Functions.html", "enum": "man.Enums.html", "constants": "man.Constants.html"}
			if tag.tag in global_type_pages:
				link = "%s#%s" % (global_type_pages[tag.tag], uid)
				name = tag.get("name")

				if tag.tag == "enum":
					hint = format_enum_values(tag)
				elif tag.tag == "constants":
					hint = format_constants_values(tag)
		else:
			if parent.tag == "class":
				parent_uid = parent.get("uid")
				link = "%s.html#%s" % (parent_uid, uid)
				name = tag.get("name")
				hint = "%s.%s" % (parent.get("name"), name)

				if tag.tag == "enum":
					hint += ' : ' + format_enum_values(tag)
	if hint:
		path = '<a href="%s" title="%s">%s</a>' % (link, hint, name)
	else:
		path = '<a href="%s">%s</a>' % (link, name)

	return path


def get_header(uid):
	if not gen_online_doc:
		title = 'Documentation'
		if uid in doc_tools.man:
			title = doc_tools.get_element_title(uid)
		return '<h1>%s</h1>' % title

	return ""


def get_footer(uid):
	return '<hr><small id="version-footer">%s %s documentation. Generated %s.</small></hr>\n' % (args.project_name, version, datetime.datetime.today().strftime('%c'))


def format_complete_html_man_page(uid, body, header=get_header, footer=get_footer):
	if not gen_online_doc:
		html = '<html>\n'
		html += '<head><meta charset="UTF-8"><link type="text/css" rel="stylesheet" href="markdown.css"></head>\n'
		html += '<body>\n'
		if header:
			html += header(uid)
		html += body
		if footer:
			html += footer(uid)
		html += '</body>\n'
		html += '</html>\n'
		return html

	html = ''
	if header:
		html += header(uid)
	return html + body


def format_html_link(text, url):
	if text in api_tools.api:
		text = '<span class="text-default">%s</span>' % text
	return '<a href="%s">%s</a>' % (url, text)


def format_html_breadcrumb(breadcrumb):
	# convert from uid to HTML link
	return ''

	if len(breadcrumb) < 2:
		if gen_online_doc:
			return '<div class="doc-breadcrumb"><p></p></div>\n'
		return ''  # do not output empty breadcrumb

	links = []
	# for uid in reversed(breadcrumb[1:]):
	for uid in reversed(breadcrumb):
		title = doc_tools.get_element_title(uid)
		path = get_element_output_path('', uid)
		links.append(format_html_link(title, path))

	# links.append(doc_tools.get_element_title(breadcrumb[0]))

	return '<div markdown=1 class="doc-breadcrumb">%s</div>' % ' &gt; '.join(links)  # offline breadcrumb


def get_class_header(uid):
	breadcrumb = doc_tools.build_man_page_breadcrumb(man_tree, 'man.Classes')
	breadcrumb.insert(0, uid)
	return get_header(uid) + format_html_breadcrumb(breadcrumb)


def get_enum_header(uid):
	breadcrumb = doc_tools.build_man_page_breadcrumb(man_tree, 'man.ScriptReference')
	breadcrumb.insert(0, uid)
	return get_header(uid) + format_html_breadcrumb(breadcrumb)


def get_man_header(uid):
	breadcrumb = doc_tools.build_man_page_breadcrumb(man_tree, uid)
	return get_header(uid) + format_html_breadcrumb(breadcrumb)


get_header_fn = {
	'class': get_class_header,
	'enum': get_enum_header
}


def save_page(output_path, uid, html):
	with open(get_element_output_path(output_path, uid), 'w', encoding='utf-8') as file:
		file.write(html)


# ------------------------------------------------------------------------------
man_tree = None


def build_doc_tree():
	# build the final documentation tree
	tree = OrderedDict()

	def insert_in_tree(uid):
		if uid not in man_tree:  # insert root UID
			if uid not in tree:
				tree[uid] = OrderedDict()
			return tree[uid]

		sub_tree = insert_in_tree(man_tree[uid])
		if uid not in sub_tree:
			sub_tree[uid] = OrderedDict()
		return sub_tree[uid]

	for uid in man_tree:
		insert_in_tree(uid)

	if True:
		# populate custom pages
		def insert_in_uid_dict(uid, uids):
			dict = insert_in_tree(uid)
			for uid in uids:
				if uid not in dict:
					dict[uid] = {}

		insert_in_uid_dict("man.Classes", doc_tools.get_all_classes())
		insert_in_uid_dict("man.Functions", sorted(html_tools.get_tag_uids("function", True)))
		insert_in_uid_dict("man.Enums", sorted(html_tools.get_tag_uids("enum", True)))

	return tree


def output_tree_html(path):
	""" Obsolete function that creates the documentation tree automatically """
	tree = build_doc_tree()

	with open(path, 'w') as file:
		def output_tree(tree, depth):
			for uid, sub_tree in tree.items():
				title = doc_tools.get_element_title(uid)
				if depth == 0:
					file.write('<li class="first">\n')
				else:
					file.write('<li>\n')
				file.write('<a href="%s"><abbr title="%s">%s</abbr></a>\n' % (uid + ".html", title, title))

				if len(sub_tree) > 0:
					file.write('<ul>\n')
					output_tree(sub_tree, depth+1)
					file.write('</ul>\n')

				file.write('</li>\n')

		file.write('<ul id="tree" class="tree doc-index">\n')
		output_tree(tree, 0)
		file.write('</ul>\n')

	return True


def create_tree_html(path):
	""" New function to create the documentation tree from a simple input file """
	with open(os.path.join(args.doc_path, "tree_desc.txt"), "r", encoding="utf-8") as file:
		tree_lines = file.read().splitlines()

	class Node:
		def __init__(self, name):
			self.name = name
			self.children = []

		def get_link(self):
			return self.name

		def __repr__(self):
			return 'NODE %s' % self.name

		def to_html(self):
			if self.name == '':
				return '<br>\n'

			str = '<li>'
			str += self.get_link()

			if len(self.children) > 0:
				str += '<ul>\n'
				for child in self.children:
					str += child.to_html()
				str += '</ul>\n'

			str += '</li>\n'
			return str

	class Link(Node):
		def __init__(self, link):
			super().__init__(link)

		def get_link(self):
			name = doc_tools.get_element_title(self.name)

			if gen_online_doc:
				return '<a href="%s">%s</a>' % (self.name + '.html', name)
			return '<a href="%s" target="content-frame">%s</a>' % (self.name + '.html', name)

		def __repr__(self):
			return 'LINK %s' % self.name

	last_node = Link('man.Pyindex')
	stack = [last_node]

	for line in tree_lines:
		current_depth = len(stack)

		# determine target depth
		target_depth = 0
		while line[target_depth:target_depth+1] == '\t':
			target_depth += 1
		target_depth += 1

		# create the new node
		decl = line[target_depth-1:]

		if decl.startswith("!"):
			node = Node(decl[1:])
		else:
			node = Link(decl)

		# make sure we only go down one level at a time
		assert target_depth <= current_depth + 1

		if target_depth > current_depth:
			stack.append(last_node)

		elif target_depth < current_depth:
			while target_depth < len(stack):
				stack.pop()

		stack[-1].children.append(node)
		last_node = node

	# output final tree
	with open(path, "w", encoding="utf-8") as file:
		if not gen_online_doc:
			file.write('<html>\n')
			file.write('<head><link type="text/css" rel="stylesheet" href="markdown.css"></head>\n')
			file.write('<body>\n')
			file.write('<h1>%s %s</h1>' % (args.project_name, version))

		file.write('<ul id="tree" class="tree doc-index">')
		for child in stack[0].children:  # skip root entry
			file.write(child.to_html())
		file.write('</ul>')

		if not gen_online_doc:
			file.write('</body>\n')
			file.write('</html>\n')

def sanity_check():
	report = {
		'missing': {},
		'obsolete': [],
		'missing_pages': [],
		'orphan_pages': []
	}
	
	ok = True

	for uid in doc_tools.doc.keys():
		if not uid in api_tools.api:
			report['obsolete'].append(uid)
	ok = ok and report['obsolete']

	for uid, tag in api_tools.api.items():
		if not uid in doc_tools.doc:
			typename = tag[0].tag
			if not typename in report['missing']:
				report['missing'][typename] = []
			report['missing'][typename].append(uid)
	ok = ok and report['missing']

	link_list = ['man.Index']
	for uid in doc_tools.man.keys():
		for link in doc_tools.gather_manual_links(uid):
			filename = '{0}.md'.format(link)
			link_list.append(link)
			if not os.path.isfile(os.path.join(args.doc_path, filename)):
				report['missing_pages'].append(link)
	ok = ok and report['missing_pages']

	with open(os.path.join(args.doc_path, "tree_desc.txt"), "r", encoding="utf-8") as file:
			tree_lines = file.read().splitlines()
			for line in tree_lines:
				line = line.strip()
				if(line.startswith('!')):
					continue
				link_list.append(line)

	for uid in doc_tools.man.keys():
		if not uid in link_list:
			report['orphan_pages'].append(uid)
	ok = ok and report['orphan_pages']

	report['missing_pages'].sort()
	report['orphan_pages'].sort()

	return (ok, report)

def save_sanity_check_report(path, report):
	with open(path, "w", encoding="utf-8") as file:
		file.write('# Error report\n')
		if report['missing']:
			file.write(' * [missing](#missing) : undocumented symbols.\n')
			for typename, items in report['missing'].items():
				if not items:
					continue
				file.write('     * [{0}](#{1})\n'.format(typename, typename))
		if report['obsolete']:
			file.write(' * [obsolete](#obsolete) : cannot find the symbol in the api. \n')
		if report['missing_pages']:
			file.write(' * [missing pages](#missing-pages) : broken link\n')
		if report['orphan_pages']:
			file.write(' * [orphan pages](#orphan-pages) : cannot find a reference to this file.\n')

		if report['missing']:
			file.write('## missing\n')
			for typename, items in report['missing'].items():
				if not items:
					continue
				file.write('### {0}\n'.format(typename))
				for uid in items:
					file.write(' * {0}\n'.format(uid))

		if report['obsolete']:
			file.write('## obsolete\n')
			for uid in report['obsolete']:
					file.write(' * {0}\n'.format(uid))

		if report['missing_pages']:
			file.write('## missing pages\n')
			for uid in report['missing_pages']:
					file.write(' * {0}\n'.format(uid))

		if report['orphan_pages']:
			file.write('## orphan pages\n')
			for uid in report['orphan_pages']:
					file.write(' * {0}\n'.format(uid))

def doc_to_html(output_path, css=None, whitelist=None, callback=None):
	"""Convert the complete documentation to HTML"""
	os.makedirs(output_path, exist_ok = True)

	link_formatter = partial(html_link_formatter, output_path)

	# gather symbol pages
	doc_pages = []
	for uid, tag in api_tools.api.items():
		if whitelist is None or uid in whitelist:
			if tag[0].tag in ['class']:
				doc_pages.append((uid, tag))

	# gather manual pages
	man_pages = []
	for uid, data in doc_tools.man.items():
		if whitelist is None or uid in whitelist:
			man_pages.append((uid, data))

	page_count = len(doc_pages) + len(man_pages)

	# generate manual pages hierarchy tree
	tree_blacklist = []

	global man_tree
	man_tree = doc_tools.generate_manual_tree(tree_blacklist)

	create_tree_html(os.path.join(output_path, "tree.html"))

	# generate all pages
	page_generated = 0

	for uid, tag in doc_pages:
		type = tag[0].tag

		if callback and not callback(uid, page_generated, page_count):
			return False
		data = doc_tools.get_content_always(uid)
		body = html_tools.format_page_content(uid, data, link_formatter)
		html = format_complete_html_man_page(uid, body, get_header_fn[type])
		save_page(output_path, uid, html)
		search_index.parse(uid, version, html)
		page_generated += 1

	for uid, data in man_pages:
		body = html_tools.format_page_content(uid, data, link_formatter)
		html = format_complete_html_man_page(uid, body, get_man_header)
		save_page(output_path, uid, html)
		search_index.parse(uid, version, html)
		page_generated += 1

	# copy css
	if css is not None:
		shutil.copyfile(css, os.path.join(output_path, 'markdown.css'))

	search_index.save(output_path, "search_index.json")

	man_tree = None
	return True


# ------------------------------------------------------------------------------
if __name__ == '__main__':
	parser = argparse.ArgumentParser(description="Convert the internal documentation format to HTML")
	parser.add_argument('--project_name', type=str, help="Project name", required=True)
	parser.add_argument('--api_path', type=str, help="API path", required=True)
	parser.add_argument('--doc_path', type=str, help="Documentation path", required=True)
	parser.add_argument('--out_path', type=str, help="Output folder", required=True)
	parser.add_argument('--uid', type=str, help="Generate the documentation for this specific UID")
	parser.add_argument('--version', type=str, help="API version")
	parser.add_argument('--online', action="store_true", help="Generate the online documentation")
	args = parser.parse_args()

	gen_online_doc = args.online

	html_tools.gen_online_doc = gen_online_doc

	if args.version: version = args.version

	# determine CSS location
	path = os.path.dirname(os.path.realpath(sys.argv[0]))
	css = os.path.join(path, "markdown.css")

	# load the API and documentation
	api_tools.load_api(args.api_path)
	doc_tools.load_doc_folder(args.doc_path)

	# generate documentation
	whitelist = None if args.uid is None else args.uid.split(',')
	doc_to_html(args.out_path, css, whitelist)

	ok, report = sanity_check()
	save_sanity_check_report(os.path.join(args.out_path, 'sanity_report.md'), report)