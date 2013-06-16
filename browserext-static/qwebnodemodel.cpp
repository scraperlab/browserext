/*
This is modified version of original qdomnodemodel.cpp and qdomnodemodel.h.
This version intends for running QXmlQuery-ies on a QWebElement.
Modified by artttmen.


Copyright (c) 2011, Stanislaw Adaszewski
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Stanislaw Adaszewski nor the
      names of other contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL STANISLAW ADASZEWSKI BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "QWebNodeModel.h"

#include <QtWebKit/QWebElement>
#include <QUrl>
#include <QVector>
#include <QSourceLocation>
#include <QVariant>


QWebNodeModel::QWebNodeModel(QXmlNamePool &pool, QWebElement &doc):
	m_Pool(pool), m_Doc(doc), m_ListPtr()
{

}

QWebNodeModel::~QWebNodeModel()
{
	for (int i=0; i<m_ListPtr.size(); i++)
		delete m_ListPtr.at(i);
	m_ListPtr.clear();
}


QUrl QWebNodeModel::baseUri (const QXmlNodeModelIndex &) const
{
	// TODO: Not implemented.
	return QUrl();
}

QXmlNodeModelIndex::DocumentOrder QWebNodeModel::compareOrder (
	const QXmlNodeModelIndex & ni1,
	const QXmlNodeModelIndex & ni2 ) const
{
	MyDomNode n1 = toDomNode(ni1);
	MyDomNode n2 = toDomNode(ni2);

	if (n1 == n2)
		return QXmlNodeModelIndex::Is;

	Path p1 = path(n1);
	Path p2 = path(n2);

	for (int i = 1; i < p1.size(); i++)
		if (p1[i] == n2)
			return QXmlNodeModelIndex::Follows;

	for (int i = 1; i < p2.size(); i++)
		if (p2[i] == n1)
			return QXmlNodeModelIndex::Precedes;

	for (int i = 1; i < p1.size(); i++)
		for (int j = 1; j < p2.size(); j++)
		{
			if (p1[i] == p2[j]) // Common ancestor
			{
				int ci1 = childIndex(p1[i-1]);
				int ci2 = childIndex(p2[j-1]);

				if (ci1 < ci2)
					return QXmlNodeModelIndex::Precedes;
				else
					return QXmlNodeModelIndex::Follows;
			}
		}

	return QXmlNodeModelIndex::Precedes; // Should be impossible!
}

QUrl QWebNodeModel::documentUri (const QXmlNodeModelIndex&) const
{
	// TODO: Not implemented.
	return QUrl();
}

QXmlNodeModelIndex QWebNodeModel::elementById ( const QXmlName & id ) const
{
	return fromDomNode(m_Doc.findFirst("#"+id.localName(m_Pool)));
}

QXmlNodeModelIndex::NodeKind QWebNodeModel::kind ( const QXmlNodeModelIndex & ni ) const
{
	MyDomNode n = toDomNode(ni);
	if (n.isAttr())
		return QXmlNodeModelIndex::Attribute;
	else if (n.isDocument())
		return QXmlNodeModelIndex::Document;
	else 
		return QXmlNodeModelIndex::Element;
	
	return (QXmlNodeModelIndex::NodeKind) 0;
}

QXmlName QWebNodeModel::name ( const QXmlNodeModelIndex & ni ) const
{
	MyDomNode n = toDomNode(ni);
	if (n.isAttr())
		return QXmlName(m_Pool, n.attrname.toLower());
	else if (n.isDocument())
		return QXmlName(m_Pool, "Document");
	else
		return QXmlName(m_Pool, n.tagName().toLower());
}

QVector<QXmlName> QWebNodeModel::namespaceBindings(const QXmlNodeModelIndex&) const
{
	// TODO: Not implemented.
	return QVector<QXmlName>();
}

QVector<QXmlNodeModelIndex> QWebNodeModel::nodesByIdref(const QXmlName&) const
{
	// TODO: Not implemented.
	return QVector<QXmlNodeModelIndex>();
}

QXmlNodeModelIndex QWebNodeModel::root ( const QXmlNodeModelIndex & ni ) const
{
	MyDomNode n = toDomNode(ni);
	return fromDomNode(n.document(), true);
}

QSourceLocation	QWebNodeModel::sourceLocation(const QXmlNodeModelIndex&) const
{
	// TODO: Not implemented.
	return QSourceLocation();
}

QString	QWebNodeModel::stringValue ( const QXmlNodeModelIndex & ni ) const
{
	MyDomNode n = toDomNode(ni);
	if (n.isAttr())
		return n.attrval;
	else
		return n.toPlainText();
}

QVariant QWebNodeModel::typedValue ( const QXmlNodeModelIndex & ni ) const
{
	return qVariantFromValue(stringValue(ni));
}

MyDomNode* QWebNodeModel::createMyDomNode(const QWebElement &n, bool isDocument) const
{
	MyDomNode *p = NULL;
	MyDomNode *ptmp = NULL;
	
	bool isfind = false;
	for (int i=0; i<m_ListPtr.size(); i++)
	{
		ptmp = m_ListPtr.at(i);
		if ((n == (QWebElement)(*ptmp)) && (ptmp->isDocument() == isDocument))
		{
			p = ptmp;
			isfind = true;
			break;
		}
	}

	if (!isfind)
	{
		p = new MyDomNode(n, isDocument);
		m_ListPtr.append(p);
	}

	return p;
}

QXmlNodeModelIndex QWebNodeModel::fromDomNode(const QWebElement &n, bool isDocument) const
{
	if (n.isNull())
		return QXmlNodeModelIndex();

	MyDomNode *p = createMyDomNode(n, isDocument);		
	return createIndex((void *)p, 0);
}

MyDomNode & QWebNodeModel::toDomNode(const QXmlNodeModelIndex &ni) const
{
	MyDomNode* node = (MyDomNode*) ni.internalPointer();
	return *node;
}

QWebNodeModel::Path QWebNodeModel::path(const MyDomNode &n) const
{
	Path res;
	QWebElement doc = n.document();
	QWebElement cur = n;

	if (n.isAttr())
	{
		res.push_back(n);
	}
	
	while (!cur.isNull())
	{
		res.push_back(*createMyDomNode(cur));
		cur = cur.parent();
	}

	res.push_back(*createMyDomNode(doc, true));

	return res;
}

int QWebNodeModel::childIndex(const MyDomNode &n) const
{
	if (!n.isDocument())
	{
		if (n.isAttr())
		{
			QStringList attrs = n.parent().attributeNames();
			int j=-1;
			for (int i=0; i<attrs.size(); i++)
				if (attrs.at(i) == n.attrname) 
				{
					j = i;
					break;
				}
			return j;
		}
		else
		{
			QWebElement child = n.parent().firstChild();
			int i = 0;
			while (child != n)
			{
				i++;
				child = child.nextSibling();
			}
			return i;
		}
	}

	return -1;
}



QVector<QXmlNodeModelIndex> QWebNodeModel::attributes ( const QXmlNodeModelIndex & ni ) const
{
	MyDomNode n = toDomNode(ni);
	QStringList attrs = n.attributeNames();
	QVector<QXmlNodeModelIndex> res;
	for (int i = 0; i < attrs.size(); i++)
	{
		QString name = attrs.at(i);
		MyDomNode *p = new MyDomNode(n, name, n.attribute(name));
		m_ListPtr.append(p);
		res.push_back(createIndex((void *)p, 0));
	}
	return res;
}

QXmlNodeModelIndex QWebNodeModel::nextFromSimpleAxis ( SimpleAxis axis, const QXmlNodeModelIndex & ni) const
{
	MyDomNode n = toDomNode(ni);
	
	/*
	if (n.isDocument())
		qDebug("Document");
	else if (n.isAttr())
	{
		char buf[200] = "";
		strcpy_s(buf, 200, n.attrname.toUtf8().constData());
		strcat_s(buf, 200, " = ");
		strcat_s(buf, 200, n.attrval.toUtf8().constData());
		qDebug(buf);
	}
	else
		qDebug(n.tagName().toLower().toUtf8().constData());
	*/

	switch(axis)
	{
	case Parent:
		if (n.isAttr())
			return fromDomNode(n);
		else
			return fromDomNode(n.parent());

	case FirstChild:
		if (n.isDocument())
			return fromDomNode(n);
		else
			return fromDomNode(n.firstChild());

	case PreviousSibling:
		return fromDomNode(n.previousSibling());

	case NextSibling:
		return fromDomNode(n.nextSibling());
	}

	return QXmlNodeModelIndex();
}
