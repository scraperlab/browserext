#include "phpbrowser.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	PhpBrowser browser(true);
    browser.setWindowTitle("BrowserExt Browser");
	browser.editableUrl(true);
	browser.show();
	return app.exec();
}
