from os import path, listdir, makedirs
from pathlib import Path
from codecs import open  # to use a consistent encoding

import sys
import cmake_build_extension
import pygit2

from setuptools import setup
from subprocess import check_call

from typing import Generator, List

# Command line examples:
#  - Create source package
#         python3 -m build --sdist --outdir dist languages/hg_python/pip
#
#  - Install from the source package
#         python3 -m pip -v install dist/harfang-3.2.2*.tar.gz
#
#  - Create binary package
#         python3 languages/hg_python/pip/setup.py bdist_wheel
#
#  - Install the wheel
#         python3 -m pip install dist/harfang-3.2.1-cp38-cp38-linux_x86_64.whl
#
# Note:
#  The following development package are necessary to rebuild harfang:
#                ubuntu: uuid-dev, libreadline-dev, libxml2-dev, libgtk-3-dev
#         centos/fedora: uuid-devel, readline-devel, libxml2-devel, gtk3-devel
#
# Harfang tools (assetc, assimp_converter, gltf_importer and gltf_exporter) can now
# be called from the command line.
# Example:
#		python3 -m harfang.bin assetc resources_path -api GL
#
# They can also be called from a python script.
# Example:
#		import harfang.bin
#		harfang.bin.assetc('resources', '-api', 'GL')
#
init_py = """
from .harfang import *
"""

# Here we bypass cmake_build_extension automatic generation of bin scripts.
# We must do this to allow tools to be launched from python scripts and not 
# just from the command line.
class BuildExtension(cmake_build_extension.BuildExtension):
	def build_extension(self, ext: cmake_build_extension.CMakeExtension) -> None:
		_binaries = ext.expose_binaries
		ext.expose_binaries = []
		super().build_extension(ext)

		if len(_binaries) <= 0:
			return

		ext_dir = Path(self.get_ext_fullpath(ext.name)).parent.absolute()
		cmake_install_prefix = ext_dir / ext.install_prefix

		bin_dirs = {str(Path(d).parents[0]) for d in _binaries}

		import inspect

		_files = {
			"__init__.py": inspect.cleandoc(
				"""
				from .run import run
				def __getattr__(name:str):
					return run(name)
				"""
			),
			"__main__.py": inspect.cleandoc(
				"""
				import sys
				from .run import run
				if __name__ == "__main__" and len(sys.argv) > 1:
					tool = run(sys.argv[1])
					exit(tool(*sys.argv[2:]))
				"""
			),
			"run.py": inspect.cleandoc(
				f"""
					from pathlib import Path
					import subprocess

					def run(name:str):
						_bin_dirs = {str(bin_dirs)}
						if not name in _bin_dirs:
							raise RuntimeError(f"Unknown tools {{name}}")
						
						def method(*args):
							_binary_name = Path(name).name
							_prefix = Path(__file__).parent.parent
							_binary_path = ""

							for dir in _bin_dirs:
								path = _prefix / Path(dir) / _binary_name
								if path.is_file():
									_binary_path = str(path)
									break
							
								path = Path(str(path) + ".exe")
								if path.is_file():
									_binary_path = str(path)
									break

							if not Path(_binary_path).is_file():
								_name = _binary_path if _binary_path != "" else _binary_name
								raise RuntimeError(f"Failed to find binary: {{_name}}")

							return subprocess.run(args=[_binary_path, *args], capture_output=False)

						return method
				"""
			)
		}

		bin_folder = cmake_install_prefix / "bin"
		Path(bin_folder).mkdir(exist_ok=True, parents=True)
		for _filename, _content in _files.items():
			with open(filename=bin_folder / _filename, mode="w") as f:
				f.write(_content)


class GitSdistFolder(cmake_build_extension.GitSdistFolder):
	@staticmethod
	def get_sdist_files(repo_root: str) -> List[Path]:

		# Create the list of files of the git folder
		repo_path = Path(repo_root)
		all_files_gen = repo_path.glob(pattern="**/*")

		ignore_list = ["tools/assimp_converter/assimp/test", "extern/openvr/samples", "extern/cmft/runtime", "extern/bgfx/bgfx/examples"]

		# Return the list of absolute paths to all the git folder files (also uncommited)
		return [f for f in all_files_gen if not f.is_dir() and ".git" not in f.parts and not any(f.is_relative_to(repo_path / Path(pattern)) for pattern in ignore_list)]


# Check if we are building from repository source or from a package source distribution.
# For the latter, the setup.py script is at the root of the source tree.
# Otherwise, it is located in languages/hg_python/pip/.
here = path.abspath(path.dirname(__file__))
source_dir = here
if here.endswith(path.join('languages','hg_python','pip')):
	# We assume that the source tree comes from the repository if the setup.py script is called
	# from languages/hg_python/pip.
	# Note that you can always call it by hand from the source package.
	source_dir = str(Path(here).parent.parent.parent)

# initialize/update git submodules
git_dir = path.join(source_dir, '.git')
commit_id_path = path.join(source_dir, "commit_id")
commit_id = "unknown"
if path.exists(git_dir):
	repository = pygit2.Repository(git_dir)
	repository.init_submodules()
	commit_id = repository.revparse_single('HEAD').hex
	with open(commit_id_path, 'w', encoding='utf-8') as f:
		f.write(commit_id)
else:
	with open(commit_id_path, 'r', encoding='utf-8') as f:
		commit_id = f.read().strip()

# create build directory if needed
build_dir = path.join(source_dir, 'build')
if not path.exists(build_dir):
	makedirs(build_dir)

# clone FABgen repository
fabgen_path = path.join(source_dir, 'extern', 'fabgen')
if not path.isdir(fabgen_path):
	pygit2.clone_repository('https://github.com/ejulien/FABGen.git', fabgen_path, checkout_branch='master')

# retrieve version
with open(path.join(source_dir, 'harfang', 'version.txt'), encoding='utf-8') as f:
	version_string = f.read().strip()

# get the long description from the relevant file
with open(path.join(source_dir, 'languages', 'hg_python', 'DESCRIPTION.rst'), encoding='utf-8') as f:
	long_description = f.read()

setup(
	name='harfang',

	version=version_string,

	description='HARFANG 3D is a game/visualization library for Python. It includes a comprehensive set of Scene, Physics, Rendering pipeline, Audio and Virtual Reality APIs. It is written in C++ and supports DirectX 11, OpenGL and OpenGL ES.',
	long_description=long_description,

	url='https://www.harfang3d.com',

	project_urls={
		'Changelog': f"https://github.com/harfang3d/harfang3d/releases/tag/v{version_string}",
		'Documentation': f"https://www.harfang3d.com/api/{version_string}/cpython/classes/",
		'Source': 'https://github.com/harfang3d/harfang3d',
		'Tracker': 'https://github.com/harfang3d/harfang3d/issues'
	},

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

		'Operating System :: POSIX',
		'Operating System :: POSIX :: Linux',
		'Operating System :: Microsoft',
		'Operating System :: Microsoft :: Windows',

		'Programming Language :: Python :: 3',
	],

	keywords='2d 3d multimedia development engine realtime rendering design visualization simulation physics vr virtual reality python lua opengl opengles directx',

	cmdclass=dict(
		build_ext=BuildExtension,
		sdist=GitSdistFolder
	),
	ext_modules=[
		cmake_build_extension.CMakeExtension(
			name="CMakeProject",
			install_prefix="harfang",
			expose_binaries=[
				"assetc/assetc",
				"assimp_converter/assimp_converter",
				"gltf_exporter/gltf_exporter",
				"gltf_importer/gltf_importer",
			],
			disable_editable=True,
			write_top_level_init=init_py,
			source_dir=source_dir,
			cmake_configure_options=[
				f"-DPython3_EXECUTABLE:FILEPATH={sys.executable}",
				'-DCMAKE_BUILD_TYPE=Release',
				'-DCMAKE_MODULE_PATH:PATH=' + path.join(source_dir, 'harfang', 'cmake'),
				'-DHG_FABGEN_PATH:PATH=' + fabgen_path,
				'-DHG_BUILD_ASSETC:BOOL=ON',
				'-DHG_BUILD_CPP_SDK:BOOL=OFF',
				'-DHG_REBUILD_GLFW:BOOL=ON',
				'-DHG_BUILD_TESTS:BOOL=OFF',
				'-DHG_BUILD_FBX_CONVERTER:BOOL=OFF',
				'-DHG_BUILD_GLTF_IMPORTER:BOOL=ON',
				'-DHG_BUILD_GLTF_EXPORTER:BOOL=ON',
				'-DHG_BUILD_ASSIMP_CONVERTER:BOOL=ON',
				'-DHG_BUILD_HG_LUA:BOOL=OFF',
				'-DHG_BUILD_HG_PYTHON:BOOL=ON',
				'-DHG_PYTHON_PIP:BOOL=ON',
				'-DHG_BUILD_HG_GO:BOOL=OFF',
				'-DHG_ENABLE_OPENVR_API:BOOL=ON',
				'-DHG_ENABLE_RECAST_DETOUR_API:BOOL=OFF',
				'-DHG_BUILD_DOCS:BOOL=OFF',
				'-DHG_COMMIT_ID:STRING=' + commit_id
			]
		)
	]
)
