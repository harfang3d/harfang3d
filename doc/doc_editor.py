from PyQt5.QtWidgets import QApplication
from doc_editor.main_window import MainWindow
import sys
import os
import argparse


if __name__ == '__main__':
	parser = argparse.ArgumentParser(description="Internal documentation editor")
	parser.add_argument('--api_path', type=str, help="API path", required=False)
	parser.add_argument('--doc_path', type=str, help="Documentation path", required=False)
	parser.add_argument('--out_path', type=str, help="Documentation output path", required=False)
	args = parser.parse_args()

	app = QApplication(sys.argv)

	path = os.path.dirname(os.path.realpath(sys.argv[0]))

	main_window = MainWindow()
	main_window.preview_css = os.path.join(path, "doc_editor", "markdown.css")

	if args.api_path:
		main_window.load_api(args.api_path)
	if args.doc_path:
		main_window.load_doc(args.doc_path)

	if args.out_path:
		main_window.build_documentation(args.out_path)
		r = 0
	else:
		main_window.showMaximized()
		r = app.exec_()

	sys.exit(r)
