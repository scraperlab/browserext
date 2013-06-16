#ifndef _QDOMNODEMODEL_H_
#define _QDOMNODEMODEL_H_

#include <QAbstractXmlNodeModel>
#include <QXmlNamePool>
#include <QWebElement>
#include <QList>


class MyDomNode: public QWebElement
{
	int type;  // тип элемента: 0 - элемент, 1 - аттрибут, 2 - document

public:
	MyDomNode(const QWebElement& other, bool isDocument = false):
		QWebElement(other)
	{
		if (isDocument)
			type = 2;
		else
			type = 0;

		this->attrname = "";
		this->attrval = "";
	}

	MyDomNode(const QWebElement& other, QString attrname, QString attrval):
		QWebElement(other)
	{
		type = 1;
		this->attrname = attrname;
		this->attrval = attrval;
	}

	MyDomNode(const MyDomNode & other):
		QWebElement(other)
	{
		type = other.type;
		attrname = other.attrname;
		attrval = other.attrval;
	}

	bool isAttr() const
	{
		return type == 1;
	}

	bool isDocument() const
	{
		return type == 2;
	}

	bool operator==(const MyDomNode & o) const 
	{
		if (type == o.type)
		{
			if (type == 1)
				return (QWebElement::operator==(o) && attrname.compare(o.attrname, Qt::CaseInsensitive)==0
						&& attrval.compare(o.attrval, Qt::CaseInsensitive)==0);
			else
				return QWebElement::operator==(o);
		}
		return false;
	}

	bool operator!=(const MyDomNode & o) const 
	{
		if (type == o.type)
		{
			if (type == 1)
				return (QWebElement::operator!=(o) || attrname.compare(o.attrname, Qt::CaseInsensitive)!=0
						|| attrval.compare(o.attrval, Qt::CaseInsensitive)!=0);
			else
				return QWebElement::operator!=(o);
		}
		return true;
	}

	MyDomNode & operator=(const MyDomNode & o)
	{
		QWebElement::operator=(o);
		type = o.type;
		if (type == 1)
		{
			attrname = o.attrname;
			attrval = o.attrval;
		}
		return *this;
	}

	QString attrname;
	QString attrval;
};



class QWebNodeModel: public QAbstractXmlNodeModel
{
public:
	typedef QVector<QWebElement> Path;

public:
	QWebNodeModel(QXmlNamePool &, QWebElement &);
	~QWebNodeModel();
	QUrl baseUri ( const QXmlNodeModelIndex & n ) const;
	QXmlNodeModelIndex::DocumentOrder compareOrder ( const QXmlNodeModelIndex & ni1, const QXmlNodeModelIndex & ni2 ) const;
	QUrl documentUri ( const QXmlNodeModelIndex & n ) const;
	QXmlNodeModelIndex elementById ( const QXmlName & id ) const;
	QXmlNodeModelIndex::NodeKind kind ( const QXmlNodeModelIndex & ni ) const;
	QXmlName name ( const QXmlNodeModelIndex & ni ) const;
	QVector<QXmlName> namespaceBindings ( const QXmlNodeModelIndex & n ) const;
	QVector<QXmlNodeModelIndex> nodesByIdref ( const QXmlName & idref ) const;
	QXmlNodeModelIndex root ( const QXmlNodeModelIndex & n ) const;
	QSourceLocation	sourceLocation ( const QXmlNodeModelIndex & index ) const;
	QString	stringValue ( const QXmlNodeModelIndex & n ) const;
	QVariant typedValue ( const QXmlNodeModelIndex & node ) const;

public:
	QXmlNodeModelIndex fromDomNode (const QWebElement & n, bool isDocument = false) const;
	MyDomNode* createMyDomNode(const QWebElement &n, bool isDocument = false) const;
	MyDomNode & toDomNode(const QXmlNodeModelIndex &) const;
	Path path(const MyDomNode&) const;
	int childIndex(const MyDomNode&) const;

protected:
	QVector<QXmlNodeModelIndex> attributes ( const QXmlNodeModelIndex & element ) const;
	QXmlNodeModelIndex nextFromSimpleAxis ( SimpleAxis axis, const QXmlNodeModelIndex & origin) const;

private:
	mutable QXmlNamePool m_Pool;
	mutable QWebElement m_Doc;
	mutable QList<MyDomNode*> m_ListPtr;
};

#endif // _QDOMNODEMODEL_H_
