import os
import sys

sys.path.insert(0, os.path.abspath("extensions"))

extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.doctest",
    "sphinx.ext.todo",
    "sphinx.ext.coverage",
    "sphinx.ext.imgmath",
    "sphinx.ext.ifconfig",
    "sphinx.ext.autodoc",
]

latex_engine = "xelatex"
todo_include_todos = True
templates_path = ["_templates"]
source_suffix = ".rst"
master_doc = "defiance"
latex_documents = [
    ("defiance", "defiance-doc.tex", "DEFIANCE Design Documentation", "BPHK2023", "manual"),
    ("defiance-design", "defiance-doc-design.tex", "DEFIANCE Design Documentation", "BPHK2023", "manual"),
    ("defiance-user", "defiance-doc-user.tex", "DEFIANCE User Documentation", "BPHK2023", "manual"),
]
exclude_patterns = []
add_function_parentheses = True

project = "DEFIANCE Module"
copyright = "2024"
author = "DEFIANCE/HPI"

version = "1.0"
release = "1.0"
