#ifndef XPATHINSPECTOR_H
#define XPATHINSPECTOR_H

#include <QtWidgets>
#include <QtWebKit>

class PhpWebView;
class PhpBrowser;

class XPathInspector : public QWidget
{
	Q_OBJECT

public:
	XPathInspector(PhpWebView *webview, QWidget *parent = 0);
	~XPathInspector();
	void setActiveElement(const QWebElement & elem, bool select = true);
	void setFewActiveElements(const QList<QWebElement> & list, bool select = true);
	void selectElements(bool setselect = true);

public Q_SLOTS:
	void itemActivated(QTreeWidgetItem *item, int column);
	void processSelection();
	void processXPath();

protected:
	QTreeWidgetItem* createTree(QWebElement root);
	virtual void closeEvent(QCloseEvent *event);
	QTreeWidgetItem* findTreeItemAndSelect(const QWebElement & elem);
	void deselectTreeItems();
	
	QBoxLayout *layout_w;
	QBoxLayout *layout2;
	QLabel *label;
	QLineEdit *edit;
	QTextEdit *edit2;
	QTreeWidget *tree;
	PhpWebView *webview;
	QWebElement activeElement;
	QString activeOldStyle;
	QList<QWebElement> selectedElements;
};

#endif