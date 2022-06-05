from setuptools import setup, Distribution
from codecs import open  # to use a consistent encoding
from os import path, listdir


class BinaryDistribution(Distribution):
	def has_ext_modules(self):
		return True


here = path.abspath(path.dirname(__file__))

# get version
with open(path.join(here, 'version.txt'), encoding='utf-8') as f:
	version_string = f.read()

# get the long description from the relevant file
with open(path.join(here, 'DESCRIPTION.rst'), encoding='utf-8') as f:
	long_description = f.read()

setup(
	name='harfang',

	version=version_string,

	description='HARFANG 3D is a game/visualization library for Python. It includes a comprehensive set of Scene, Physics, Rendering pipeline, Audio and Virtual Reality APIs. It is written in C++ and supports DirectX 11, OpenGL and OpenGL ES.',
	long_description=long_description,

	url='https://www.harfang3d.com',

	author='NWNC HARFANG',
	author_email='contact@harfang3d.com',

	license='Other/Proprietary License',

	# See https://pypi.python.org/pypi?%3Aaction=list_classifiers
	classifiers=[
		'Intended Audience :: Developers',
		'Intended Audience :: Science/Research',

		'Topic :: Software Development',
		'Topic :: Software Development :: Libraries :: Application Frameworks',
		'Topic :: Multimedia :: Sound/Audio',
		'Topic :: Multimedia :: Graphics :: 3D Rendering',
		'Topic :: Scientific/Engineering :: Visualization',

		'Operating System :: MacOS :: MacOS X',
		'Operating System :: POSIX',
		'Operating System :: POSIX :: Linux',
		'Operating System :: Microsoft',
		'Operating System :: Microsoft :: Windows',

		'Programming Language :: Python :: 3',
	],

	keywords='2d 3d multimedia development engine realtime rendering design visualization simulation physics vr virtual reality python lua opengl opengles directx',

	packages=['harfang'],
	package_data={
		'harfang': ['*.pyd', '*.pdb', '*.so', '*.so.*', '*.dll', '*.dylib']
	},
	distclass=BinaryDistribution
)
