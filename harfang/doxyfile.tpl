DOXYFILE_ENCODING      = UTF-8
PROJECT_NAME           = HARFANG®
PROJECT_NUMBER         = @HG_VERSION@
PROJECT_BRIEF          = "3D application framework for Industry and Entertainment"
PROJECT_LOGO           = @CMAKE_CURRENT_SOURCE_DIR@/../doc/img/harfang.png
OUTPUT_DIRECTORY       = @CMAKE_INSTALL_PREFIX@/cppsdk_docs
CREATE_SUBDIRS         = NO
ALLOW_UNICODE_NAMES    = NO
OUTPUT_LANGUAGE        = English
BRIEF_MEMBER_DESC      = YES
REPEAT_BRIEF           = YES
TAB_SIZE               = 4
OPTIMIZE_OUTPUT_FOR_C  = NO
MARKDOWN_SUPPORT       = YES
TOC_INCLUDE_HEADINGS   = 0
AUTOLINK_SUPPORT       = YES
BUILTIN_STL_SUPPORT    = YES
CPP_CLI_SUPPORT        = NO
SIP_SUPPORT            = NO
IDL_PROPERTY_SUPPORT   = YES
DISTRIBUTE_GROUP_DOC   = NO
GROUP_NESTED_COMPOUNDS = NO
SUBGROUPING            = YES
INLINE_GROUPED_CLASSES = NO
INLINE_SIMPLE_STRUCTS  = NO
TYPEDEF_HIDES_STRUCT   = NO
LOOKUP_CACHE_SIZE      = 0

# Build related configuration options

JAVADOC_AUTOBRIEF      = YES
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = NO
EXTRACT_PACKAGE        = NO
EXTRACT_STATIC         = NO
EXTRACT_LOCAL_CLASSES  = YES
EXTRACT_LOCAL_METHODS  = NO
EXTRACT_ANON_NSPACES   = NO
HIDE_UNDOC_MEMBERS     = NO
HIDE_UNDOC_CLASSES     = NO
HIDE_FRIEND_COMPOUNDS  = NO
HIDE_IN_BODY_DOCS      = NO
INTERNAL_DOCS          = NO
CASE_SENSE_NAMES       = NO
HIDE_SCOPE_NAMES       = NO
HIDE_COMPOUND_REFERENCE= NO
SHOW_INCLUDE_FILES     = YES
SHOW_GROUPED_MEMB_INC  = NO
FORCE_LOCAL_INCLUDES   = NO
INLINE_INFO            = YES
SORT_MEMBER_DOCS       = YES
SORT_BRIEF_DOCS        = NO
SORT_MEMBERS_CTORS_1ST = YES
SORT_GROUP_NAMES       = YES
SORT_BY_SCOPE_NAME     = YES
STRICT_PROTO_MATCHING  = NO
GENERATE_TODOLIST      = YES
GENERATE_TESTLIST      = YES
GENERATE_BUGLIST       = YES
GENERATE_DEPRECATEDLIST= YES
ENABLED_SECTIONS       = 
MAX_INITIALIZER_LINES  = 30
SHOW_USED_FILES        = YES
SHOW_FILES             = YES
SHOW_NAMESPACES        = NO
FILE_VERSION_FILTER    = 
LAYOUT_FILE            = 
CITE_BIB_FILES         = 

# Configuration options related to warning and progress messages

QUIET                  = NO
WARNINGS               = YES
WARN_IF_UNDOCUMENTED   = YES
WARN_IF_DOC_ERROR      = YES
WARN_FORMAT            = "$file:$line: $text"

# Configuration options related to the input files

INPUT                  = @CMAKE_SOURCE_DIR@/readme.md @CMAKE_SOURCE_DIR@/harfang/platform @CMAKE_SOURCE_DIR@/harfang/foundation @CMAKE_SOURCE_DIR@/harfang/engine @CMAKE_SOURCE_DIR@/harfang/script
INPUT_ENCODING         = UTF-8
FILE_PATTERNS          = *.h *.md
USE_MDFILE_AS_MAINPAGE = @CMAKE_SOURCE_DIR@/readme.md
RECURSIVE              = NO
EXCLUDE                =

# Configuration options related to source browsing

STRIP_CODE_COMMENTS    = YES
REFERENCED_BY_RELATION = NO
REFERENCES_RELATION    = NO
REFERENCES_LINK_SOURCE = YES
SOURCE_TOOLTIPS        = YES
USE_HTAGS              = NO
VERBATIM_HEADERS       = YES
CLANG_ASSISTED_PARSING = NO
CLANG_OPTIONS          = 

# Configuration options related to the alphabetical class index

ALPHABETICAL_INDEX     = YES
COLS_IN_ALPHA_INDEX    = 3
IGNORE_PREFIX          = 

# Configuration options related to the HTML output

GENERATE_HTML          = YES
HTML_OUTPUT            = html
HTML_FILE_EXTENSION    = .html
HTML_HEADER            = 
HTML_FOOTER            = 
HTML_STYLESHEET        = 
HTML_EXTRA_STYLESHEET  = 
HTML_EXTRA_FILES       = 
HTML_COLORSTYLE_HUE    = 220
HTML_COLORSTYLE_SAT    = 100
HTML_COLORSTYLE_GAMMA  = 80
HTML_TIMESTAMP         = NO
HTML_DYNAMIC_SECTIONS  = NO
HTML_INDEX_NUM_ENTRIES = 100
SEARCHENGINE           = YES
SERVER_BASED_SEARCH    = NO
EXTERNAL_SEARCH        = NO
SEARCHENGINE_URL       =
SEARCHDATA_FILE        = searchdata.xml
EXTERNAL_SEARCH_ID     =
EXTRA_SEARCH_MAPPINGS  =

# ...

GENERATE_DOCSET        = NO
GENERATE_HTMLHELP      = NO
GENERATE_CHI           = NO
GENERATE_QHP           = NO
GENERATE_ECLIPSEHELP   = NO
GENERATE_LATEX         = NO
GENERATE_RTF           = NO
GENERATE_MAN           = NO
GENERATE_XML           = NO
GENERATE_DOCBOOK       = NO
GENERATE_AUTOGEN_DEF   = NO
GENERATE_PERLMOD       = NO

# Tree view

GENERATE_TREEVIEW      = YES
ENUM_VALUES_PER_LINE   = 4
TREEVIEW_WIDTH         = 250

# Configuration options related to the preprocessor

ENABLE_PREPROCESSING   = YES
MACRO_EXPANSION        = YES
EXPAND_ONLY_PREDEF     = YES
SEARCH_INCLUDES        = YES
INCLUDE_PATH           = 
INCLUDE_FILE_PATTERNS  = 
PREDEFINED             = "comp_decl_member_get_sync_set_async_noimpl(C, T, N, V)=public: const T Get##N() const; void Set##N(const T &v);" \
                         "comp_decl_member_get_sync_set_async(C, T, N, V)=public: const T Get##N() const; void Set##N(const T &v);" \
                         "comp_decl_member_get_sync_set_async_validated(C, T, N, V, VALIDATOR)=public: const T Get##N() const; void Set##N(const T &v);" \
                         "comp_decl_member_get_sync_noimpl_set_async_validated(C, T, N, V, VALIDATOR)=public: const T Get##N() const; void Set##N(const T &v);" \
                         "comp_decl_member_get_sync_set_async_noimpl_novar(C, T, N)=public: const T Get##N() const; void Set##N(const T &v);" \
                         "comp_decl_member(C, T, N)=public: const T Get##N() const; void Set##N(const T &v);" \
                         "decl_component_defaults(CLASS, ASPECT)=const std::string &GetAspect() const override;" \
                         "decl_system_base_get_set_async_noimpl(SYSTEM, TYPE, NAME)=public: virtual TYPE Get##NAME() const; void Set##NAME(const TYPE &v);" \
                         "decl_system_base_get_set_async(SYSTEM, TYPE, NAME, VAR)=public: TYPE Get##NAME() const; void Set##NAME(const TYPE &v);" \
                         "decl_system_base_component_get_set_async(SYSTEM, COMP, TYPE, NAME)=public: virtual TYPE Get##NAME(const COMP *c) const = 0; void Set##NAME(COMP *c, const TYPE &v);" \
                         "node_decl_flag_get_sync_set_async(N, V)=public: bool Get##N() const; void Set##N(bool v);" \
                         "protected=private"

EXPAND_AS_DEFINED      = 
SKIP_FUNCTION_MACROS   = YES
