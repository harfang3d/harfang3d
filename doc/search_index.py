from bs4 import BeautifulSoup
import json
import os

index = []

def get_soup_title_and_category(uid, soup):
	if uid == 'man.Functions':
		uid = uid

	title, category = None, 'other'

	tutorial_header = soup.find('table', class_='tutorial-header')
	is_tutorial = tutorial_header is not None

	#
	if title is None:
		doc_details = soup.find('div', class_='doc-details')

		if doc_details is not None:
			h1 = doc_details.find('h1')
			if h1 is not None:
				title = str(h1.text)

	if title is None:
		breadcrumb = soup.find('div', class_='doc-breadcrumb')

		if breadcrumb is not None:
			links = breadcrumb.find_all('a')
			if len(links) > 0:
				title = str(links[-1].string)
			else:
				title = None
		else:
			h1 = soup.find('h1')
			if h1 is not None:
				title = h1.string
				if title is not None:
					title = str(title)

	if title is None or title == 'Documentation':
		title = uid  # fallback to uid

	if is_tutorial:
		title = "Tutorial '%s'" % title

	# determine category
	if is_tutorial:
		category = 'tutorial'
	elif uid.startswith('man.'):
		category = 'manual'
	elif title.endswith('Class'):
		category = 'class'
		title = title[:-5].rstrip()

	return title, category


def filter_and_parse(uid, version, html, filter):
	soup = BeautifulSoup(html, 'html.parser')

	# get title and category
	title, category = get_soup_title_and_category(uid, soup)

	# apply filter
	soup, body = filter(soup)
	if body == '':
		return

	# build excerpt candidates
	excerpts = []

	def add_excerpt(ex):
		if ex is None:
			return
		s = str(ex).strip()
		if len(s) > 0:
			excerpts.append(s)

	for p in soup.find_all('p'):  # raw paragraph content
		add_excerpt(p.string)

	for div in soup.find_all('div'):  # function documentation
		if 'class' in div.attrs:
			for class_ in div['class']:
				if class_ in ['function_doc']:
					add_excerpt(div.string)

	# finalize entry
	entry = {'uid': uid, 'category': category, 'title': title, 'body': body, 'excerpts': excerpts, 'version': str(version)}
	index.append(entry)

	return soup, title


def clean_text(t):
	return t.lstrip().rstrip()


def extract_page_functions(uid, version, page_title, soup):
	function_divs = soup.find_all('div', class_='function_div')

	for function_div in function_divs:
		id = function_div.get('id')

		if id.startswith(uid):
			name = id[len(uid)+1:]
			title = '%s.%s' % (uid, name)
		else:
			name = id
			title = page_title

		body = name

		function_proto = function_div.find('div', class_='function_proto')
		function_doc = function_div.find('div', class_='function_doc')

		excerpt = []
		if function_doc:
			excerpt.append(clean_text(function_doc.text))

		entry = {'uid': '%s#%s' % (uid, id), 'category': 'function', 'title': title, 'body': body, 'excerpts': excerpt, 'version': str(version)}
		index.append(entry)


def parse(uid, version, html):
	def basic_stripper_filter(soup):
		# strip breadcrumb
		breadcrumb = soup.find('div', {'class': 'doc-breadcrumb'})
		if breadcrumb is not None:
			breadcrumb.extract()

		# strip version footer
		version_span = soup.find('small', {'id': 'version-footer'})
		if version_span is not None:
			version_span.extract()

		code_divs = soup.find_all('div', {'class': 'codehilite'})
		for div in code_divs:
			div.extract()

		# extract body
		body = soup.get_text(separator=' ')
		# body = ' '.join(body.split())

		for s in ['(', '[', '{', '\n']:
			body = body.replace(s + ' ', s)
		for s in [')', ']', '}', ',', ':', ';', '\n']:
			body = body.replace(' ' + s, s)

		for i in range(4):
			body = body.replace('  ', ' ')
			body = body.replace('\n\n', '\n')

		return soup, body

	soup, title = filter_and_parse(uid, version, html, basic_stripper_filter)

	# extract page functions
	extract_page_functions(uid, version, title, soup)


def save(dir, path):
	with open(os.path.join(dir, path), 'w') as file:
		for entry in index:
			file.write('{"index":{}}\n')
			json.dump(entry, file)
			file.write('\n')
		file.write('{"index":{}}\n')
