#include "xpathinspector.h"
#include "phpbrowser.h"


XPathInspector::XPathInspector(PhpWebView *webview, QWidget *parent)
	: QWidget(parent, Qt::WindowStaysOnTopHint | Qt::Window)
{
	this->webview = webview;
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle("XPathInspector");
	resize(600, 350);
	layout_w = new QBoxLayout(QBoxLayout::TopToBottom, this);
	layout2 = new QBoxLayout(QBoxLayout::LeftToRight);
	layout_w->addLayout(layout2);
	label = new QLabel("xpath: ", this);
	edit = new QLineEdit("", this);
	layout2->addWidget(label);
	layout2->addWidget(edit);

	edit2 = new QTextEdit(this);
	edit2->resize(edit2->width(), 100);
	edit2->sizePolicy().setVerticalPolicy(QSizePolicy::Fixed);
	edit2->setMaximumHeight(120);
	edit2->setMinimumHeight(120);
	tree = new QTreeWidget(this);
	layout_w->addWidget(tree);
	layout_w->addWidget(edit2);

	QWebElement root = this->webview->page()->mainFrame()->documentElement();
    tree->addTopLevelItem(createTree(root));
	tree->setSelectionMode(QAbstractItemView::ExtendedSelection);

	//connect(tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(itemActivated(QTreeWidgetItem*, int)));
	connect(edit, SIGNAL(returnPressed()), this, SLOT(processXPath()));
	connect(tree, SIGNAL(itemSelectionChanged()), this, SLOT(processSelection()));
}


XPathInspector::~XPathInspector()
{
	/*delete tree;
	delete edit2;
	delete edit;
	delete label;
	delete layout2;
	delete layout_w;*/
}


QTreeWidgetItem* XPathInspector::createTree(QWebElement root)
{
	QString js = "var arr = []; ";
    js +=        "for (var i=0; i<this.childNodes.length; i++) ";
    js +=        "   if (this.childNodes[i].nodeType == 3 && this.childNodes[i].nodeValue.replace(/^\\s+|\\s+$/g,'') != '') ";
    js +=        "      arr.push(this.childNodes[i].nodeValue); ";
	js +=        "   else if (this.childNodes[i].nodeType == 1)";
	js +=        "      arr.push(this.childNodes[i].tagName); ";
    js +=        "arr; ";
	QVariant v = root.evaluateJavaScript(js);
	QStringList nodes = v.toStringList();

	QWebElement child = root.firstChild();
	QList<QTreeWidgetItem*> list;
	int i = 0;
	while (!(i>=nodes.count() && child.isNull()))
	{
		while (i<nodes.count() && nodes.at(i) != child.tagName())
		{
			QTreeWidgetItem *item2 = new QTreeWidgetItem();
			item2->setText(0, nodes.at(i));
			list.append(item2);
			i++;
		}
		if (!child.isNull())
		{
			list.append(createTree(child));
			child = child.nextSibling();
			i++;
		}
	}

	QString text = "<"+root.tagName().toLower();
	QStringList attrs = root.attributeNames();
	for (int i=0; i<attrs.count(); i++)
		text += " "+attrs.at(i)+"=\""+root.attribute(attrs.at(i))+"\"";
	text += ">";
	
	QTreeWidgetItem *item = new QTreeWidgetItem();
	item->setText(0, text);
	QVariant v2(QVariant::UserType);
	v2.setValue<QWebElement>(root);
	item->setData(0, Qt::UserRole, v2);
	item->addChildren(list);
	return item;
}


void XPathInspector::setActiveElement(const QWebElement & elem, bool select)
{
	if (elem.isNull()) return;

	selectElements(false);
	//if (!activeElement.isNull())
	//	activeElement.setStyleProperty("border", activeOldStyle);

	selectedElements.clear();
	selectedElements.append(elem);
	activeElement = elem;

	QString path = PhpBrowser::getxpath(elem);
	
	//activeOldStyle = activeElement.styleProperty("border", QWebElement::ComputedStyle);
	//activeElement.setStyleProperty("border", "2px dashed red");
	selectElements(true);
	
	QString attrsStr = "";
	QStringList attrs = activeElement.attributeNames();
	for (int i=0; i<attrs.size(); i++)
	{
		attrsStr += attrs.at(i) + " = \"" + elem.attribute(attrs.at(i)) +"\"\n";
	}
	attrsStr += "rect: x:"+QString::number(elem.geometry().topLeft().x())+", y:"+QString::number(elem.geometry().topLeft().y())+
	     		", w:"+QString::number(elem.geometry().width())+", h:"+QString::number(elem.geometry().height())+"\n";

	edit->setText(path);
	edit2->setPlainText(attrsStr);

	if (select)
	{
		deselectTreeItems();
		findTreeItemAndSelect(elem);
	}
}


void XPathInspector::closeEvent(QCloseEvent * event)
{
	//if (!activeElement.isNull())
		//activeElement.setStyleProperty("border", activeOldStyle);
	selectElements(false);
	QWidget::closeEvent(event);
}


void XPathInspector::itemActivated(QTreeWidgetItem *item, int column)
{
	QWebElement node = item->data(0, Qt::UserRole).value<QWebElement>();
	setActiveElement(node);
}


void XPathInspector::processSelection()
{
	QList<QTreeWidgetItem*> list = tree->selectedItems();
	if (list.count() == 1)
	{
		QWebElement node = list.at(0)->data(0, Qt::UserRole).value<QWebElement>();
		setActiveElement(node, false);
	}
	else if (list.count() > 1)
	{
		QList<QWebElement> newlist;
		for (int i=0; i<list.count(); i++)
		{
			QWebElement node = list.at(i)->data(0, Qt::UserRole).value<QWebElement>();
			newlist.append(node);
		}
		setFewActiveElements(newlist, false);
	}
}


void XPathInspector::setFewActiveElements(const QList<QWebElement> & list, bool select)
{
	if (list.count() == 0)
		return;

	QList<QStringList> xpath;
	for (int i=0; i<list.count(); i++)
	{
		QString path = PhpBrowser::getxpath(list.at(i));
		xpath.append(path.split("/"));
	}

	for (int i=1; i<xpath.count(); i++)
		if (xpath.at(i).count() != xpath.at(i-1).count())
			return;

	QStringList result;
	for (int i=0; i<xpath.at(0).count(); i++) 
	{
		bool isEqual = true;
		for (int j=1; j<xpath.count(); j++)
			if (xpath[j][i] != xpath[j-1][i])
			{
				isEqual = false;
				break;
			}
		if (isEqual)
			result.append(xpath[0][i]);
		else
			break;
	}

	for (int i=result.count(); i<xpath.at(0).count(); i++)
	{
		bool isEqual = true;
		for (int j=1; j<xpath.count(); j++)
			if (xpath[j][i] != xpath[j-1][i])
			{
				isEqual = false;
				break;
			}

		if (!isEqual)
		{
			QRegExp rx("\\[\\d+\\]");
			QString el = xpath[0][i].replace(rx, "");
			for (int j=0; j<xpath.count(); j++)
			{
				QRegExp rx2("\\[\\d+\\]");
				QString el2 = xpath[j][i].replace(rx2, "");
				if (el2 != el)
					return;
			}
			result.append(el);
		}
		else
			result.append(xpath[0][i]);
	}

	QString xpathstr = result.join("/");
	edit->setText(xpathstr);
	edit2->setPlainText("");

	selectElements(false);
	selectedElements = webview->getAllElementsByXPath(xpathstr);
	selectElements(true);

	if (select)
	{
		tree->blockSignals(true);
		deselectTreeItems();
		for (int i=0; i<selectedElements.count(); i++)
			findTreeItemAndSelect(selectedElements.at(i));
		tree->blockSignals(false);
	}
}


void XPathInspector::selectElements(bool setselect)
{
	for (int i=0; i<selectedElements.count(); i++)
		webview->selectElement(selectedElements[i], setselect);
}


void XPathInspector::processXPath()
{
	tree->blockSignals(true);
	QList<QWebElement> list = webview->getAllElementsByXPath(edit->text());
	if (list.count() == 1)
	{
		setActiveElement(list.at(0));
	}
	else if (list.count() > 1)
	{
		selectElements(false);
		selectedElements = list;
		selectElements(true);

		
		deselectTreeItems();
		for (int i=0; i<list.count(); i++)
			findTreeItemAndSelect(list.at(i));
	}
	tree->blockSignals(false);
}


QTreeWidgetItem* XPathInspector::findTreeItemAndSelect(const QWebElement & elem)
{
	QList<QWebElement> patharr;
	QWebElement current = elem;
	while (!current.isNull())
	{
		patharr.prepend(current);
		current = current.parent();
	}

	QTreeWidgetItem *topitem;
	for (int i=0; i<tree->topLevelItemCount(); i++)
	{
		QTreeWidgetItem *item = tree->topLevelItem(i);
		QWebElement node = item->data(0, Qt::UserRole).value<QWebElement>();
		if (node == patharr.at(0))
		{
			tree->expandItem(item);
			topitem = item;
			break;
		}
	}

	for (int i=1; i<patharr.count(); i++)	
	{
		for (int j=0; j<topitem->childCount(); j++)
		{
			QWebElement node = topitem->child(j)->data(0, Qt::UserRole).value<QWebElement>();
			if (patharr.at(i) == node)
			{
				topitem = topitem->child(j);
				tree->expandItem(topitem);
				break;
			}
		}
	}

	topitem->setSelected(true);
	return topitem;
}


void XPathInspector::deselectTreeItems()
{
	QList<QTreeWidgetItem*> list = tree->selectedItems();
	for (int i=0; i<list.count(); i++)
		list[i]->setSelected(false);
}