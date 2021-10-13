from pypeg2 import parse
from doc_utils.metadata import Directive, TextBlock, parse_metadata
import re


r = parse_metadata(
	'.section(name="Getting Started", icon="/images/icon/documentation/gettingstarted.png")\n'
	'.row\n'
	'.block(name="Requirements", link="man.Requirements", desc="Basic software and hardware requirements to develop with Ookpy and run your creations.")\n'
	'.block(name="Installation", link="man.Installation", desc="How to install Ookpy on your computer and the end-user computer.")\n'
	'.block(name="Getting Help", link="man.GettingHelp", desc="Where to find help when you hit a rock on your path to success.")\n'
	'.endrow\n'
	'.endsection\n')

r = parse('.section(name="Section", link="man.Section")', Directive)

r = parse(
	'   A text block and...'
	'   ...multiple lines!', TextBlock, whitespace=re.compile("(?m)\s+"))

r = parse(".proto(l)", Directive)
r = parse(".proto(l=python)", Directive)

r = parse(".proto [[Picture]] LoadPicture ($str path)", Directive)

r = parse(
	'.notes\n'
	'    No caching mechanism involved, every call to this function will result in a filesystem access.\n'
	'    .lang(lua)\n'
	'        .warning\n'
	'            Starts with 1 in Lua.\n'
	'        .endwarning\n'
	'    .endlang\n'
	'.endnotes\n', Directive)

data = 'test text \n' \
			  '\n' \
			  '.notes\n' \
			  '    No caching mechanism involved, every call to this function will result in a filesystem access.\n' \
			  '    .lang(lua)\n' \
			  '        .warning\n' \
			  '            Starts with 1 in Lua.\n' \
			  '        .endwarning\n' \
			  '    .endlang\n' \
			  '.endnotes\n' \
			  '.lang(lua)\n' \
			  '    .codesnippet\n' \
			  '        local my_picture = LoadPicture("assets/picture.png")\n' \
			  '    .endcodesnippet\n' \
			  '.endlang\n' \
			  '.proto $bool PictureSaveTGA ([[Picture]] pict, $str path)\n' \
			  '.param(pict) The [[Picture]] object to be saved.\n' \
			  '.param(path) The path where to save the file.\n' \
			  '// bla bla comment\n' \
			  '.return\n' \
			  '    Returns $true if the picture was succesfully saved.\n' \
			  '    Otherwise returns $false.\n' \
			  '.endreturn\n' \
			  '.desc\n' \
			  '    Save a [[Picture]]. The image format is inferred from the file extension (TGA, BMP, JPG or PSD).\n' \
			  '.enddesc\n' \
			  '.notes\n' \
			  '    Make sure that you have access to the filesystem you plan on writing to.\n' \
			  '    See [[SystemHasMountPoint]].\n' \
			  '.endnotes'

r = parse_metadata(data)
r = r